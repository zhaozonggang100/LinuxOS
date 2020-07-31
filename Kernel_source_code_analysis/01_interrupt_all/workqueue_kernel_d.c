
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
			for_each_cpu_worker_pool(pool, cpu) {
				pool->flags &= ~POOL_DISASSOCIATED;
				BUG_ON(!create_worker(pool));//创建内核线程
		}
	}
			


}
	




