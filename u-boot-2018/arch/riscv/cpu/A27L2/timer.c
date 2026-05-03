// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/timer.h>
#include <asm/arch/plic.h>
#include <div64.h>
#include <asm/arch/cpu.h>
#include <asm/arch/clock.h>

DECLARE_GLOBAL_DATA_PTR;

void aw_set_hosc_freq(u32 hosc_freq)
{
	if (hosc_freq == 24 || hosc_freq == 40)
		gd->hosc_freq = hosc_freq;
	else
		asm volatile("j .");
}

u32 aw_get_hosc_freq(void)
{
	return gd->hosc_freq;
}

/* init timer register */
int timer_init(void)
{
	struct sunxi_timer_reg *timers =
		(struct sunxi_timer_reg *)SUNXI_TIMER_BASE;

	timers->tirqen  = 0;
	timers->tirqsta |= 0x03;

	return 0;
}

void timer_exit(void)
{
	struct sunxi_timer_reg *timers = (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;

	timers->tirqen  = 0;
	timers->tirqsta |= 0x043f;
	timers->timer[0].ctl = 0;
	timers->timer[1].ctl = 0;

}

/*
* 64bit arch timer.CNTPCT
* Freq = gd->hosc_freq
*/
u64 notrace arch_read_timer(void)
{
	u32 lo, hi;

	do {
		hi = readl(IOMEM_ADDR(H_MTIME_REG));
		lo = readl(IOMEM_ADDR(L_MTIME_REG));
	} while (hi != readl(IOMEM_ADDR(H_MTIME_REG)));

	return ((u64)hi << 32) | (u64)lo;
}

void __udelay(unsigned long us)
{
	u64 t1, t2;

	t1 = arch_read_timer();
	t2 = t1 + us*gd->hosc_freq ;
	do {
		t1 = arch_read_timer();
	} while (t2 >= t1);
}

void __usdelay(unsigned long us)
{
	u64 t1, t2;

	t1 = arch_read_timer();
	t2 = t1 + us*gd->hosc_freq ;
	do {
		t1 = arch_read_timer();
	} while (t2 >= t1);
}

void __msdelay(unsigned long ms)
{
	__usdelay(ms * 1000);
	return ;
}

/* get the current time(ms), freq = gd->hosc_freq Hz*/
int runtime_tick(void)
{
	u64 cnt = 0;
	cnt = arch_read_timer();
	return lldiv(cnt, (gd->hosc_freq * 1000));
}

unsigned long notrace timer_get_us(void)
{
	u64 cnt = 0;
	cnt	= arch_read_timer();
	return lldiv(cnt, gd->hosc_freq);
}

ulong get_timer_masked(void)
{
	/* current tick value */
	ulong now = runtime_tick();
	return now;
}

/* timer without interrupts */
/* count the delay by seconds */
ulong get_timer(ulong base)
{
    return get_timer_masked() - base;
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	u64 cnt;

	cnt = arch_read_timer();

	return cnt;
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}

void watchdog_disable(void)
{
	struct sunxi_timer_reg *timers = (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;

	writel(0, IOMEM_ADDR(timers->wdog[0].mode));
	return ;
}

void watchdog_enable(void)
{
	struct sunxi_timer_reg *timers = (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;

	writel(1, IOMEM_ADDR(timers->wdog[0].cfg));
	writel(1, IOMEM_ADDR(timers->wdog[0].mode));

	return ;
}

static  int  timer_used_status;

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
static void timerX_callback_default(void *data);

struct __timer_callback {
	void (*func_back)(void *data);
	unsigned long data;
};

struct __timer_callback timer_callback[2] = {
	{timerX_callback_default, 0},
	{timerX_callback_default, 1}
};

static void timerX_callback_default(void *data)
{
	printf("this is only for test, timer number=%d\n", (uint)(ulong)data);
}

void timer0_func(void *data)
{
	struct sunxi_timer_reg *timer_control = (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;

	if (!(timer_control->tirqsta & 0x01)) {
		return ;
	}
	timer_control->tirqen  &= ~0x01;
	timer_control->tirqsta  =  0x01;
	irq_disable(AW_IRQ_TIMER0);
	timer_used_status &= ~1;
	pr_err("\ntimer 0 occur\n");
	timer_callback[0].func_back((void *)timer_callback[0].data);
}

void timer1_func(void *data)
{
	struct sunxi_timer_reg *timer_control = (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;

	if (!(timer_control->tirqsta & 0x02)) {
		return ;
	}
	timer_control->tirqen  &= ~0x02;
	timer_control->tirqsta  =  0x02;
	irq_disable(AW_IRQ_TIMER1);
	timer_used_status &= ~(1 << 1);
	pr_err("\ntimer 1 occur\n");
	timer_callback[1].func_back((void *)timer_callback[1].data);
}


void init_timer(struct timer_list *timer)
{
    return ;
}

__weak void clock_open_timer(int timernum)
{
	pr_info("__weak %s...%d\n", __func__, __LINE__);
	return ;
}

__weak void clock_close_timer(int timernum)
{
	pr_info("__weak %s...%d\n", __func__, __LINE__);
	return ;
}

void add_timer(struct timer_list *timer)
{
	u32 reg_val;
	int timer_num;
	struct sunxi_timer     *timer_tcontrol;
	struct sunxi_timer_reg *timer_reg;

	if (timer->expires <= 0) {
		timer->expires = 1000;
	}

	if (!timer->expires) {
		return ;
	}
	debug("timer delay time %d\n", timer->expires);
	if (!(timer_used_status & 0x01)) {
		timer_used_status |= 0x01;
		timer_num = 0;
	} else if (!(timer_used_status & 0x02)) {
		timer_used_status |= 0x02;
		timer_num = 1;
	} else {
		printf("timer err: there is no timer cound be used\n");
		return ;
	}

	clock_open_timer(timer_num);

	timer->timer_num = timer_num;
	timer_reg      =   (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;
	timer_tcontrol = &((struct sunxi_timer_reg *)SUNXI_TIMER_BASE)->timer[timer_num];
#ifndef FPGA_PLATFORM
	reg_val =	(0 << 0)  |
				(1 << 1)  |
				(1 << 2)  |
				(5 << 4)  |
				(1 << 7);
#else
	reg_val =	(0 << 0)  |
				(1 << 1)  |
				(0 << 2)  |
				(0 << 4)  |
				(1 << 7);
#endif
	timer_tcontrol->ctl = reg_val;
#ifndef FPGA_PLATFORM
	timer_tcontrol->inter = timer->expires * ((gd->hosc_freq * 1000) / 32);
#else
	timer_tcontrol->inter = timer->expires * 1000 / 32;
#endif
	timer_callback[timer_num].func_back = timer->function;
	timer_callback[timer_num].data      = timer->data;
	if (!timer_num) {
		irq_install_handler(AW_IRQ_TIMER0 + timer_num, timer0_func, (void *)&timer_callback[timer_num].data);
	} else {
		irq_install_handler(AW_IRQ_TIMER0 + timer_num, timer1_func, (void *)&timer_callback[timer_num].data);
	}
	timer_tcontrol->ctl |= (1 << 1);

	while
		(timer_tcontrol->ctl & 0x02);

	irq_enable(AW_IRQ_TIMER0 + timer_num);
	timer_tcontrol->ctl |= 1;
	timer_reg->tirqsta  |= (1 << timer_num);
	timer_reg->tirqen  |= (1 << timer_num);

	return ;
}


void del_timer(struct timer_list *timer)
{
	struct sunxi_timer *timer_tcontrol;
	struct sunxi_timer_reg *timer_reg;
	int    num = timer->timer_num;

	timer_reg      =   (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;
	timer_tcontrol = &((struct sunxi_timer_reg *)SUNXI_TIMER_BASE)->timer[num];

	clock_close_timer(num);

	irq_disable(AW_IRQ_TIMER0 + num);
	timer_tcontrol->ctl &= ~1;
	timer_reg->tirqsta &= ~(1 << num);
	timer_reg->tirqen  &= ~(1 << num);

	timer_callback[num].data = num;
	timer_callback[num].func_back = timerX_callback_default;
	timer_used_status &= ~(1 << num);

	return ;
}

