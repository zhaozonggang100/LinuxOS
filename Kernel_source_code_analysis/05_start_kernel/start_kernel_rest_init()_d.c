/*
说明：次函数主要是说明下，ksoftirqd 线程的建立过程
此函数主要作用是 创建1 2 进程，自己退化idle进程，1 2进程各自初始化自己的进程
*/
init/mian.c
rest_init();
	static noinline void __ref rest_init(void)
	{
		int pid;

		rcu_scheduler_starting();
		/*
		 * We need to spawn init first so that it obtains pid 1, however
		 * the init task will end up wanting to create kthreads, which, if
		 * we schedule it before we create kthreadd, will OOPS.
		 */
		kernel_thread(kernel_init, NULL, CLONE_FS);//创建1号进程（）
			//kernel_init属于回调函数1号进程的祖父
			static int __ref kernel_init(void *unused)
			{
				int ret;

				kernel_init_freeable();
					//
					static noinline void __init kernel_init_freeable(void)
					{
						/*
						 * Wait until kthreadd is all set-up.
						 */
						wait_for_completion(&kthreadd_done);// 等待kthreadd_done完成量，其实是在等待kthreadd进程创建完成
					
						/* Now the scheduler is fully set up and can do blocking allocations */
						gfp_allowed_mask = __GFP_BITS_MASK;
					
						/*
						 * init can allocate pages on any node
						 */
						set_mems_allowed(node_states[N_MEMORY]);// 设置init进程可以分配的物理页面
						/*
						 * init can run on any cpu.
						 */
						set_cpus_allowed_ptr(current, cpu_all_mask);// 通过设置cpu_bit_mask, 使init进程可以在任意cpu上运行
					
						cad_pid = task_pid(current);// cad：ctrl-alt-del 设置init进程来处理ctrl-alt-del信号
					
						smp_prepare_cpus(setup_max_cpus);// 对全部可用cpu核调用cpu_prepare函数，并将其设为present状态
					
						do_pre_smp_initcalls();// 调用level小于0的initcall函数
							static void __init do_pre_smp_initcalls(void)
							{
								initcall_t *fn;

								for (fn = __initcall_start; fn < __initcall0_start; fn++)
									do_one_initcall(*fn);
										int __init_or_module do_one_initcall(initcall_t fn)
										{
											int count = preempt_count();
											int ret;
											char msgbuf[64];
										
											if (initcall_blacklisted(fn))
												return -EPERM;
										
											if (initcall_debug)
												ret = do_one_initcall_debug(fn);
											else
												ret = fn();
										
											msgbuf[0] = 0;
										
											if (preempt_count() != count) {
												sprintf(msgbuf, "preemption imbalance ");
												preempt_count_set(count);
											}
											if (irqs_disabled()) {
												strlcat(msgbuf, "disabled interrupts ", sizeof(msgbuf));
												local_irq_enable();
											}
											WARN(msgbuf[0], "initcall %pF returned with %s\n", fn, msgbuf);
										
											add_latent_entropy();
											return ret;
										}
							}
						lockup_detector_init();// 使能watchdog
					
						smp_init();// 启动cpu0外的其他cpu核
						sched_init_smp();// 进程调度域初始化
					
						page_alloc_init_late();
					
						do_basic_setup();
					
						/* Open the /dev/console on the rootfs, this should never fail */
						if (sys_open((const char __user *) "/dev/console", O_RDWR, 0) < 0)
							pr_err("Warning: unable to open an initial console.\n");
					
						(void) sys_dup(0);// 标准输入
						(void) sys_dup(0);// 标准输出
						/*
						 * check if there is an early userspace init.  If yes, let it do all
						 * the work
						 */
					
						if (!ramdisk_execute_command)
							ramdisk_execute_command = "/init";
					
						if (sys_access((const char __user *) ramdisk_execute_command, 0) != 0) {
							ramdisk_execute_command = NULL;
							prepare_namespace();
						}
					
						/*
						 * Ok, we have completed the initial bootup, and
						 * we're essentially up and running. Get rid of the
						 * initmem segments and start the user-mode stuff..
						 *
						 * rootfs is available now, try loading the public keys
						 * and default modules
						 */
					
						integrity_load_keys();
						load_default_modules();// 加载IO调度的电梯算法
					}
					
				/* need to finish all async __init code before freeing the memory */
				async_synchronize_full();
				free_initmem();
				mark_readonly();
				system_state = SYSTEM_RUNNING;
				numa_default_policy();

				flush_delayed_fput();

				rcu_end_inkernel_boot();

				if (ramdisk_execute_command) {
					ret = run_init_process(ramdisk_execute_command);// do_execve(“/init”)  // 运行init程序，从一个内核进程变成用户进程
					if (!ret)
						return 0;
					pr_err("Failed to execute %s (error %d)\n",
					       ramdisk_execute_command, ret);
				}

				/*
				 * We try each of these until one succeeds.
				 *
				 * The Bourne shell can be used instead of init if we are
				 * trying to recover a really broken machine.
				 */
				if (execute_command) {
					ret = run_init_process(execute_command);
					if (!ret)
						return 0;
					panic("Requested init %s failed (error %d).",
					      execute_command, ret);
				}
				if (!try_to_run_init_process("/sbin/init") ||
				    !try_to_run_init_process("/etc/init") ||
				    !try_to_run_init_process("/bin/init") ||
				    !try_to_run_init_process("/bin/sh"))
					return 0;

				panic("No working init found.  Try passing init= option to kernel. "
				      "See Linux Documentation/init.txt for guidance.");
			}
			
		numa_default_policy();//设置默认访问策略
		
		pid = kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES);//创建2号进程
			//kthreadd 2号进程 自定义的方法创建 内核线程
			int kthreadd(void *unused)
			{
				struct task_struct *tsk = current;

				/* Setup a clean context for our children to inherit. */
				set_task_comm(tsk, "kthreadd");
				ignore_signals(tsk);
				set_cpus_allowed_ptr(tsk, cpu_all_mask);
				set_mems_allowed(node_states[N_MEMORY]);

				current->flags |= PF_NOFREEZE;
				cgroup_init_kthreadd();

				for (;;) {
					set_current_state(TASK_INTERRUPTIBLE);
					if (list_empty(&kthread_create_list))////如果队列空，睡眠
						schedule();
					__set_current_state(TASK_RUNNING);

					spin_lock(&kthread_create_lock);
					while (!list_empty(&kthread_create_list)) { //队列不为空，则对该队列进行循环，创建线程
						struct kthread_create_info *create;

						create = list_entry(kthread_create_list.next,struct kthread_create_info, list);//这个就是申请者传过来的结构

						list_del_init(&create->list);//先从队列上删除该create 
						spin_unlock(&kthread_create_lock);

						create_kthread(create);//为申请者创建线程

						spin_lock(&kthread_create_lock);
					}
					spin_unlock(&kthread_create_lock);
				}

				return 0;
			}
		rcu_read_lock();
		kthreadd_task = find_task_by_pid_ns(pid, &init_pid_ns);//获取kthreadd的进程描述符
		rcu_read_unlock();

		complete(&kthreadd_done);// 通知kernel_init进程kthreadd进程已创建完成

		/*
		 * The boot idle thread must execute schedule()
		 * at least once to get things moving:
		 */
		init_idle_bootup_task(current);// 设置当前进程（0号进程）为idle进程类
		schedule_preempt_disabled(); //主动调用进程调度，并禁止内核抢占
		/* Call into cpu_idle with preempt disabled */
		cpu_startup_entry(CPUHP_ONLINE);// 0号进程完成kernel初始化的工作，进入idle循环，化身idle进程
	}





>>暂时无法找到ksoftirqd线程是如何创建的
直接看ksoftirqd 创建的代码
kernel/softirq.c

static struct notifier_block cpu_nfb = {
    .notifier_call = cpu_callback
};

static struct smp_hotplug_thread softirq_threads = {
    .store            = &ksoftirqd,
    .thread_should_run    = ksoftirqd_should_run,
    .thread_fn        = run_ksoftirqd,
    .thread_comm        = "ksoftirqd/%u",
};

static __init int spawn_ksoftirqd(void)
{
    register_cpu_notifier(&cpu_nfb);

    BUG_ON(smpboot_register_percpu_thread(&softirq_threads));

    return 0;
}
early_initcall(spawn_ksoftirqd);


static int __smpboot_create_thread(struct smp_hotplug_thread *ht, unsigned int cpu)
{
...
    tsk = kthread_create_on_cpu(smpboot_thread_fn, td, cpu,
                    ht->thread_comm);
...
}


static int smpboot_thread_fn(void *data)//[ksoftirqd/0]死循环函数，等待被调度
{
    struct smpboot_thread_data *td = data;
    struct smp_hotplug_thread *ht = td->ht;

    while (1) {
        set_current_state(TASK_INTERRUPTIBLE);
		...
        if (!ht->thread_should_run(td->cpu)) {
            preempt_enable_no_resched();
            schedule();
        } else {
            __set_current_state(TASK_RUNNING);
            preempt_enable();
            ht->thread_fn(td->cpu);
        }
    }
}

## thread_should_run(td->cpu) = ksoftirqd_should_run ##
static int ksoftirqd_should_run(unsigned int cpu)
{
    return local_softirq_pending();//判断是否有pending(未处理的软中断)
}


## thread_fn = run_ksoftirqd ##
static void run_ksoftirqd(unsigned int cpu)
{
    local_irq_disable();
    if (local_softirq_pending()) {
        /*
         * We can safely run softirq on inline stack, as we are not deep
         * in the task stack here.
         */
        __do_softirq();//处理软中断（上文已经详细描述）
        local_irq_enable();
        cond_resched_rcu_qs();
        return;
    }
    local_irq_enable();
}

	
