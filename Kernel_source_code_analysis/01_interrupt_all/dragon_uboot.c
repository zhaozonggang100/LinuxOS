static int run_main_loop(void)
{
#ifdef CONFIG_SANDBOX
	sandbox_main_loop_init();
#endif
	/* main_loop() can return to retry autoboot, if so just run it again */
	//第一部分初始化完毕
	puts("zhaozonggang main!\n");
	for (;;)
		
		main_loop();
	return 0;
}

>>/cmd/bootmenu.c
static int abortboot_single_key(int bootdelay)
{
	int abort = 0;
	unsigned long ts;

	printf("Hit any key to stop autoboot: %2d ", bootdelay);
}


