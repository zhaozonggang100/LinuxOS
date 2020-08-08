https://www.cnblogs.com/oracleloyal/p/5359515.html

register_chrdev();

static inline int register_chrdev(unsigned int major, const char *name,
				  const struct file_operations *fops)
{   //register_chrdev(123,"Dragon", &tdd_fops)
	return __register_chrdev(major, 0, 256, name, fops);
}

int __register_chrdev(unsigned int major, unsigned int baseminor,
			unsigned int count, const char *name,
			const struct file_operations *fops)
{
 	 struct char_device_struct *cd;
 	 struct cdev *cdev;
 	 int err = -ENOMEM;
	//静态申请一个设备号
	cd = __register_chrdev_region(major, baseminor, count, name);//
 	 if (IS_ERR(cd))
		  return PTR_ERR(cd);


	//cdev_alloc() 动态申请一个cdev内存，并做了cdev_init中所做的前面3步初始化工作
  	cdev = cdev_alloc();//注册一个cdev结构体
 	 if (!cdev)
		  goto out2;

  	cdev->owner = fops->owner;
  	cdev->ops = fops;
 	 kobject_set_name(&cdev->kobj, "%s", name);
	 
/**
	 cdev_init( ) 建立cdev与 file_operations之间的连接，
	 cdev_add( ) 向系统添加一个cdev以完成注册;
**/
 	 err = cdev_add(cdev, MKDEV(cd->major, baseminor), count);
 	 if (err)
		  goto out;

  	cd->cdev = cdev;

  	return major ? 0 : cd->major;
out:
 	 kobject_put(&cdev->kobj);
out2:
 	 kfree(__unregister_chrdev_region(cd->major, baseminor, count));
 	 return err;
}


int cdev_add(struct cdev *p, dev_t dev, unsigned count)
{
	int error;

	p->dev = dev;
	p->count = count;
/*
kobj_map() 函数就是用来把字符设备编号和 cdev 结构变量一起保存到 cdev_map 这个散列表里。
当后续要打开一个字符设备文件时，通过调用 kobj_lookup() 函数，
根据设备编号就可以找到 cdev 结构变量，从而取出其中的 ops 字

*/
	error = kobj_map(cdev_map, dev, count, NULL,
			 exact_match, exact_lock, p);
	if (error)
		return error;

	kobject_get(p->kobj.parent);

	return 0;
}

  

cdev_del()

void cdev_del(struct cdev *p)
{
	cdev_unmap(p->dev, p->count);
	kobject_put(&p->kobj);
}





