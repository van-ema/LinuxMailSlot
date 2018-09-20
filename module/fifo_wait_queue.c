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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include "fifo_wait_queue.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Vannacci");
MODULE_DESCRIPTION("MailSlot service via dev file driver");

long go_to_sleep(wait_queue_fifo * queue)
{
	volatile elem me;
	elem *aux;

	me.next = NULL;
	me.prev = NULL;
	me.task = current;
	me.pid = current->pid;
	me.awake = NO;
	me.already_hit = NO;

	AUDIT
	    printk("fifo sleep queue called from thread %d\n",current->pid);

	spin_lock(&queue->queue_lock);

	aux = &queue->tail;
	if (aux->prev == NULL) {
		spin_unlock(&queue->queue_lock);
		printk("malformed sleep-wakeup-queue - service damaged\n");
		return -1;
	}

	aux->prev->next = &me;
	me.prev = aux->prev;
	aux->prev = &me;
	me.next = aux;

	spin_unlock(&queue->queue_lock);

	atomic_inc(&queue->count);

	AUDIT printk("thread %d actually going to sleep\n", current->pid);

	wait_event_interruptible(queue->wq, me.awake == YES);

	spin_lock(&queue->queue_lock);

	aux = &queue->head;

	if (aux == NULL) {
		spin_unlock(&queue->queue_lock);
		printk
		    ("malformed sleep-wakeup-queue upon wakeup - service damaged\n");
		return -1;
	}

	me.prev->next = me.next;
	me.next->prev = me.prev;

	spin_unlock(&queue->queue_lock);

	AUDIT
	    printk("thread %d exiting sleep for a wakeup or signal\n",
		   current->pid);

	atomic_dec(&queue->count);	//finally awaken

	if (me.awake == NO) {
		AUDIT
		    printk("thread %d exiting sleep for signal\n",
			   current->pid);
		return -EINTR;
	}

	return 0;
}

int awake(wait_queue_fifo * queue)
{
	struct task_struct *the_task;
	int its_pid = -1;
	elem *aux;

	printk("sys_awake called from thread %d\n", current->pid);

	aux = &queue->head;

	spin_lock(&queue->queue_lock);

	if (aux == NULL) {
		spin_unlock(&queue->queue_lock);
		printk("malformed sleep-wakeup-queue\n");
		return -1;
	}

	while (aux->next != &queue->tail) {

		if (aux->next->already_hit == NO) {
			the_task = aux->next->task;
			aux->next->awake = YES;
			aux->next->already_hit = YES;
			its_pid = aux->next->pid;
			wake_up_process(the_task);
			goto awaken;
		}

		aux = aux->next;

	}

	spin_unlock(&queue->queue_lock);

	return 0;

 awaken:
	spin_unlock(&queue->queue_lock);

	AUDIT printk("called the awake of thread %d\n", its_pid);

	return its_pid;
}

int init_waitqueue_fifo(wait_queue_fifo * queue)
{
/*	queue = kmalloc(sizeof(wait_queue_fifo), GFP_KERNEL);
	if (queue == NULL) {
		printk(KERN_ERR "memory allocation error\n");
		return -1;
	}
*/
	spin_lock_init(&queue->queue_lock);

	queue->head.task = NULL;
	queue->head.pid = -1;
	queue->head.awake = -1;
	queue->head.already_hit=  -1;
	queue->head.next = NULL;
	queue->head.prev = NULL;


	queue->tail.task = NULL;
	queue->tail.pid = -1;
	queue->tail.awake = -1;
	queue->tail.already_hit = -1;
	queue->tail.next = NULL;
	queue->tail.prev = NULL;

	queue->head.next = &queue->tail;
	queue->tail.prev = &queue->head;
	if(queue->tail.prev == NULL)
		printk(KERN_DEBUG "errrrrrroooooooorrr!");
	init_waitqueue_head(&queue->wq);
	atomic_set(&queue->count, 0);

	return 0;
}
