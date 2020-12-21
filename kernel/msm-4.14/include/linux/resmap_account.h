/*
 * resmap_account.h
 *
 * Provide resmap_account external call interface
 *
 */

#ifndef RESMAP_ACCOUNT_H
#define RESMAP_ACCOUNT_H

#include <linux/types.h>
#include <linux/percpu.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/atomic.h>

#define NS_PER_SEC (1000000000LLU)
#define TRIGGER_TIME_NS (300*NS_PER_SEC)

/* Only used for 32bit task, reference arch/arm64/mm/mmap.c */
#define STACK_RLIMIT_OVERFFLOW ((32 << 20) + ((0x7ff >> (PAGE_SHIFT - 12)) << PAGE_SHIFT))

enum resmap_item {
	RESMAP_ACTION,
	RESMAP_SUCCESS,
	RESMAP_FAIL,
	RESMAP_TEXIT,
	RESMAP_ITEMS
};

struct resmap_event_state {
	unsigned int event[RESMAP_TEXIT];
};

DECLARE_PER_CPU(struct resmap_event_state, resmap_event_states);

static inline void __count_resmap_event(enum resmap_item item)
{
	raw_cpu_inc(resmap_event_states.event[item]);
}

static inline void count_resmap_event(enum resmap_item item)
{
	this_cpu_inc(resmap_event_states.event[item]);
}

extern int rlimit_svm_log;
extern int reserved_area_enable;
extern int svm_oom_pid;
extern unsigned long svm_oom_jiffies;
extern char svm_oom_msg[128];
#endif
