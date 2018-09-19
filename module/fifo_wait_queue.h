#ifndef _FIFO_WAIT_QUEUE_H
#define _FIFO_WAIT_QUEUE_H

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>

#define NO (0)
#define YES (NO+1)
#define AUDIT if(1)

typedef struct _elem {
	struct task_struct *task;
	int pid;
	int awake;
	int already_hit;
	struct _elem *next;
	struct _elem *prev;
} elem;

typedef struct list {
	elem head;
	elem tail;
	wait_queue_head_t wq;
	spinlock_t queue_lock;
	atomic_t count;
} wait_queue_fifo;

int init_waitqueue_fifo(wait_queue_fifo * queue);
long go_to_sleep(wait_queue_fifo * queue);
int awake(wait_queue_fifo * queue);

#endif
