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

  

cdev_del()

void cdev_del(struct cdev *p)
{
	cdev_unmap(p->dev, p->count);
	kobject_put(&p->kobj);
}





