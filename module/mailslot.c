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
#include <asm/atomic.h>
#include "mailslot.h"
#include "fifo_wait_queue.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Vannacci");
MODULE_DESCRIPTION("MailSlot service via dev file driver");

#define MODNAME "Mailslot"

static ssize_t ms_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t ms_write(struct file *, const char __user *, size_t, loff_t *);
static long ms_ioctl(struct file *, unsigned int, unsigned long);
static int ms_open(struct inode *, struct file *);
static int ms_release(struct inode *, struct file *);

#define DEBUG if(1)
#define DEVICE_NAME "MailSlot"
#define MAX_MSG_SZ 8
#define MAX_MS_SZ (MAX_MSG_SZ * 5)
#define N_MAX_INSTANCES 256
#define N_START_INSTANCES 256

/*
 * Policy
 */
#define BLOCKING 0
#define NO_BLOCKING 1

static int Major;
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
	wait_queue_fifo reader_queue;
	wait_queue_fifo writer_queue;

	unsigned int max_msg_size;
	unsigned int max_ms_size;

	atomic_t count;
} mailslot_t;

typedef struct metadata_t {
	short policy;
} metadata;

static mailslot_t mailslot[N_START_INSTANCES];

mailslot_msg_t *alloc_msg(int len)
{
	mailslot_msg_t *msg = kmalloc(sizeof(mailslot_msg_t), GFP_KERNEL);
	if (!msg) {
		errno = ENOMEM;
		return NULL;
	}

	msg->data = kmalloc(sizeof(char) * len, GFP_KERNEL);
	if (!msg->data) {
		errno = ENOMEM;
		return NULL;
	}

	msg->next = NULL;
	msg->size = len;

	return msg;
}

int full(int minor, int len)
{
	return mailslot[minor].size + len >= mailslot[minor].max_ms_size;
}

int empty(int minor)
{
	return mailslot[minor].size == 0;
}

static ssize_t push(int minor, mailslot_msg_t * msg, short policy)
{
	mailslot_t *ms = mailslot + minor;

	mutex_lock(&ms->mtx);

	if (msg->size > ms->max_msg_size) {
		printk(KERN_ERR "%s: messagge too long\n", MODNAME);
		mutex_unlock(&ms->mtx);
		return -EMSGSIZE;
	}

	while (full(minor, msg->size)) {

		mutex_unlock(&ms->mtx);

		if (policy == NO_BLOCKING) {
			printk(KERN_INFO
			       "%s: message cannot be accepted because it "
			       "exceed available memory\n", MODNAME);
			return -ERANGE;
		} else {
			go_to_sleep(&ms->writer_queue);
			mutex_lock(&ms->mtx);
		}
	}

	if (empty(minor)) {
		ms->first = msg;
		ms->last = msg;
	} else {
		ms->last->next = msg;
		ms->last = msg;
	}

	ms->size += msg->size;

	mutex_unlock(&ms->mtx);
	// the queue is not-empty: wake up a reader
	awake(&ms->reader_queue);

	return msg->size;
}

static mailslot_msg_t *pull(int minor, size_t len, short policy)
{
	mailslot_msg_t *msg;
	mailslot_t *ms = mailslot + minor;

	mutex_lock(&ms->mtx);

	while (empty(minor)) {

		mutex_unlock(&mailslot[minor].mtx);
		if (policy == NO_BLOCKING) {
			errno = EMSGSIZE;
			return NULL;
		} else {
			go_to_sleep(&ms->reader_queue);
			mutex_lock(&mailslot[minor].mtx);
		}
	}

	if(ms->first->size > len) {
		mutex_unlock(&ms->mtx);

		// maybe the next reader can get the message
		if(policy == BLOCKING)
			awake(&ms->reader_queue);

		errno = EMSGSIZE;
		return NULL;
	}

	// dequeue message and update queue's head
	msg = ms->first;
	ms->first = ms->first->next;
	ms->size -= msg->size;

	mutex_unlock(&ms->mtx);
	// the queue is not-full: wake up a writer
	awake(&ms->writer_queue);
	return msg;
}

static ssize_t
ms_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	int minor, size;
	short policy;
	metadata *mdata;
	mailslot_msg_t *msg;

	minor = MINOR(filp->f_path.dentry->d_inode->i_rdev);
	mdata = (metadata *) filp->private_data;
	policy = mdata->policy;

	errno = 0;
	msg = alloc_msg(len);
	if (msg == NULL || errno != 0) {
		printk(KERN_ERR "message allocation error: %d\n", errno);
		return -errno;
	}

	if (copy_from_user(msg->data, buff, len)) {
		printk(KERN_ERR "%s: memory coping from user error.", MODNAME);
		return -ENOMEM;
	}

	size = push(minor, msg, policy);
	if (size < 0) {
		kfree(msg);
		return size;
	}

	DEBUG
	    printk(KERN_DEBUG "%s: written: %.*s\n", MODNAME, (int)msg->size,
		   msg->data);

	return size;
}

static ssize_t
ms_read(struct file *filp, char __user * buff, size_t len, loff_t * off)
{
	int ret, minor;
	short p;
	mailslot_msg_t *msg;
	metadata *data;

	minor = MINOR(filp->f_path.dentry->d_inode->i_rdev);

	data = (metadata *) filp->private_data;
	p = data->policy;

	DEBUG
	    printk(KERN_DEBUG
		   "%s: reading minor:%d, policy:%d (0 for BLOCKING)\n",
		   MODNAME, minor, p);

	msg = pull(minor, len, p);
	if (msg == NULL) {
		printk(KERN_ERR "%s: no message to read\n", MODNAME);
		return 0;
	}

	ret = copy_to_user(buff, msg->data, msg->size);
	if (ret) {
		DEBUG printk(KERN_ERR "%s: error delivering data to user\n",
			     MODNAME);
		return -ENOMEM;
	}

	DEBUG printk(KERN_DEBUG "%s: read: %.*s\n", MODNAME, msg->size,
		     msg->data);

	kfree(msg->data);
	kfree(msg);

	return msg->size;
}

static long ms_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	metadata *data;
	int minor = MINOR(filp->f_path.dentry->d_inode->i_rdev);
	mailslot_t *ms = mailslot + minor;

	if (_IOC_TYPE(cmd) != MAILSLOT_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > MS_IOC_MAXR)
		return -ENOTTY;

/*
  	if(_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void _ _user *) arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void _ _user *) arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;
*/

	switch (cmd) {
	case MS_IOCSPOLICY:
		data = (metadata *) filp->private_data;
		data->policy = arg == NO_BLOCKING ? NO_BLOCKING : BLOCKING;
		break;

	case MS_IOCQPOLICY:
		data = filp->private_data;
		return data->policy;

	case MS_IOCSMSGSZ:
		ms->max_msg_size = arg > MAX_MSG_SZ ? MAX_MSG_SZ : arg;
		break;

	case MS_IOCGMSGSZ:
		return ms->max_msg_size;

	case MS_IOCSSZ:
		ms->max_ms_size = arg > MAX_MS_SZ ? MAX_MS_SZ : arg;
		break;

	case MS_IOCGSZ:
		return ms->max_ms_size;
	}

	return 0;
}

static int ms_open(struct inode *inode, struct file *file)
{
	metadata *data;
	int minor = MINOR(file->f_path.dentry->d_inode->i_rdev);
	if (minor < 0 || minor > 255) {
		printk(KERN_ERR "%s: Opening file with bad minor number\n",
		       MODNAME);
		return -ENXIO;
	}
	// private_data is used to keep policy info
	file->private_data = kmalloc(sizeof(metadata), GFP_KERNEL);
	if (file->private_data == NULL) {
		printk(KERN_ERR "%s: memory allocation failure\n", MODNAME);
		return -ENOMEM;
	}
	data = (metadata *) file->private_data;
	data->policy = (file->f_flags & O_NONBLOCK) >> 11;

	DEBUG
	    printk(KERN_DEBUG "%s: opening file with minor %d, policy %d",
		   MODNAME, minor, data->policy);

	atomic_inc(&mailslot[minor].count);
	return 0;
}

static int ms_release(struct inode *inode, struct file *file)
{

	int minor = MINOR(file->f_path.dentry->d_inode->i_rdev);
	kfree(file->private_data);
	atomic_dec(&mailslot[minor].count);
	return 0;
}

static struct file_operations fops = {
	.read = ms_read,
	.write = ms_write,
	.unlocked_ioctl = ms_ioctl,
	.open = ms_open,
	.release = ms_release
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

	for (i = 0; i < N_START_INSTANCES; i++) {
		mutex_init(&mailslot[i].mtx);
		init_waitqueue_fifo(&mailslot[i].reader_queue);
		init_waitqueue_fifo(&mailslot[i].writer_queue);
		mailslot[i].first = NULL;
		mailslot[i].last = NULL;
		mailslot[i].max_ms_size = MAX_MS_SZ;
		mailslot[i].max_msg_size = MAX_MSG_SZ;

		atomic_set(&mailslot[i].count, 0);
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
