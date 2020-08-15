init/mian.c
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
init_IRQ()://硬件中断 根据体系不同 调用不同的中断函数 初始化函数（以ARM为例子讲解）
	// arch/arm/kernel/irq.c
	void __init init_IRQ(void)
	{
		int ret;

		if (IS_ENABLED(CONFIG_OF) && !machine_desc->init_irq)
			irqchip_init();
				// drives/irqchip.c
				void __init irqchip_init(void)
				{
					of_irq_init(__irqchip_of_table);
					acpi_probe_device_table(irqchip);
				}
		else
			machine_desc->init_irq();

		if (IS_ENABLED(CONFIG_OF) && IS_ENABLED(CONFIG_CACHE_L2X0) && (machine_desc->l2c_aux_mask || machine_desc->l2c_aux_val)) {
			if (!outer_cache.write_sec)
				outer_cache.write_sec = machine_desc->l2c_write_sec;
			ret = l2x0_of_init(machine_desc->l2c_aux_val,
					   machine_desc->l2c_aux_mask);
			if (ret && ret != -ENODEV)
				pr_err("L2C: failed to init: %d\n", ret);
		}

		uniphier_cache_init();
	}
		
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//以下是X86的初始化代码（/arch/x86/kernel/irqinit.c）
void __init init_IRQ(void)
{
	int i;

	/*
	 * On cpu 0, Assign ISA_IRQ_VECTOR(irq) to IRQ 0..15.
	 * If these IRQ's are handled by legacy interrupt-controllers like PIC,
	 * then this configuration will likely be static after the boot. If
	 * these IRQ's are handled by more mordern controllers like IO-APIC,
	 * then this vector space can be freed and re-used dynamically as the
	 * irq's migrate etc.
	 */
	for (i = 0; i < nr_legacy_irqs(); i++)
		per_cpu(vector_irq, 0)[ISA_IRQ_VECTOR(i)] = irq_to_desc(i);

	x86_init.irqs.intr_init();
}




