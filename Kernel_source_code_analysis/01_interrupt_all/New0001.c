
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
crw-r--r--   1 root root      1,  11 8月   7 08:54 kmsg
crw-rw-rw-   1 root root      1,   3 8月   7 08:54 null
crw-r-----   1 root kmem      1,   4 8月   7 08:54 port
crw-rw-rw-   1 root root      1,   8 8月   7 08:54 random
crw-rw-rw-   1 root root      1,   9 8月   7 08:54 urandom
crw-rw-rw-   1 root root      1,   5 8月   7 08:54 zero










