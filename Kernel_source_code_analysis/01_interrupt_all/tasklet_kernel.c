
tasklet_schedule

>>include/linux/inerrupt.h
static inline void tasklet_schedule(struct tasklet_struct *t)
{
	//TASKLET_STATE_SCHED,	/* Tasklet is scheduled for execution */
	if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state))//设置TASKLET_STATE_SCHED
		__tasklet_schedule(t);
}


>>include/linux/inerrupt.h
void __tasklet_schedule(struct tasklet_struct *t)
{
	unsigned long flags;

	local_irq_save(flags);////保存IF标志的状态，并禁用本地中断
	t->next = NULL;              //下面的这两行代码就是为该tasklet分配per_cpu变量
	*__this_cpu_read(tasklet_vec.tail) = t;
	__this_cpu_write(tasklet_vec.tail, &(t->next));
	/*
	内核中有个ksoftirqd()的内核线程，它会周期的遍历软中断的向量列表，
	如果发现哪个软中断向量被挂起了（pending）,就执行相应的处理函数。
	*/
	raise_softirq_irqoff(TASKLET_SOFTIRQ);//触发软中断，让其在下一次do_softirq()的时候，有机会被执行
	local_irq_restore(flags);//恢复前面保存的标志
}

>>tasklet对应的处理函数就是tasklet_action,这个函数在系统启动初始化软中断时，
>>就在软中断向量表中注册。tasklet_action()遍历全局的tasklet_vec链表。

>>kernel/sofirq.c

tasklet_action()被执行是有ksoftirqd()的内核线程,周期的遍历软中断的向量列表
static __latent_entropy void tasklet_action(struct softirq_action *a)
{
	struct tasklet_struct *list;

	local_irq_disable();
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



