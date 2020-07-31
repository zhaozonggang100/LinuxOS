
struct softirq_action
{
	void	(*action)(struct softirq_action *);
};

//NR_SOFTIRQS最大的软中断个数
static struct softirq_action softirq_vec[NR_SOFTIRQS] __cacheline_aligned_in_smp;

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
例子：
	enum
	{
		HI_SOFTIRQ=0,------------------------最高优先级的软中断类型
		TIMER_SOFTIRQ,-----------------------Timer定时器软中断
		NET_TX_SOFTIRQ,----------------------发送网络数据包软中断
		NET_RX_SOFTIRQ,----------------------接收网络数据包软中断
		BLOCK_SOFTIRQ,
		BLOCK_IOPOLL_SOFTIRQ,----------------块设备软中断
		TASKLET_SOFTIRQ,---------------------专门为tasklet机制准备的软中断
		SCHED_SOFTIRQ,-----------------------进程调度以及负载均衡软中断
		HRTIMER_SOFTIRQ,---------------------高精度定时器软中断
		RCU_SOFTIRQ,	/* Preferable RCU should always be the last softirq */----RCU服务软中断
	
		NR_SOFTIRQS
	};

//softirq_vec数组下标就是软中断号(入口函数地址)
	softirq_vec = $12 = 
 {{
    action = 0xffffffff94695ad0 //指向各自的回调函数
  }, {
    action = 0xffffffff94702ff0
  }, {
    action = 0xffffffff94c36a10
  }, {
    action = 0xffffffff94c38cf0
  }, {
    action = 0xffffffff9494e7e0
  }, {
    action = 0xffffffff949a5b10
  }, {
    action = 0xffffffff94695bd0
  }, {
    action = 0xffffffff946d0a60
  }, {
    action = 0x0
  }, {
    action = 0xffffffff946fb1f0
  }}

crash> p tasklet_action //此函数是tasklet的软中断入口
tasklet_action = $14 = 
 {void (struct softirq_action *)} 0xffffffff94695bd0

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

//注册一个软中断 open_softirq() nr是中断号 action 是回调
void open_softirq(int nr, void (*action)(struct softirq_action *))
{
	softirq_vec[nr].action = action;
}

//raise_softirq()主动触发一个软中断，nr中断号
void raise_softirq(unsigned int nr)
{
	unsigned long flags;

	local_irq_save(flags);//保存IF标志的状态，并禁用本地中断
	raise_softirq_irqoff(nr);
	local_irq_restore(flags);//恢复
}

inline void raise_softirq_irqoff(unsigned int nr)
{
	__raise_softirq_irqoff(nr);

	if (!in_interrupt())
		wakeup_softirqd();//如果不处于中断上下文中，则尽快执行软中断处理。
}

void __raise_softirq_irqoff(unsigned int nr)
{
	trace_softirq_raise(nr);
	or_softirq_pending(1UL << nr);//置位nr位的软中断，表示此软中断处于pending状态
}


>>引入重要的问题，软件中断在什么时机去 执行
//软中断执行时机
1> 硬件中断退出的时机 
irq_exit()-->>invoke_softirq()->wakeup_softirq()->唤醒ksoftirqd内核线程
void irq_exit(void)
{
#ifndef __ARCH_IRQ_EXIT_IRQS_DISABLED
	local_irq_disable();
#else
	WARN_ON_ONCE(!irqs_disabled());
#endif

	account_irq_exit_time(current);
	preempt_count_sub(HARDIRQ_OFFSET);
	//两个条件满足 是否处于中断上下文 是否有中断来临(有等待处理的软中断)
	if (!in_interrupt() && local_softirq_pending())
		invoke_softirq();

	tick_irq_exit();
	rcu_irq_exit();
	trace_hardirq_exit(); /* must be last! */
}

static inline void invoke_softirq(void)
{
	if (ksoftirqd_running())
		return;

	if (!force_irqthreads) {
#ifdef CONFIG_HAVE_IRQ_EXIT_ON_IRQ_STACK
		/*
		 * We can safely execute softirq on the current stack if
		 * it is the irq stack, because it should be near empty
		 * at this stage.
		 */
		__do_softirq(); //首先遍历执行处于pending状态的软中断函数；如果超出一定条件，将工作交给ksoftirqd处理
#else
		/*
		 * Otherwise, irq_exit() is called on the task stack that can
		 * be potentially deep already. So call softirq in its own stack
		 * to prevent from any overrun.
		 */
		do_softirq_own_stack();
#endif
	} else {
		wakeup_softirqd();
	}
}

asmlinkage __visible void __softirq_entry __do_softirq(void)
{
	unsigned long end = jiffies + MAX_SOFTIRQ_TIME;
	unsigned long old_flags = current->flags;
	int max_restart = MAX_SOFTIRQ_RESTART;
	struct softirq_action *h;
	bool in_hardirq;
	__u32 pending;
	int softirq_bit;

	/*
	 * Mask out PF_MEMALLOC s current task context is borrowed for the
	 * softirq. A softirq handled such as network RX might set PF_MEMALLOC
	 * again if the socket is related to swap
	 */
	current->flags &= ~PF_MEMALLOC;

	pending = local_softirq_pending();//检查是否有软中断 未处理
	account_irq_enter_time(current);

	__local_bh_disable_ip(_RET_IP_, SOFTIRQ_OFFSET);//增加preempt_count中的softirq域计数，表明当前在软中断上下文中
	in_hardirq = lockdep_softirq_start();

restart:
	/* Reset the pending bitmask before enabling irqs */
	set_softirq_pending(0);//软中断寄存器__softirq_pending 表明中断已经处理

	local_irq_enable();//打开本地中断

	h = softirq_vec; //ftirq_vec第一个元素，即软中断HI_SOFTIRQ对应的处理函数。
	//ffs()找到pending中第一个置位的比特位，返回值是第一个为1的位序号。这里的位是从低位开始，这也和优先级相吻合，低位优先得到执行。如果没有则返回0，退出循环。
	while ((softirq_bit = ffs(pending))) {
		unsigned int vec_nr;
		int prev_count;

		h += softirq_bit - 1;//根据sofrirq_bit找到对应的软中断描述符，即软中断处理函数

		vec_nr = h - softirq_vec;//软中断序号
		prev_count = preempt_count();

		kstat_incr_softirqs_this_cpu(vec_nr);

		trace_softirq_entry(vec_nr);
		h->action(h);         //执行对应软中断函数
		trace_softirq_exit(vec_nr);
		if (unlikely(prev_count != preempt_count())) {
			pr_err("huh, entered softirq %u %s %p with preempt_count %08x, exited with %08x?\n",
			       vec_nr, softirq_to_name[vec_nr], h->action,
			       prev_count, preempt_count());
			preempt_count_set(prev_count);
		}
		h++;//h递增，指向下一个软中断
		pending >>= softirq_bit;
	}

	rcu_bh_qs();
	local_irq_disable();//关闭本地中断

	pending = local_softirq_pending();//再次检查是否有软中断产生，在上一次检查至此这段时间有新软中断产生。
	if (pending) {
	//再次触发软中断执行的三个条件：
	//			1.软中断处理时间不超过2jiffies，200Hz的系统对应10ms；
	//			2.当前没有有进程需要调度，即!need_resched()；
	//			3.这种循环不超过10次。
		if (time_before(jiffies, end) && !need_resched() &&
		    --max_restart)
			goto restart;

		wakeup_softirqd();//如果上面的条件不满足，则唤醒ksoftirq内核线程来处理软中断
	}

	lockdep_softirq_end(in_hardirq);
	account_irq_exit_time(current);
	__local_bh_enable(SOFTIRQ_OFFSET);//减少preempt_count的softirq域计数,和前面增加计数呼应。表示这段代码处于软中断上下文。
	WARN_ON_ONCE(in_interrupt());
	tsk_restore_flags(current, old_flags, PF_MEMALLOC);
}


wakeup_softirqd()>>唤醒ksoftirqd内核线程

static void wakeup_softirqd(void)
{
	/* Interrupts are disabled: no need to stop preemption */
	struct task_struct *tsk = __this_cpu_read(ksoftirqd);

	//如果当前task不处于TASK_RUNNING，则去唤醒此进程
	if (tsk && tsk->state != TASK_RUNNING)
		wake_up_process(tsk);
}

2>

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
<*>ksoftirqd内核线程的创建
系统初始化就会 每个CPU创建一个ksoftirqd的内核进程
eg:
root          3  0.0  0.0      0     0 ?        S    09:36   0:00 [ksoftirqd/0]
root         16  0.0  0.0      0     0 ?        S    09:36   0:00 [ksoftirqd/1]

创建流程如下：

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




