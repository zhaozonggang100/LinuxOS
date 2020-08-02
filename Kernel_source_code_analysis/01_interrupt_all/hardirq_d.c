>> /kernel/irq/handle.c
/*
关于ARM中断的一些描述
ARM共有7中模式：
    SVC
    IRQ
    FIQ
ARM寄存器CPSR：
    I位=1 用来表示 禁用 IRQ寄存器
    F位=1 用来表示 禁用 FIR寄存器
   如果IRQ=0表示可以相应中断->切换到IRQ模式->寻找相应的IRQ中断向量表
对于linux情况分析：
	键盘如何这中断联系起来？
    Linux不用FIQ，只用到了IRQ,当中断来临，有IRQ处理，之后交给SVC模式继续处理。
*/

irqreturn_t handle_irq_event_percpu(struct irq_desc *desc)
{
	irqreturn_t retval;
	unsigned int flags = 0;

	retval = __handle_irq_event_percpu(desc, &flags);

	add_interrupt_randomness(desc->irq_data.irq, flags);
}


irqreturn_t __handle_irq_event_percpu(struct irq_desc *desc, unsigned int *flags)
{
	irqreturn_t retval = IRQ_NONE;
	unsigned int irq = desc->irq_data.irq;
	struct irqaction *action;

	for_each_action_of_desc(desc, action) {
		irqreturn_t res;

		trace_irq_handler_entry(irq, action);
		//从上半部 获得返回值
		res = action->handler(irq, action->dev_id);
		trace_irq_handler_exit(irq, action, res);

		if (WARN_ONCE(!irqs_disabled(),"irq %u handler %pF enabled interrupts\n",
			      irq, action->handler))
			local_irq_disable();
		//判断返回值
		switch (res) {
		case IRQ_WAKE_THREAD://唤醒下半部 内核线程

			__irq_wake_thread(desc, action);

			/* Fall through to add to randomness */
		case IRQ_HANDLED:
			*flags |= action->flags;
			break;

		default:
			break;
		}

		retval |= res;
	}

	return retval;
}

void __irq_wake_thread(struct irq_desc *desc, struct irqaction *action)
{
	//唤醒 下半部进程
	wake_up_process(action->thread);
}



