init/mian.c
softirq_init();
	void __init softirq_init(void)
	{
		int cpu;
		//每个CPU都要创建一个 ksoftirqd的线程
		for_each_possible_cpu(cpu) {
			per_cpu(tasklet_vec, cpu).tail = &per_cpu(tasklet_vec, cpu).head;
			per_cpu(tasklet_hi_vec, cpu).tail = &per_cpu(tasklet_hi_vec, cpu).head;
		}
		/*
		注册中断 相当于绑定了TASKLET_SOFTIRQ的回调函数tasklet_action;
					   绑定了HI_SOFTIRQ的回调函数tasklet_hi_action
		*/
		>>kernel/softirq.c
		open_softirq(TASKLET_SOFTIRQ, tasklet_action); //俗称软中断下半部  TASKLET_SOFTIRQ=6
			void open_softirq(int nr, void (*action)(struct softirq_action *))
			{
				softirq_vec[nr].action = action;
			}
			//按照一般的逻辑来说 只是一个回调 没有死循环如何 唤醒 呢？
			tasklet_action//回调分析
			static __latent_entropy void tasklet_action(struct softirq_action *a)
			{
				struct tasklet_struct *list;

				local_irq_disable();//禁止本地中断传递
				list = __this_cpu_read(tasklet_vec.head);//得到当前处理器上的tasklet链表tasklet_vec或者tasklet_hi_vec
				__this_cpu_write(tasklet_vec.head, NULL);//将当前处理器上的该链表设置为NULL, 达到清空的效果。
				__this_cpu_write(tasklet_vec.tail, this_cpu_ptr(&tasklet_vec.head));
				local_irq_enable();//打开本地中断

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
			
		open_softirq(HI_SOFTIRQ, tasklet_hi_action);  //hi 最高优先级的软中断


		crash-log-注：softirq_vec属于全局变量
		    softirq_vec = $1 = 
			 {{
			    action = 0xffffffff99295ad0 
			  }, {
			    action = 0xffffffff99302ff0 // hi tasklet_action 回调地址
			  }, {
			    action = 0xffffffff99836a10
			  }, {
			    action = 0xffffffff99838cf0
			  }, {
			    action = 0xffffffff9954e7e0
			  }, {
			    action = 0xffffffff995a5b10
			  }, {
			    action = 0xffffffff99295bd0 //tasklet_action 回调函数
			  }, {
			    action = 0xffffffff992d0a60
			  }, {
			    action = 0x0
			  }, {
			    action = 0xffffffff992fb1f0
		  }}
	}



