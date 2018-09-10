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

static ssize_t mailslot_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t mailslot_write(struct file *, const char __user *, size_t, loff_t *);
static long mailslot_ioctl(struct file *, unsigned int, unsigned long);
static int mailslot_open(struct inode *, struct file *);
static int mailslot_release(struct inode *, struct file *);

#define DEVICE_NAME "MailSlot"

static int Major;

static ssize_t mailslot_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
	return 0;
}


static ssize_t mailslot_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	return 0;
}


static long mailslot_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
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
	.read   	= mailslot_read,
        .write   	= mailslot_write,
	.unlocked_ioctl = mailslot_ioctl,
        .open    	= mailslot_open,
        .release 	= mailslot_release
};

int init_module(void)
{

	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
	  printk("Registering Mailslot device failed\n");
	  return Major;
	}

	printk(KERN_INFO "Mailslot device registered, it is assigned major number %d\n", Major);


	return 0;
}

void cleanup_module(void)
{

	unregister_chrdev(Major, DEVICE_NAME);

	printk(KERN_INFO "Mailslot device unregistered, it was assigned major number %d\n", Major);
}
