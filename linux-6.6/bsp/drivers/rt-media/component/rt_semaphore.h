/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _RT_SEMAPHORE_H_
#define _RT_SEMAPHORE_H_

#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/wait.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

typedef struct rt_sem_t {
	wait_queue_head_t wait_queue; //wait_queue_t
	struct mutex mutex;
	unsigned int semval;
} rt_sem_t;

int rt_sem_init(rt_sem_t *tsem, unsigned int val);
void rt_sem_deinit(rt_sem_t *tsem);

void rt_sem_down(rt_sem_t *tsem);
int rt_sem_down_timedwait(rt_sem_t *tsem, unsigned int timeout);

void rt_sem_up(rt_sem_t *tsem);
void rt_sem_up_unique(rt_sem_t *tsem);

void rt_sem_wait_unique(rt_sem_t *tsem);
int rt_sem_timedwait_unique(rt_sem_t *tsem, unsigned int timeout);

void rt_sem_reset(rt_sem_t *tsem);
unsigned int rt_sem_get_val(rt_sem_t *tsem);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _RT_SEMAPHORE_H_ */

