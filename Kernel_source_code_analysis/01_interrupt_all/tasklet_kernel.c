//参考连接：https://www.cnblogs.com/arnoldlu/p/8659986.html#smpboot_thread_fn
tasklet_schedule

tasklet前沿描述：
结构体：
struct tasklet_struct
{
    struct tasklet_struct *next;------------------多个tasklet串成一个链表。
    unsigned long state;--------------------------TASKLET_STATE_SCHED表示tasklet已经被调度，正准备运行；TASKLET_STATE_RUN表示tasklet正在运行中。
    atomic_t count;-------------------------------0表示tasklet处于激活状态；非0表示该tasklet被禁止，不允许执行。
    void (*func)(unsigned long);------------------该tasklet处理程序
    unsigned long data;---------------------------传递给tasklet处理函数的参数
};


每个CPU维护着两个tasklet链表，
	tasklet_vec用于普通优先级，
	tasklet_hi_vec用于高优先级；它们都是per-CPU变量
	
struct tasklet_head {
    struct tasklet_struct *head;
    struct tasklet_struct **tail;
};
//tasklet_vec tasklet_hi_vec属于 per-CPU变量
static DEFINE_PER_CPU(struct tasklet_head, tasklet_vec);
static DEFINE_PER_CPU(struct tasklet_head, tasklet_hi_vec);
>>>>实战案例
	crash> p tasklet_hi_vec 
		PER-CPU DATA TYPE:
  		struct tasklet_head tasklet_hi_vec;
		PER-CPU ADDRESSES:
  		[0]: ffff8db63c60ddf0 //0号cpu
 		[1]: ffff8db63c64ddf0 //1号cpu

	crash> p tasklet_vec
		PER-CPU DATA TYPE:
  		struct tasklet_head tasklet_vec;
		PER-CPU ADDRESSES:
		[0]: ffff8db63c60de00 //0号cpu
  		[1]: ffff8db63c64de00 //1号cpu

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
tasklet初始化流程
asmlinkage __visible void __init start_kernel(void)
{
...
    softirq_init();
...
}

void __init softirq_init(void)
{
    int cpu;

    for_each_possible_cpu(cpu) {
        per_cpu(tasklet_vec, cpu).tail =&per_cpu(tasklet_vec, cpu).head;
        per_cpu(tasklet_hi_vec, cpu).tail =&per_cpu(tasklet_hi_vec, cpu).head;
    }
	/*
	注册中断 相当于绑定了TASKLET_SOFTIRQ的回调函数tasklet_action;
				   绑定了HI_SOFTIRQ的回调函数tasklet_hi_action
	*/
    open_softirq(TASKLET_SOFTIRQ, tasklet_action);
    open_softirq(HI_SOFTIRQ, tasklet_hi_action);
}

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

>>include/linux/inerrupt.h
static inline void tasklet_schedule(struct tasklet_struct *t)
{
	//TASKLET_STATE_SCHED,	/* Tasklet is scheduled for execution */
	//置TASKLET_STATE_SCHED位，如果原来未被置位，则调用__tasklet_schedule()。
	if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state))
		__tasklet_schedule(t);
} 


>>include/linux/inerrupt.h
void __tasklet_schedule(struct tasklet_struct *t)
{
	unsigned long flags;

	local_irq_save(flags);////保存IF标志的状态，并禁用本地中断
	t->next = NULL;              //下面的这两行代码就是为该tasklet分配per_cpu变量
	*__this_cpu_read(tasklet_vec.tail) = t;//将t挂入到tasklet_vec链表中
	__this_cpu_write(tasklet_vec.tail, &(t->next));
	/*
	内核中有个ksoftirqd()的内核线程，它会周期的遍历软中断的向量列表，
	如果发现哪个软中断向量被挂起了（pending）,就执行相应的处理函数。
	*/
	raise_softirq_irqoff(TASKLET_SOFTIRQ);//触发软中断(实际标记软中断位图)，让其在下一次do_softirq()的时候，有机会被执行
	local_irq_restore(flags);//恢复前面保存的标志
}

>>tasklet对应的处理函数就是tasklet_action,这个函数在系统启动初始化软中断时，
>>就在软中断向量表中注册。tasklet_action()遍历全局的tasklet_vec链表。

>>kernel/sofirq.c

tasklet_action()被执行是有ksoftirqd()的内核线程,周期的遍历软中断的向量列表

static __latent_entropy void tasklet_action(struct softirq_action *a)
{
	struct tasklet_struct *list;

	local_irq_disable();//禁止本地中断传递
	list = __this_cpu_read(tasklet_vec.head);//得到当前处理器上的tasklet链表tasklet_vec或者tasklet_hi_vec
	__this_cpu_write(tasklet_vec.head, NULL);//将当前处理器上的该链表设置为NULL, 达到清空的效果。
	__this_cpu_write(tasklet_vec.tail, this_cpu_ptr(&tasklet_vec.head));
	local_irq_enable();/允许响应中断

//循环遍历获得链表上的每一个待处理的tasklet
	while (list) {
		struct tasklet_struct *t = list;

		list = list->next;

		if (tasklet_trylock(t)) {
			if (!atomic_read(&t->count)) {
				if (!test_and_clear_bit(TASKLET_STATE_SCHED,
							&t->state))
					BUG();
				t->func(t->data);
				tasklet_unlock(t);
				continue;
			}
			tasklet_unlock(t);
		}

		local_irq_disable();
		t->next = NULL;
		*__this_cpu_read(tasklet_vec.tail) = t;
		__this_cpu_write(tasklet_vec.tail, &(t->next));
		__raise_softirq_irqoff(TASKLET_SOFTIRQ);
		local_irq_enable();
	}
}



