/*
 *   Copyright (C) 2018 Emanuele Vannacci
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define EXPORT_SYMTAB
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/tty.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <asm/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Vannacci");
MODULE_DESCRIPTION("MailSlot service via dev file driver");

#define MODNAME "Mailslot"

static ssize_t mailslot_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t mailslot_write(struct file *, const char __user *, size_t,
			      loff_t *);
static long mailslot_ioctl(struct file *, unsigned int, unsigned long);
static int mailslot_open(struct inode *, struct file *);
static int mailslot_release(struct inode *, struct file *);

#define DEBUG if(1)
#define DEVICE_NAME "MailSlot"
#define MSG_SZ 256
#define MAILSLOT_SZ (MSG_SZ * 1000)
#define N_INSTANCES 256

static int Major;
static unsigned int max_msg_size = MSG_SZ;
int errno;

typedef struct mailslot_msg_t {
	struct mailslot_msg_t *next;
	char *data;
	unsigned int size;
} mailslot_msg_t;

typedef struct mailslot_t {
	mailslot_msg_t *first;
	mailslot_msg_t *last;
	size_t size;
	struct mutex mtx;
	wait_queue_head_t reader_queue;
	wait_queue_head_t writer_queue;
} mailslot_t;

static mailslot_t mailslot[N_INSTANCES];

mailslot_msg_t *new_msg(char *data, int len)
{
	mailslot_msg_t *msg = kmalloc(sizeof(mailslot_msg_t), GFP_KERNEL);
	if (!msg) {
		errno = ENOMEM;
		return NULL;
	}

	msg->size = len;
	msg->data = kmalloc(sizeof(char) * len, GFP_KERNEL);
	if (!msg->data) {
		errno = ENOMEM;
		return NULL;
	}

	strncpy(msg->data, data, len);
	msg->next = NULL;
	return msg;
}

static ssize_t push(int minor, mailslot_msg_t * msg)
{
	mutex_lock(&mailslot[minor].mtx);

	if (mailslot[minor].size + msg->size > MAILSLOT_SZ) {
		printk(KERN_INFO "%s: message cannot be accepted because it "
		       "exceed available memory\n", MODNAME);
		kfree(msg);
		return -ERANGE;
	}

	if (mailslot[minor].last == NULL) {
		mailslot[minor].first = msg;
		mailslot[minor].last = msg;
		goto exit;
	}

	mailslot[minor].last->next = msg;
	mailslot[minor].last = msg;

 exit:
	mailslot[minor].size += msg->size;

	DEBUG
	    printk(KERN_DEBUG "%s: minor:%d size %ld\n", MODNAME, minor,
		   mailslot[minor].size);

	mutex_unlock(&mailslot[minor].mtx);

	return msg->size;
}

static mailslot_msg_t *pull(int minor, size_t len)
{
	mailslot_msg_t *msg;

	mutex_lock(&mailslot[minor].mtx);

	if (mailslot[minor].first == NULL || mailslot[minor].first->size > len) {
		mutex_unlock(&mailslot[minor].mtx);
		return NULL;
	}

	msg = mailslot[minor].first;
	mailslot[minor].first = mailslot[minor].first->next;
	mailslot[minor].size -= msg->size;

	DEBUG
	    printk(KERN_DEBUG "%s: minor:%d size %ld\n", MODNAME, minor,
		   mailslot[minor].size);

	mutex_unlock(&mailslot[minor].mtx);
	return msg;
}

static ssize_t mailslot_write(struct file *filp, const char *buff, size_t len,
			      loff_t * off)
{
	int size;
	mailslot_msg_t *msg;
	char kbuf[len];

	if (len > max_msg_size) {
		printk(KERN_ERR "%s: messagge too long\n", MODNAME);
		return -EMSGSIZE;
	}

	if (copy_from_user(kbuf, buff, len)) {
		printk(KERN_ERR "%s: memory coping from user error.", MODNAME);
		return -ENOMEM;
	}

	errno = 0;
	msg = new_msg(kbuf, len);
	if (errno != 0) {
		printk(KERN_ERR
		       "mailslot message allocation failed with error code %d\n",
		       errno);
		return -errno;
	}

	size = push(MINOR(filp->f_path.dentry->d_inode->i_rdev), msg);

	DEBUG
	    printk(KERN_DEBUG "%s: written: %.*s\n", MODNAME, (int)msg->size,
		   msg->data);

	return size;
}

static ssize_t mailslot_read(struct file *filp, char __user * buff, size_t len,
			     loff_t * off)
{
	int ret;
	int minor;
	mailslot_msg_t *msg;

	minor = MINOR(filp->f_path.dentry->d_inode->i_rdev);

	msg = pull(minor, len);
	if (msg == NULL) {
		printk(KERN_ERR "%s: no message to read\n", MODNAME);
		return 0;
	}

	ret = copy_to_user(buff, msg->data, msg->size);
	if (ret) {
		DEBUG
		    printk(KERN_ERR "%s: error delivering data to user\n",
			   MODNAME);
		return -ENOMEM;
	}

	DEBUG
	    printk(KERN_DEBUG "%s: read: %.*s\n", MODNAME, msg->size,
		   msg->data);

	kfree(msg->data);
	kfree(msg);

	return msg->size;
}

static long mailslot_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg)
{
	return 0;
}

static int mailslot_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int mailslot_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations fops = {
	.read = mailslot_read,
	.write = mailslot_write,
	.unlocked_ioctl = mailslot_ioctl,
	.open = mailslot_open,
	.release = mailslot_release
};

int init_module(void)
{
	int i;
	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
		printk("Registering Mailslot device failed\n");
		return Major;
	}

	printk(KERN_INFO "Mailslot device registered,"
	       " it is assigned major number %d\n", Major);

	for (i = 0; i < N_INSTANCES; i++) {
		mutex_init(&mailslot[i].mtx);
		init_waitqueue_head(&mailslot[i].reader_queue);
		init_waitqueue_head(&mailslot[i].writer_queue);
		mailslot[i].size = 0;
		mailslot[i].first = NULL;
		mailslot[i].last = NULL;
	}

	printk(KERN_INFO "%s: initialization terminated\n", MODNAME);
	return 0;
}

void cleanup_module(void)
{

	unregister_chrdev(Major, DEVICE_NAME);

	printk(KERN_INFO "Mailslot device unregistered,"
	       " it was assigned major number %d\n", Major);
}
