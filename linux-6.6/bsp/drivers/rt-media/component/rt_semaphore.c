/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "rt_common.h"
#include "rt_semaphore.h"

/** Initializes the semaphore at a given value
 *
 * @param tsem the semaphore to initialize
 * @param val the initial value of the semaphore
 *
 */
int rt_sem_init(rt_sem_t *tsem, unsigned int val)
{
	init_waitqueue_head(&tsem->wait_queue);
	mutex_init(&tsem->mutex);
	tsem->semval = val;
	return 0;
}

/** Destroy the semaphore
 *
 * @param tsem the semaphore to destroy
 */
void rt_sem_deinit(rt_sem_t *tsem)
{
}

/** Decreases the value of the semaphore. Blocks if the semaphore
 * value is zero.
 *
 * @param tsem the semaphore to decrease
 */
void rt_sem_down(rt_sem_t *tsem)
{
	mutex_lock(&tsem->mutex);
	//RT_LOGD("semdown:%p val:%d", tsem, tsem->semval);
	while (tsem->semval == 0) {
		mutex_unlock(&tsem->mutex);
		//RT_LOGD("semdown wait:%p val:%d", tsem, tsem->semval);
		wait_event(tsem->wait_queue, (tsem->semval > 0));
		//RT_LOGD("semdown wait end:%p val:%d",tsem,tsem->semval);
		mutex_lock(&tsem->mutex);
		if (0 == tsem->semval) {
			RT_LOGE("fatal error! why tsem:%p val is 0?", tsem);
		}
	}
	tsem->semval--;
	mutex_unlock(&tsem->mutex);
}

/**
  decrease semval with timeout.

  @param timeout
	unit:ms
  @return
    0:sem down success.
    ETIMEDOUT: timeout
*/
int rt_sem_down_timedwait(rt_sem_t *tsem, unsigned int timeout)
{
	int result;
	int ret = 0;
	mutex_lock(&tsem->mutex);
	while (tsem->semval == 0) {
		mutex_unlock(&tsem->mutex);
		ret = wait_event_timeout(tsem->wait_queue, (tsem->semval > 0), msecs_to_jiffies(timeout));
		mutex_lock(&tsem->mutex);
		if (0 == ret) {
			if (tsem->semval != 0) {
				RT_LOGE("fatal error! wait event timeout, why semval[%d]!=0", tsem->semval);
			}
			break;
		} else {
			if (0 == tsem->semval) {
				RT_LOGE("fatal error! wait event success, remaining jiffies:%d, why semval==0?", ret);
			}
		}
	}
	if (tsem->semval > 0) {
		tsem->semval--;
		result = 0;
	} else {
		result = ETIMEDOUT;
	}
	mutex_unlock(&tsem->mutex);
	return result;
}

/** Increases the value of the semaphore
 *
 * @param tsem the semaphore to increase
 */
void rt_sem_up(rt_sem_t *tsem)
{
	mutex_lock(&tsem->mutex);
	tsem->semval++;
	wake_up(&tsem->wait_queue);
	mutex_unlock(&tsem->mutex);
}

/**
  up sem, max to 1.
*/
void rt_sem_up_unique(rt_sem_t *tsem)
{
	mutex_lock(&tsem->mutex);
	if (0 == tsem->semval) {
		tsem->semval++;
		wake_up(&tsem->wait_queue);
	}
	mutex_unlock(&tsem->mutex);
}

void rt_sem_wait_unique(rt_sem_t *tsem)
{
	int ret = 0;
	mutex_lock(&tsem->mutex);
	while (0 == tsem->semval) {
		mutex_unlock(&tsem->mutex);
		wait_event(tsem->wait_queue, (tsem->semval > 0));
		mutex_lock(&tsem->mutex);
		if (0 == tsem->semval) {
			RT_LOGE("fatal error! why tsem:%p val is 0?", tsem);
		}
	}
	mutex_unlock(&tsem->mutex);
}

/**
  @timeout unit:ms
  @return
    0:success
    ETIMEDOUT: fail
*/
int rt_sem_timedwait_unique(rt_sem_t *tsem, unsigned int timeout)
{
	int result;
	int ret = 0;
	mutex_lock(&tsem->mutex);
	while (0 == tsem->semval) {
		mutex_unlock(&tsem->mutex);
		ret = wait_event_timeout(tsem->wait_queue, (tsem->semval > 0), msecs_to_jiffies(timeout));
		mutex_lock(&tsem->mutex);
		if (0 == ret) {
			if (tsem->semval != 0) {
				RT_LOGE("fatal error! wait event timeout, why semval[%d]!=0", tsem->semval);
			}
			break;
		} else {
			if (0 == tsem->semval) {
				RT_LOGE("fatal error! wait event success, remaining jiffies:%d, why semval==0?", ret);
			}
		}
	}
	if (tsem->semval > 0) {
		result = 0;
	} else {
		result = ETIMEDOUT;
	}
	mutex_unlock(&tsem->mutex);
	return result;
}

/** Reset the value of the semaphore
 *
 * @param tsem the semaphore to reset
 */
void rt_sem_reset(rt_sem_t *tsem)
{
	mutex_lock(&tsem->mutex);
	tsem->semval = 0;
	mutex_unlock(&tsem->mutex);
}

unsigned int rt_sem_get_val(rt_sem_t *tsem)
{
	mutex_lock(&tsem->mutex);
	unsigned int val = tsem->semval;
	mutex_unlock(&tsem->mutex);
	return val;
}

