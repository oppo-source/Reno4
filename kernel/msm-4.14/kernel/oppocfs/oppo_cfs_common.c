/**********************************************************************************
* Copyright (c), 2008-2019 , Guangdong OPPO Mobile Comm Corp., Ltd.
* VENDOR_EDIT
* File: oppo_cfs_common.c
* Description: UI First
* Version: 2.0
* Date: 2019-10-01
* Author: Liujie.Xie@TECH.BSP.Kernel.Sched
* ------------------------------ Revision History: --------------------------------
* <version>           <date>                <author>                            <desc>
* Revision 1.0        2019-05-22       Liujie.Xie@TECH.BSP.Kernel.Sched      Created for UI First
* Revision 2.0        2019-10-01       Liujie.Xie@TECH.BSP.Kernel.Sched      Add for UI First 2.0
***********************************************************************************/

#include <linux/version.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/jiffies.h>
#include <trace/events/sched.h>
#include <../kernel/sched/sched.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <../fs/proc/internal.h>

#include <linux/oppocfs/oppo_cfs_common.h>

int ux_min_sched_delay_granularity;     /*ux thread delay upper bound(ms)*/
int ux_max_dynamic_exist = 1000;        /*ux dynamic max exist time(ms)*/
int ux_max_dynamic_granularity = 32;
int ux_min_migration_delay = 10;        /*ux min migration delay time(ms)*/
int ux_max_over_thresh = 2000; /* ms */
#define S2NS_T 1000000

static int entity_before(struct sched_entity *a,
			struct sched_entity *b)
{
	return (s64)(a->vruntime - b->vruntime) < 0;
}

static int entity_over(struct sched_entity *a,
			struct sched_entity *b)
{
	return (s64)(a->vruntime - b->vruntime) > (s64)ux_max_over_thresh * S2NS_T;
}

extern const struct sched_class fair_sched_class;
/* return true: this task can be treated as a ux task, which means it will be sched first and so on */
inline bool test_task_ux(struct task_struct *task)
{
	u64 now = 0;

	if (task && task->sched_class != &fair_sched_class)
		return false;

	if (task && task->static_ux)
		return true;

	now = jiffies_to_nsecs(jiffies);
	if (task && atomic64_read(&task->dynamic_ux) &&
		(now - task->dynamic_ux_start) <= (u64)ux_max_dynamic_granularity * S2NS_T)
		return true;

	return false;
}

void enqueue_ux_thread(struct rq *rq, struct task_struct *p)
{
	struct list_head *pos, *n;
	bool exist = false;

	if (!rq || !p || !list_empty(&p->ux_entry)) {
		return;
	}
	p->enqueue_time = rq->clock;
	if (test_task_ux(p)) {
		list_for_each_safe(pos, n, &rq->ux_thread_list) {
			if (pos == &p->ux_entry) {
				exist = true;
				break;
			}
		}
		if (!exist) {
			list_add_tail(&p->ux_entry, &rq->ux_thread_list);
			get_task_struct(p);
		}
	}
}

void dequeue_ux_thread(struct rq *rq, struct task_struct *p)
{
	struct list_head *pos, *n;
	u64 now =  jiffies_to_nsecs(jiffies);

	if (!rq || !p) {
		return;
	}
	p->enqueue_time = 0;
	if (!list_empty(&p->ux_entry)) {
		if (atomic64_read(&p->dynamic_ux) && (now - p->dynamic_ux_start) > (u64)ux_max_dynamic_exist * S2NS_T) {
			atomic64_set(&p->dynamic_ux, 0);
		}
		list_for_each_safe(pos, n, &rq->ux_thread_list) {
			if (pos == &p->ux_entry) {
				list_del_init(&p->ux_entry);
				put_task_struct(p);
				return;
			}
		}
	}
}

static struct task_struct *pick_first_ux_thread(struct rq *rq)
{
	struct list_head *ux_thread_list = &rq->ux_thread_list;
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	struct task_struct *temp = NULL;
	struct task_struct *leftmost_task = NULL;
	list_for_each_safe(pos, n, ux_thread_list) {
		temp = list_entry(pos, struct task_struct, ux_entry);
		/*ensure ux task in current rq cpu otherwise delete it*/
		if (unlikely(task_cpu(temp) != rq->cpu)) {
			//ux_warn("task(%s,%d,%d) does not belong to cpu%d", temp->comm, task_cpu(temp), temp->policy, rq->cpu);
			list_del_init(&temp->ux_entry);
			continue;
		}
		if (!test_task_ux(temp)) {
			continue;
		}
		if (leftmost_task == NULL) {
			leftmost_task = temp;
		} else if (entity_before(&temp->se, &leftmost_task->se)) {
			leftmost_task = temp;
		}
	}

	return leftmost_task;
}

void pick_ux_thread(struct rq *rq, struct task_struct **p, struct sched_entity **se)
{
	struct task_struct *ori_p;
	struct task_struct *key_task;
	struct sched_entity *key_se;
	if (!rq || !p || !se) {
		return;
	}
	ori_p = *p;
	if (ori_p && !ori_p->static_ux && !atomic64_read(&ori_p->dynamic_ux)) {
		if (!list_empty(&rq->ux_thread_list)) {
			key_task = pick_first_ux_thread(rq);
			/* in case that ux thread keep running too long */
			if (key_task && entity_over(&key_task->se, &ori_p->se))
				return;

			if (key_task) {
				key_se = &key_task->se;
				if (key_se && (rq->clock >= key_task->enqueue_time) &&
				rq->clock - key_task->enqueue_time >= ((u64)ux_min_sched_delay_granularity * S2NS_T)) {
					*p = key_task;
					*se = key_se;
				}
			}
		}
	}
}

#define DYNAMIC_UX_SEC_WIDTH   8
#define DYNAMIC_UX_MASK_BASE   0x00000000ff

#define dynamic_ux_offset_of(type) (type * DYNAMIC_UX_SEC_WIDTH)
#define dynamic_ux_mask_of(type) ((u64)(DYNAMIC_UX_MASK_BASE) << (dynamic_ux_offset_of(type)))
#define dynamic_ux_get_bits(value, type) ((value & dynamic_ux_mask_of(type)) >> dynamic_ux_offset_of(type))
#define dynamic_ux_one(type) ((u64)1 << dynamic_ux_offset_of(type))


bool test_dynamic_ux(struct task_struct *task, int type)
{
	u64 dynamic_ux;
	if (!task) {
		return false;
	}
	dynamic_ux = atomic64_read(&task->dynamic_ux);
	return dynamic_ux_get_bits(dynamic_ux, type) > 0;
}

static bool test_task_exist(struct task_struct *task, struct list_head *head)
{
	struct list_head *pos, *n;
	list_for_each_safe(pos, n, head) {
		if (pos == &task->ux_entry) {
			return true;
		}
	}
	return false;
}

static inline void dynamic_ux_dec(struct task_struct *task, int type)
{
	atomic64_sub(dynamic_ux_one(type), &task->dynamic_ux);
}

static inline void dynamic_ux_inc(struct task_struct *task, int type)
{
	atomic64_add(dynamic_ux_one(type), &task->dynamic_ux);
}

static void __dynamic_ux_dequeue(struct task_struct *task, int type)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0)
	unsigned long flags;
#else
	struct rq_flags flags;
#endif
	bool exist = false;
	struct rq *rq = NULL;
	u64 dynamic_ux = 0;

	rq = task_rq_lock(task, &flags);
	dynamic_ux = atomic64_read(&task->dynamic_ux);
	if (dynamic_ux <= 0) {
		task_rq_unlock(rq, task, &flags);
		return;
	}
	dynamic_ux_dec(task, type);
	dynamic_ux = atomic64_read(&task->dynamic_ux);
	if (dynamic_ux > 0) {
		task_rq_unlock(rq, task, &flags);
		return;
	}
	task->ux_depth = 0;

	exist = test_task_exist(task, &rq->ux_thread_list);
	if (exist) {
		list_del_init(&task->ux_entry);
		put_task_struct(task);
	}
	task_rq_unlock(rq, task, &flags);
}

void dynamic_ux_dequeue(struct task_struct *task, int type)
{
	if (!task || type >= DYNAMIC_UX_MAX) {
		return;
	}
	__dynamic_ux_dequeue(task, type);
}

static void __dynamic_ux_enqueue(struct task_struct *task, int type, int depth)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0)
	unsigned long flags;
#else
	struct rq_flags flags;
#endif
	bool exist = false;
	struct rq *rq = NULL;

	rq = task_rq_lock(task, &flags);
/*
	if (task->sched_class != &fair_sched_class) {
		task_rq_unlock(rq, task, &flags);
		return;
	}
*/
	if (unlikely(!list_empty(&task->ux_entry))) {
		//ux_warn("task(%s,%d,%d) is already in another list", task->comm, task->pid, task->policy);
		task_rq_unlock(rq, task, &flags);
		return;
	}

	dynamic_ux_inc(task, type);
	task->dynamic_ux_start = jiffies_to_nsecs(jiffies);
	task->ux_depth = task->ux_depth > depth + 1 ? task->ux_depth : depth + 1;
	if (task->state == TASK_RUNNING && task->sched_class == &fair_sched_class) {
		exist = test_task_exist(task, &rq->ux_thread_list);
		if (!exist) {
			get_task_struct(task);
			list_add_tail(&task->ux_entry, &rq->ux_thread_list);
		}
	}
	task_rq_unlock(rq, task, &flags);
}

void dynamic_ux_enqueue(struct task_struct *task, int type, int depth)
{
	if (!task || type >= DYNAMIC_UX_MAX) {
		return;
	}
	__dynamic_ux_enqueue(task, type, depth);
}

inline bool test_task_ux_depth(int ux_depth)
{
	return ux_depth < UX_DEPTH_MAX;
}

inline bool test_set_dynamic_ux(struct task_struct *tsk)
{
	return tsk && (tsk->static_ux || atomic64_read(&tsk->dynamic_ux)) &&
		test_task_ux_depth(tsk->ux_depth);
}

void ux_init_rq_data(struct rq *rq)
{
	if (!rq) {
		return;
	}
	rq->active_ux_balance = 0;
	INIT_LIST_HEAD(&rq->ux_thread_list);
}

int ux_prefer_cpu[NR_CPUS] = {0};
void ux_init_cpu_data(void) {
	int i = 0;
	int min_cpu = 0, ux_cpu = 0;

	for (; i < NR_CPUS; ++i) {
		ux_prefer_cpu[i] = -1;
	}

	ux_cpu = cpumask_weight(topology_core_cpumask(min_cpu));
	if (ux_cpu == 0) {
		ux_warn("failed to init ux cpu data\n");
		return;
	}

	for (i = 0; i < NR_CPUS && ux_cpu < NR_CPUS; ++i) {
		ux_prefer_cpu[i] = ux_cpu++;
	}
}

bool test_ux_task_cpu(int cpu) {
	return (cpu >= ux_prefer_cpu[0]);
}

void find_ux_task_cpu(struct task_struct *tsk, int *target_cpu) {
	int i = 0;
	int temp_cpu = 0;
	struct rq *rq = NULL;

	for (i = (NR_CPUS - 1); i >= 0; --i) {
		temp_cpu = ux_prefer_cpu[i];
		if (temp_cpu <= 0 || temp_cpu >= NR_CPUS)
			continue;

		rq = cpu_rq(temp_cpu);
		if (!rq)
			continue;

		if (rq->curr->prio <= MAX_RT_PRIO)
			continue;

		if (!test_task_ux(rq->curr) && cpu_online(temp_cpu) && !cpu_isolated(temp_cpu)
			&& cpumask_test_cpu(temp_cpu, &tsk->cpus_allowed)) {
			*target_cpu = temp_cpu;
			return;
		}
	}
	return;
}

/*
 * add for create proc node: proc/pid/task/pid/static_ux
*/
bool is_special_entry(struct dentry *dentry, const char* special_proc)
{
	const unsigned char *name;
	if (NULL == dentry || NULL == special_proc)
		return false;

	name = dentry->d_name.name;
	if (NULL != name && !strncmp(special_proc, name, 32))
		return true;
	else
		return false;
}

static int proc_static_ux_show(struct seq_file *m, void *v)
{
	struct inode *inode = m->private;
	struct task_struct *p;
	p = get_proc_task(inode);
	if (!p) {
		return -ESRCH;
	}
	task_lock(p);
	seq_printf(m, "%d\n", p->static_ux);
	task_unlock(p);
	put_task_struct(p);
	return 0;
}

static int proc_static_ux_open(struct inode* inode, struct file *filp)
{
	return single_open(filp, proc_static_ux_show, inode);
}

static ssize_t proc_static_ux_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct task_struct *task;
	char buffer[PROC_NUMBUF];
	int err, static_ux;

	memset(buffer, 0, sizeof(buffer));
	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;
	if (copy_from_user(buffer, buf, count)) {
		return -EFAULT;
	}

	err = kstrtoint(strstrip(buffer), 0, &static_ux);
	if(err) {
		return err;
	}
	task = get_proc_task(file_inode(file));
	if (!task) {
		return -ESRCH;
	}

	task->static_ux = static_ux;

	put_task_struct(task);
	return count;
}

static ssize_t proc_static_ux_read(struct file* file, char __user *buf,
							    size_t count, loff_t *ppos)
{
	char buffer[PROC_NUMBUF];
	struct task_struct *task = NULL;
	int static_ux = -1;
	size_t len = 0;
	task = get_proc_task(file_inode(file));
	if (!task) {
		return -ESRCH;
	}
	static_ux = task->static_ux;
	put_task_struct(task);
	len = snprintf(buffer, sizeof(buffer), "%d\n", static_ux);
	return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

const struct file_operations proc_static_ux_operations = {
	.open		= proc_static_ux_open,
	.write		= proc_static_ux_write,
	.read		= proc_static_ux_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
