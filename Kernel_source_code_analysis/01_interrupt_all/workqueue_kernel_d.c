
//工作队列使用说明
内核线程处理工作队列

1>>定义个数据结构
	static struct work_struct work; 

2>>初始化工作队列
	INIT_WORK(&work,work_handler);

3>>进行工作队列调度
	schedule_work(&work);//放入队列并唤醒线程
	>>源码讲解
static inline bool schedule_work(struct work_struct *work)
{
	//放入系统提供的默认队列中
	return queue_work(system_wq, work);
}

static inline bool queue_work(struct workqueue_struct *wq,
			      struct work_struct *work)
{
	return queue_work_on(WORK_CPU_UNBOUND, wq, work);
}

			  
bool queue_work_on(int cpu, struct workqueue_struct *wq,
		   struct work_struct *work)
{
	bool ret = false;
	unsigned long flags;

	local_irq_save(flags);//关中断

   //WORK_STRUCT_PENDING_BIT	= 0,	/* work item is pending execution */
	test_and_set_bit(int nr, long* addr)
	将*addr 的第n位设置成1，并返回原来这一位的值

	if (!test_and_set_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(work))) {
		__queue_work(cpu, wq, work);
		ret = true;
	}

	local_irq_restore(flags);//开中断
	return ret;
}
EXPORT_SYMBOL(queue_work_on);






struct work_struct {
	atomic_long_t data;
	struct list_head entry;
	work_func_t func;
#ifdef CONFIG_LOCKDEP
	struct lockdep_map lockdep_map;
#endif
};






//系统的工作队列如何创建的

是否可以这样理解 work_pool结构体 管理对应的线程
static int __init init_workqueues(void)
{
	/* initialize CPU pools */
			for_each_possible_cpu(cpu) {
				struct worker_pool *pool;
		
				i = 0;
				  /* 对每一个CPU都创建2个worker_pool结构体，它是含有ID的 */
                 /*  一个worker_pool对应普通优先级的work，第2个对应高优先级的work */
				for_each_cpu_worker_pool(pool, cpu) {
					BUG_ON(init_worker_pool(pool));
					pool->cpu = cpu;
					cpumask_copy(pool->attrs->cpumask, cpumask_of(cpu));
					pool->attrs->nice = std_nice[i++];
					pool->node = cpu_to_node(cpu);
		
					/* alloc pool ID */
					mutex_lock(&wq_pool_mutex);
					BUG_ON(worker_pool_assign_id(pool));
					mutex_unlock(&wq_pool_mutex);
				}
			}


			/* create the initial worker */
			/* 对每一个CPU的每一个worker_pool，创建一个worker */ 
			/* 每一个worker对应一个内核线程 */
			for_each_online_cpu(cpu) {
				struct worker_pool *pool;
				//对于每一个pool创建内核线程
			for_each_cpu_worker_pool(pool, cpu) {
				pool->flags &= ~POOL_DISASSOCIATED;
				BUG_ON(!create_worker(pool));//创建内核线程
				}
			}


	//workqueue_struct  *system_wq;	
	//system_wq = $1 = (struct workqueue_struct *) 0xffff8ddbff80d800
	/*
	alloc_workqueue() 申请一个workqueue_struct(system_wq)让系统使用
	    schedule_work(&work);-->>queue_work(system_wq, work);
	但是一个问题？worker_pool和workqueue_struct如何关联起来的？
	
	init_workqueues
	system_wq = alloc_workqueue("events", 0, 0);//次函数会做关联操作
		__alloc_workqueue_key
			wq = kzalloc(sizeof(*wq) + tbl_size, GFP_KERNEL);  // 分配workqueue_struct
			alloc_and_link_pwqs(wq) // 跟worker_poll建立联系
	*/
	system_wq = alloc_workqueue("events", 0, 0);
	system_highpri_wq = alloc_workqueue("events_highpri", WQ_HIGHPRI, 0);
	system_long_wq = alloc_workqueue("events_long", 0, 0);
	system_unbound_wq = alloc_workqueue("events_unbound", WQ_UNBOUND,
					    WQ_UNBOUND_MAX_ACTIVE);
	system_freezable_wq = alloc_workqueue("events_freezable",
					      WQ_FREEZABLE, 0);
	system_power_efficient_wq = alloc_workqueue("events_power_efficient",
					      WQ_POWER_EFFICIENT, 0);
	system_freezable_power_efficient_wq = alloc_workqueue("events_freezable_power_efficient",
					      WQ_FREEZABLE | WQ_POWER_EFFICIENT,
					      0);


  
}
	
static struct worker *create_worker(struct worker_pool *pool)
{
	struct worker *worker = NULL;
	int id = -1;
	char id_buf[16];

	/* ID is needed to determine kthread name */
	id = ida_simple_get(&pool->worker_ida, 0, 0, GFP_KERNEL);

	worker = alloc_worker(pool->node);

	worker->pool = pool;
	worker->id = id;

	if (pool->cpu >= 0)
		snprintf(id_buf, sizeof(id_buf), "%d:%d%s", pool->cpu, id,
			 pool->attrs->nice < 0  ? "H" : "");
	else
		snprintf(id_buf, sizeof(id_buf), "u%d:%d", pool->id, id);

	//创建的内核线程
	worker->task = kthread_create_on_node(worker_thread, worker, pool->node,
					      "kworker/%s", id_buf);


	set_user_nice(worker->task, pool->attrs->nice);
	kthread_bind_mask(worker->task, pool->attrs->cpumask);
	/* successful, attach the worker to the pool */
	worker_attach_to_pool(worker, pool);

	/* start the newly created worker */
	spin_lock_irq(&pool->lock);
	worker->pool->nr_workers++;
	worker_enter_idle(worker);
	wake_up_process(worker->task);
	spin_unlock_irq(&pool->lock);

	return worker;
	return NULL;
}


//内核线程死循环在worker_thread 等待work任务
static int worker_thread(void *__worker)
{
	struct worker *worker = __worker;
	struct worker_pool *pool = worker->pool;

	/* tell the scheduler that this is a workqueue worker */
	worker->task->flags |= PF_WQ_WORKER;
woke_up:
	spin_lock_irq(&pool->lock);

	/* am I supposed to die? */
	if (unlikely(worker->flags & WORKER_DIE)) {
		spin_unlock_irq(&pool->lock);
		WARN_ON_ONCE(!list_empty(&worker->entry));
		worker->task->flags &= ~PF_WQ_WORKER;

		set_task_comm(worker->task, "kworker/dying");
		ida_simple_remove(&pool->worker_ida, worker->id);
		worker_detach_from_pool(worker, pool);
		kfree(worker);
		return 0;
	}

	worker_leave_idle(worker);
recheck:
	/* no more worker necessary? */
	if (!need_more_worker(pool))
		goto sleep;

	/* do we need to manage? */
	if (unlikely(!may_start_working(pool)) && manage_workers(worker))
		goto recheck;

	/*
	 * ->scheduled list can only be filled while a worker is
	 * preparing to process a work or actually processing it.
	 * Make sure nobody diddled with it while I was sleeping.
	 */
	WARN_ON_ONCE(!list_empty(&worker->scheduled));

	/*
	 * Finish PREP stage.  We're guaranteed to have at least one idle
	 * worker or that someone else has already assumed the manager
	 * role.  This is where @worker starts participating in concurrency
	 * management if applicable and concurrency management is restored
	 * after being rebound.  See rebind_workers() for details.
	 */
	worker_clr_flags(worker, WORKER_PREP | WORKER_REBOUND);

	do {
		struct work_struct *work =
			list_first_entry(&pool->worklist,
					 struct work_struct, entry);

		pool->watchdog_ts = jiffies;

		if (likely(!(*work_data_bits(work) & WORK_STRUCT_LINKED))) {
			/* optimization path, not strictly necessary */
			process_one_work(worker, work);
			if (unlikely(!list_empty(&worker->scheduled)))
				process_scheduled_works(worker);
		} else {
			move_linked_works(work, &worker->scheduled, NULL);
			process_scheduled_works(worker);
		}
	} while (keep_working(pool));

	worker_set_flags(worker, WORKER_PREP);
sleep:
	/*
	 * pool->lock is held and there's no work to process and no need to
	 * manage, sleep.  Workers are woken up only while holding
	 * pool->lock or from local cpu, so setting the current state
	 * before releasing pool->lock is enough to prevent losing any
	 * event.
	 */
	worker_enter_idle(worker);
	__set_current_state(TASK_INTERRUPTIBLE);
	spin_unlock_irq(&pool->lock);
	schedule();
	goto woke_up;
}



