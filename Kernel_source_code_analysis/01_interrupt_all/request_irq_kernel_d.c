
	/*
	irq 中断号(硬件中断号)
	myirq_handler 中断回调
	IRQF_SHARED 中断类型
	devname 中断设备名字
	&mydev传入到回调的函数的参数
	可以使用键盘中断 来模拟 中断；
	如下操作：
	    键盘的中断号是：2
		键盘是输入设备/dev/inpurt/event1 
		注意一点：键盘按下释放 会触发两次
	*/
if(request_irq(irq,myirq_handler,IRQF_SHARED,devname,&mydev)!=0)
{
    printk("%s request IRQ:%d failed..\n",devname,irq);
    return -1;
}
	
request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
		const char *name, void *dev)
{
	//irq_handler_t thread_fn = NULL 不会创建内核线程
	return request_threaded_irq(irq, handler, NULL, flags, name, dev);
}

int request_threaded_irq(unsigned int irq, irq_handler_t handler,
 	 irq_handler_t thread_fn, unsigned long irqflags,
	  const char *devname, void *dev_id)
 {
        struct irqaction *action;
		struct irq_desc *desc;
		int retval;
	
		if (irq == IRQ_NOTCONNECTED)
			return -ENOTCONN;
	
		if (((irqflags & IRQF_SHARED) && !dev_id) ||
			(!(irqflags & IRQF_SHARED) && (irqflags & IRQF_COND_SUSPEND)) ||
			((irqflags & IRQF_NO_SUSPEND) && (irqflags & IRQF_COND_SUSPEND)))
			return -EINVAL;
	
		desc = irq_to_desc(irq);
		if (!desc)
			return -EINVAL;
	
		if (!irq_settings_can_request(desc) ||
			WARN_ON(irq_settings_is_per_cpu_devid(desc)))
			return -EINVAL;
		//如果没有上半部函数回调 
		if (!handler) {
			if (!thread_fn)
				return -EINVAL;
			//handle被替换为内核指定的函数
			handler = irq_default_primary_handler;
				static irqreturn_t irq_default_primary_handler(int irq, void *dev_id)
				{
					//返回值目的唤醒内核线程
					return IRQ_WAKE_THREAD;
				}
		}
		//irqaction 分配结构体
		action = kzalloc(sizeof(struct irqaction), GFP_KERNEL);

		action->handler = handler;
		action->thread_fn = thread_fn;
		action->flags = irqflags;
		action->name = devname;
		action->dev_id = dev_id;
	
		retval = irq_chip_pm_get(&desc->irq_data);
		
		chip_bus_lock(desc);
		
		retval = __setup_irq(irq, desc, action);
		
		chip_bus_sync_unlock(desc);
	
		if (retval) {
			irq_chip_pm_put(&desc->irq_data);
			kfree(action->secondary);
			kfree(action);
		}
	
		return retval;
}
	EXPORT_SYMBOL(request_threaded_irq);

static int __setup_irq(unsigned int irq, struct irq_desc *desc, struct irqaction *new)
{
	struct irqaction *old, **old_ptr;
	unsigned long flags, thread_mask = 0;
	int ret, nested, shared = 0;
	cpumask_var_t mask;

	if (!desc)
		return -EINVAL;

	if (desc->irq_data.chip == &no_irq_chip)
		return -ENOSYS;
	if (!try_module_get(desc->owner))
		return -ENODEV;

	new->irq = irq;

	/*
	 * If the trigger type is not specified by the caller,
	 * then use the default for this interrupt.
	 */
	if (!(new->flags & IRQF_TRIGGER_MASK))
	new->flags |= irqd_get_trigger_type(&desc->irq_data);
		
	
		nested = irq_settings_is_nested_thread(desc);

		//我提供了一个 内核回调函数
		if (new->thread_fn && !nested) {
			//创建一个内核线程
			ret = setup_irq_thread(new, irq, false);
			if (ret)
				goto out_mput;
			if (new->secondary) {
				ret = setup_irq_thread(new->secondary, irq, true);
				if (ret)
					goto out_thread;
			}
		}
			
		if (!alloc_cpumask_var(&mask, GFP_KERNEL)) {
			ret = -ENOMEM;
			goto out_thread;
		}
	

		if (desc->irq_data.chip->flags & IRQCHIP_ONESHOT_SAFE)
			new->flags &= ~IRQF_ONESHOT;
	
		/*
		 * The following block of code has to be executed atomically
		 */
		raw_spin_lock_irqsave(&desc->lock, flags);
		old_ptr = &desc->action;
		old = *old_ptr;
		if (old) {
	
		if (!((old->flags & new->flags) & IRQF_SHARED) ||
			((old->flags ^ new->flags) & IRQF_TRIGGER_MASK) ||
			((old->flags ^ new->flags) & IRQF_ONESHOT))
			goto mismatch;

		/* All handlers must agree on per-cpuness */
		if ((old->flags & IRQF_PERCPU) !=
			(new->flags & IRQF_PERCPU))
			goto mismatch;

		/* add new interrupt at end of irq queue */
		do {

			thread_mask |= old->thread_mask;
			old_ptr = &old->next;
			old = *old_ptr;
		} while (old);
		shared = 1;
		}

		if (new->flags & IRQF_ONESHOT) {
			/*
			 * Unlikely to have 32 resp 64 irqs sharing one line,
			 * but who knows.
			 */
			if (thread_mask == ~0UL) {
				ret = -EBUSY;
				goto out_mask;
			}

			new->thread_mask = 1 << ffz(thread_mask);
	
		} else if (new->handler == irq_default_primary_handler &&
			   !(desc->irq_data.chip->flags & IRQCHIP_ONESHOT_SAFE)) {

			pr_err("Threaded irq requested with handler=NULL and !ONESHOT for irq %d\n",
				   irq);
			ret = -EINVAL;
			goto out_mask;
		}
	
		if (!shared) {
			ret = irq_request_resources(desc);
			if (ret) {
				pr_err("Failed to request resources for %s (irq %d) on irqchip %s\n",
					   new->name, irq, desc->irq_data.chip->name);
				goto out_mask;
			}
	
			init_waitqueue_head(&desc->wait_for_threads);
	
			/* Setup the type (level, edge polarity) if configured: */
			if (new->flags & IRQF_TRIGGER_MASK) {
				ret = __irq_set_trigger(desc,
							new->flags & IRQF_TRIGGER_MASK);
			
			if (ret) {
				irq_release_resources(desc);
				goto out_mask;
			}
		}

		desc->istate &= ~(IRQS_AUTODETECT | IRQS_SPURIOUS_DISABLED | \
				  IRQS_ONESHOT | IRQS_WAITING);
		irqd_clear(&desc->irq_data, IRQD_IRQ_INPROGRESS);

		if (new->flags & IRQF_PERCPU) {
			irqd_set(&desc->irq_data, IRQD_PER_CPU);
			irq_settings_set_per_cpu(desc);
		}

		if (new->flags & IRQF_ONESHOT)
			desc->istate |= IRQS_ONESHOT;

		if (irq_settings_can_autoenable(desc))
			irq_startup(desc, true);
		else
			/* Undo nested disables: */
			desc->depth = 1;

		/* Exclude IRQ from balancing if requested */
		if (new->flags & IRQF_NOBALANCING) {
			irq_settings_set_no_balancing(desc);
			irqd_set(&desc->irq_data, IRQD_NO_BALANCING);
		}

		/* Set default affinity mask once everything is setup */
		setup_affinity(desc, mask);

	} else if (new->flags & IRQF_TRIGGER_MASK) {
		unsigned int nmsk = new->flags & IRQF_TRIGGER_MASK;
		unsigned int omsk = irqd_get_trigger_type(&desc->irq_data);

		if (nmsk != omsk)
			/* hope the handler works with current	trigger mode */
			pr_warn("irq %d uses trigger mode %u; requested %u\n",
				irq, omsk, nmsk);
	}
			
	*old_ptr = new;

	irq_pm_install_action(desc, new);

	/* Reset broken irq detection when installing new handler */
	desc->irq_count = 0;
	desc->irqs_unhandled = 0;

	if (shared && (desc->istate & IRQS_SPURIOUS_DISABLED)) {
		desc->istate &= ~IRQS_SPURIOUS_DISABLED;
		__enable_irq(desc);
	}

	raw_spin_unlock_irqrestore(&desc->lock, flags);
			

	if (new->thread)
		wake_up_process(new->thread);
	if (new->secondary)
		wake_up_process(new->secondary->thread);

	register_irq_proc(irq, desc);
	new->dir = NULL;
	register_handler_proc(irq, new);
	free_cpumask_var(mask);

	return 0;


}



static int	setup_irq_thread(struct irqaction *new, unsigned int irq, bool secondary)
{
	struct task_struct *t;
	struct sched_param param = {
		.sched_priority = MAX_USER_RT_PRIO/2,
	};

	if (!secondary) {
		//真正的创建内核线程函数
		//irq_thread 回调
		//irq/%d-%s 名字
		t = kthread_create(irq_thread, new, "irq/%d-%s", irq,
				   new->name);
	} else {
		t = kthread_create(irq_thread, new, "irq/%d-s-%s", irq,
				   new->name);
		param.sched_priority -= 1;
	}

	sched_setscheduler_nocheck(t, SCHED_FIFO, &param);

	get_task_struct(t);
	new->thread = t;//放入actoin结构体中

	set_bit(IRQTF_AFFINITY, &new->thread_flags);
	return 0;
}


static int irq_thread(void *data)
{
	struct callback_head on_exit_work;
	struct irqaction *action = data;
	struct irq_desc *desc = irq_to_desc(action->irq);
	irqreturn_t (*handler_fn)(struct irq_desc *desc,
			struct irqaction *action);

	if (force_irqthreads && test_bit(IRQTF_FORCED_THREAD,
					&action->thread_flags))
		handler_fn = irq_forced_thread_fn;
	else
		handler_fn = irq_thread_fn;

	init_task_work(&on_exit_work, irq_thread_dtor);
	task_work_add(current, &on_exit_work, false);

	irq_thread_check_affinity(desc, action);
	//死循环     等待中断事件的发生
	while (!irq_wait_for_interrupt(action)) {
		irqreturn_t action_ret;

		irq_thread_check_affinity(desc, action);
		//调用此函数执行 代码
		action_ret = handler_fn(desc, action);
	
		//唤醒等待中断的其他线程
		wake_threads_waitq(desc);
	}


	task_work_cancel(current, irq_thread_dtor);
	return 0;
}

static irqreturn_t irq_thread_fn(struct irq_desc *desc,
		struct irqaction *action)
{
	irqreturn_t ret;
	//调用自己注册的 thread_fn函数
	ret = action->thread_fn(action->irq, action->dev_id);
	irq_finalize_oneshot(desc, action);
	return ret;
}



