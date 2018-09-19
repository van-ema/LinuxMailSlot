#ifndef _MAILSLOT_H
#define _MAILSLOT_H

#include <linux/ioctl.h>

/* magic number */
#define MAILSLOT_IOC_MAGIC 'm'

/*
 * S means "Set": directly with the argument value
 * G means "Get": response is on the return value
 */

#define MS_IOCSPOLICY	_IOW(MAILSLOT_IOC_MAGIC,1, int)
#define MS_IOCQPOLICY	_IO(MAILSLOT_IOC_MAGIC,	2)
#define MS_IOCSMSGSZ	_IOW(MAILSLOT_IOC_MAGIC,3, int)
#define MS_IOCGMSGSZ	_IO(MAILSLOT_IOC_MAGIC, 4)
#define MS_IOCSSZ	_IOW(MAILSLOT_IOC_MAGIC,5, int)
#define MS_IOCGSZ	_IO(MAILSLOT_IOC_MAGIC, 6)

#define MS_IOC_MAXR 6

#endif
