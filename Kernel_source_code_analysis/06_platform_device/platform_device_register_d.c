platform_device_register()
	int platform_device_register(struct platform_device *pdev)
	{
		device_initialize(&pdev->dev);
		arch_setup_pdev_archdata(pdev);
		return platform_device_add(pdev);
	}
	EXPORT_SYMBOL_GPL(platform_device_register);

