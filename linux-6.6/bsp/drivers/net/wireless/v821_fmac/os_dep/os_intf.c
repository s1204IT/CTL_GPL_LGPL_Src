#include "os_intf.h"

void xradio_k_usleep(u32 us)
{
	usleep_range(us, 10);
}

void xradio_k_msleep(u32 ms)
{
	msleep(ms);
}
void xradio_k_udelay(u32 us)
{
	udelay(us);
}

void xradio_k_mdely(u32 ms)
{
	mdelay(ms);
}

void xradio_k_spin_lock_init(xr_spinlock_t *l)
{
	spin_lock_init(&l->lock);
}

// disable all cpu(smp),soft irq, hw irq
void xradio_k_spin_lock_bh(xr_spinlock_t *l)
{
	spin_lock_bh(&l->lock);
}

void xradio_k_spin_unlock_bh(xr_spinlock_t *l)
{
	spin_unlock_bh(&l->lock);
}

void *xradio_k_zmalloc(u32 sz)
{
	return kzalloc(sz, GFP_ATOMIC | GFP_DMA);
}

void xradio_k_free(void *p)
{
	kfree(p);
}

int xradio_k_thread_create(xr_thread_handle_t *thread, const char *name, xr_thread_func_t func,
									void *arg, u8 priority, u32 stack_size)
{
	thread->handle = kthread_run(func, arg, name);
	if (IS_ERR(thread->handle)) {
		thread->handle = NULL;
		return -1;
	}
	return 0;
}

int xradio_k_thread_should_stop(xr_thread_handle_t *thread)
{
	return kthread_should_stop();
}

int xradio_k_thread_delete(xr_thread_handle_t *thread)
{
	return kthread_stop(thread->handle);
}

void xradio_k_thread_exit(xr_thread_handle_t *thread)
{
	;
}

u16 xradio_k_cpu_to_le16(u16 val)
{
	return cpu_to_le16(val);
}

u16 xradio_k_cpu_to_be16(u16 val)
{
	return cpu_to_be16(val);
}

u32 xradio_k_cpu_to_be32(u32 val)
{
	return cpu_to_be32(val);
}

void xradio_k_atomic_add(int i, xr_atomic_t *v)
{
	atomic_add(i, &v->h);
}

int xradio_k_atomic_read(xr_atomic_t *v)
{
	return atomic_read(&v->h);
}

void xradio_k_atomic_set(xr_atomic_t *v, int i)
{
	atomic_set(&v->h, i);
}

void xradio_k_atomic_dec(xr_atomic_t *v)
{
	atomic_dec(&v->h);
}

struct sk_buff *xradio_alloc_skb(u32 len, const char *func)
{
	struct sk_buff *skb = NULL;

	u8 offset;

	skb = netdev_alloc_skb(NULL, len + INTERFACE_HEADER_PADDING);

	if (skb) {
		/* Align SKB data pointer */
		offset = ((unsigned long)skb->data) & (SKB_DATA_ADDR_ALIGNMENT - 1);

		if (offset)
			skb_reserve(skb, INTERFACE_HEADER_PADDING - offset);
	}
	return skb;
}

void xradio_free_skb(struct sk_buff *skb, const char *func)
{
	if (skb) {
		dev_kfree_skb(skb);
	}
}

void xradio_free_skb_any(struct sk_buff *skb)
{
	if (skb)
		dev_kfree_skb_any(skb);
}

void xradio_k_mutex_init(xr_mutex_t *l)
{
	mutex_init(&l->mutex);
}

void xradio_k_mutex_lock(xr_mutex_t *l)
{
	mutex_lock(&l->mutex);
}

void xradio_k_mutex_unlock(xr_mutex_t *l)
{
	mutex_unlock(&l->mutex);
}

void xradio_k_mutex_deinit(xr_mutex_t *l)
{
	;
}

int xradio_k_sema_init(xr_sem_t *s, int val)
{
	sema_init(&s->sema, val);
	return 0;
}

int xradio_k_sem_deinit(xr_sem_t *s)
{
	return 0;
}

int xradio_k_sem_take(xr_sem_t *s)
{
	down(&s->sema);
	return 0;
}

int xradio_k_sem_take_timeout(xr_sem_t *s, long timeout)
{
	return down_timeout(&s->sema, timeout);
}

int xradio_k_sem_give(xr_sem_t *s)
{
	up(&s->sema);
	return 0;
}

