uboot目的是启动内核
	1 读flash + 写flash 功能
	1 初始化时钟 关闭看门狗
	2 初始化SDram
	3 启动内核
<1> 分析配置过程     make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- vexpress_ca9x4_defconfig
<2> 配置流程         make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- menuconfig
<2> 分析编译         make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -j8

<**> arch/arm/cpu/u-boot.lds链接脚本 本身存在的文件

#include <config.h>
#include <asm/psci.h>

		OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
		OUTPUT_ARCH(arm)
		ENTRY(_start)//将_start这个符号设置成入口地址
		SECTIONS
		{
			}
<**> arch/arm/lib/vector.s _start执行的是这个*.S汇编文件

_start:
#ifdef CONFIG_SYS_DV_NOR_BOOT_CFG
	.word	CONFIG_SYS_DV_NOR_BOOT_CFG
#endif
	ARM_VECTORS
#endif /* !defined(CONFIG_ENABLE_ARM_SOC_BOOT0_HOOK) */

/*
 *************************************************************************
 *
 * Indirect vectors table
 *
 * Symbols referenced here must be defined somewhere else
 *
 *************************************************************************
 */

	.globl  _reset
	.globl	_undefined_instruction
	.globl	_software_interrupt
	.globl	_prefetch_abort
	.globl	_data_abort
	.globl	_not_used
	.globl	_irq
	.globl	_fiq




start_code:

    /*设置CPSR寄存器,让CPU进入管理模式*/
       mrs  r0, cpsr                 //读出cpsr的值
       bic   r0, r0, #0x1f           //清位
       orr   r0, r0, #0xd3          //位或
       msr  cpsr, r0                 //写入cpsr

#if   defined(CONFIG_AT91RM9200DK) || defined(CONFIG_AT91RM9200EK)
       /*
        * relocate exception table
        */
       ldr   r0, =_start            
       ldr   r1, =0x0                //r1等于异常向量基地址
       mov r2, #16
copyex:
       subs       r2, r2, #1           //减16次,s表示每次减都要更新条件标志位
       ldr   r3, [r0], #4       
       str   r3, [r1], #4      //将_start标号后的16个符号存到异常向量基地址0x0~0x3c处
       bne  copyex             //直到r2减为0
#endif

#ifdef CONFIG_S3C24X0

       /* 关看门狗*/
#  define pWTCON       0x53000000
#  define INTMSK 0x4A000008    /* Interrupt-Controller base addresses */
#  define INTSUBMSK  0x4A00001C
#  define CLKDIVN       0x4C000014    /* clock divisor register */

       ldr   r0, =pWTCON       
       mov r1, #0x0        
       str   r1, [r0]           //关看门狗,使WTCON寄存器=0

       /*关中断*/
       mov r1, #0xffffffff
       ldr   r0, =INTMSK
       str   r1, [r0]                  //关闭所有中断
# if defined(CONFIG_S3C2410)
       ldr   r1, =0x3ff
       ldr   r0, =INTSUBMSK
       str   r1, [r0]                  //关闭次级所有中断
# endif

    /* 设置时钟频率, FCLK:HCLK:PCLK = 1:2:4 ,而FCLK默认为120Mhz*/
       ldr   r0, =CLKDIVN
       mov r1, #3
       str   r1, [r0]

 #ifndef CONFIG_SKIP_LOWLEVEL_INIT
       bl    cpu_init_crit                         //关闭mmu,并初始化各个bank

#endif

call_board_init_f:
       ldr   sp, =(CONFIG_SYS_INIT_SP_ADDR) //CONFIG_SYS_INIT_SP_ADDR=0x30000f80
       bic   sp, sp, #7         //sp=0x30000f80
       ldr   r0,=0x00000000
       bl    board_init_f

<*> 最终跳到 board_init_f C语言函数 初始化定时器,GPIO,串口等,划分内存区域

<**> 最后执行 board_init_r 完成初始化  /common/board_r.c
    initcall_run_list(init_sequence_r)执行一系列后半部分的板级初始化函数
    run_main_loop() 
    main_loop()