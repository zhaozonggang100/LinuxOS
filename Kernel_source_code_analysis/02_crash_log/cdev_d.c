//nihao
crash> p cdev_map
cdev_map = $3 = (struct kobj_map *) 0xffff8820bf81a800

crash> struct kobj_map 0xffff8820bf81a800
struct kobj_map {   
  probes = {0xffff8820bf92f300, 0xffff8820ba309800, 0xffff8820bf92f300, 
  	        0xffff8820bf92f300, 0xffff8820ab70d5c0, 0xffff8820ab70d480, 
  	        0xffff8820bf92f300, 0xffff8820ba309900, 0xffff8820bf92f300, 
  	        0xffff8820bf92f300, 0xffff8820ba7ff080, 0xffff8820bf92f300, 
  	        0xffff8820bf92f300, 0xffff8820b578e500, 0xffff8820bf92f300, 
  	        0xffff8820bf92f300, 0xffff8820bf92f300, 0xffff8820bf92f300, 
  	        ......
  	        0xffff8820b578e0c0, 0xffff8820bf92f300, 0xffff8820bf92f300, 
  	        0xffff8820bf92f300, 0xffff8820bf92f300, 0xffff8820bf92f300, 
  	        0xffff8820bf92f300, 0xffff8820bf92f300, 0xffff8820bbf79500, 
  	        0xffff8820bf92f300, 0xffff8820bf92f300, 0xffff8820bf92f300, 
  	        0xffff8820bf92f300, 0xffff8820bf92f300, 0xffff8820bf92f300, 
  	        0xffff8820bf92f300, 0xffff8820bf92f300, 0xffff8820bf92f300, 
  	        0xffff8820bf92f300, 0xffff8820bf92f300, 0xffff8820bf92f300, 
  	        0xffff8820bf92f300, 0xffff8820bf92f300, 0xffff8820bf92f300, 
  	        ......
  			}


crash> struct  probe  0xffff8820ba309800
struct probe {
  next = 0xffff8820bf92f300, 
  dev = 1048576, 
  range = 256, 
  owner = 0x0, 
  get = 0xffffffff9cc43140, 
  lock = 0xffffffff9cc432a0, 
  data = 0xffff8820bbfdc600
}


crash> struct cdev 0xffff8820bbfdc600
struct cdev {
  kobj = {
    name = 0xffffffff9d45104f "mem", 
    entry = {
      next = 0xffff8820bbfdc608, 
      prev = 0xffff8820bbfdc608
    }, 
    parent = 0x0, 
    kset = 0x0, 
    ktype = 0xffffffff9d6adc40, 
    sd = 0x0, 
    kref = {
      refcount = {
        counter = 110
      }
    }, 
    state_initialized = 1, 
    state_in_sysfs = 0, 
    state_add_uevent_sent = 0, 
    state_remove_uevent_sent = 0, 
    uevent_suppress = 0
  }, 
  owner = 0x0, 
  ops = 0xffffffff9d28e460, 
  list = {
    next = 0xffff8820ba330278, 
    prev = 0xffff8820ba331298
  }, 
  //其中dev_t是一个32位的数，12位表示主设备号，另外20位表示次设备号
  //所以主设备是1 次设备是0
  1048576 = 0001 0000 0000 0000 0000 0000 
  dev = 1048576, 
  count = 256
}

crw-r-----   1 root kmem      1,   1 8月   7 08:54 mem
crw-rw-rw-   1 root root      1,   7 8月   7 08:54 full
crw-rw-rw-   1 root root      1,   3 8月   7 08:54 null
crw-r-----   1 root kmem      1,   4 8月   7 08:54 port
crw-rw-rw-   1 root root      1,   8 8月   7 08:54 random
crw-rw-rw-   1 root root      1,   9 8月   7 08:54 urandom
crw-rw-rw-   1 root root      1,   5 8月   7 08:54 zero
crw-r--r--   1 root root      1,  11 8月   7 08:54 kmsg

>>>>>>>>>>>>>>>>>>>>>>>
以下是输入子系统的 log 参数
##############################################
crash> struct  probe  0xffff8820b578e500
struct probe {
  next = 0xffff8820b2651240, 
  dev = 13631557, 
  range = 1, 
  owner = 0x0, 
  get = 0xffffffff9cc43140, 
  lock = 0xffffffff9cc432a0, 
  data = 0xffff8820ac9c7388
}

crash> struct cdev 0xffff8820ac9c7388
struct cdev {
  kobj = {
    name = 0x0, 
    entry = {
      next = 0xffff8820ac9c7390, 
      prev = 0xffff8820ac9c7390
    }, 
    parent = 0xffff8820ac9c70c0, 
    kset = 0x0, 
    ktype = 0xffffffff9d6adc80, 
    sd = 0x0, 
    kref = {
      refcount = {
        counter = 1
      }
    }, 
    state_initialized = 1, 
    state_in_sysfs = 0, 
    state_add_uevent_sent = 0, 
    state_remove_uevent_sent = 0, 
    uevent_suppress = 0
  }, 
  owner = 0x0, 
  ops = 0xffffffff9d2aa1e0, 
  list = {
    next = 0xffff8820acf63ae8, 
    prev = 0xffff8820acf63ae8
  }, 
  //13 69
  dev = 13631557, 
  count = 1
}
#######################################################################
crash> struct probe 0xffff8820b2651240
struct probe {
  next = 0xffff8820b2651200, 
  dev = 13631556, 
  range = 1, 
  owner = 0x0, 
  get = 0xffffffff9cc43140, 
  lock = 0xffffffff9cc432a0, 
  data = 0xffff8820b36d6788
}

crash> struct cdev 0xffff8820b36d6788
struct cdev {
  kobj = {
    name = 0x0, 
    entry = {
      next = 0xffff8820b36d6790, 
      prev = 0xffff8820b36d6790
    }, 
    parent = 0xffff8820b36d64c0, 
    kset = 0x0, 
    ktype = 0xffffffff9d6adc80, 
    sd = 0x0, 
    kref = {
      refcount = {
        counter = 2
      }
    }, 
    state_initialized = 1, 
    state_in_sysfs = 0, 
    state_add_uevent_sent = 0, 
    state_remove_uevent_sent = 0, 
    uevent_suppress = 0
  }, 
  owner = 0x0, 
  ops = 0xffffffff9d2aa1e0, 
  list = {
    next = 0xffff8820b231c5a8, 
    prev = 0xffff8820b231c5a8
  }, 
  //13 68
  dev = 13631556, 
  count = 1
}
#################################################################
crash> struct probe 0xffff8820b2651200
struct probe {
  next = 0xffff8820ab70ddc0, 
  dev = 13631522, 
  range = 1, 
  owner = 0x0, 
  get = 0xffffffff9cc43140, 
  lock = 0xffffffff9cc432a0, 
  data = 0xffff8820b3fc5380
}

crash> struct cdev 0xffff8820b3fc5380
struct cdev {
  kobj = {
    name = 0x0, 
    entry = {
      next = 0xffff8820b3fc5388, 
      prev = 0xffff8820b3fc5388
    }, 
    parent = 0xffff8820b3fc50b8, 
    kset = 0x0, 
    ktype = 0xffffffff9d6adc80, 
    sd = 0x0, 
    kref = {
      refcount = {
        counter = 1
      }
    }, 
    state_initialized = 1, 
    state_in_sysfs = 0, 
    state_add_uevent_sent = 0, 
    state_remove_uevent_sent = 0, 
    uevent_suppress = 0
  }, 
  owner = 0x0, 
  ops = 0xffffffff9d2a9d60, 
  list = {
    next = 0xffff8820b3fc53d0, 
    prev = 0xffff8820b3fc53d0
  }, 
  //13 34
  dev = 13631522, 
  count = 1
}

crw-rw----  1 root input 13, 64 8月   7 08:54 event0
crw-rw----  1 root input 13, 65 8月   7 08:54 event1
crw-rw----  1 root input 13, 66 8月   7 08:54 event2
crw-rw----  1 root input 13, 67 8月   7 08:54 event3
crw-rw----  1 root input 13, 68 8月   7 08:54 event4
crw-rw----  1 root input 13, 69 8月   7 08:54 event5
crw-rw----  1 root input 13, 63 8月   7 08:54 mice
crw-rw----  1 root input 13, 32 8月   7 08:54 mouse0
crw-rw----  1 root input 13, 33 8月   7 08:54 mouse1
crw-rw----  1 root input 13, 34 8月   7 08:54 mouse2










