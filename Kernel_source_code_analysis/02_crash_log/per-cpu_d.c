

crash> p __per_cpu_start 
PER-CPU DATA TYPE:
  [undetermined type] __per_cpu_start;
PER-CPU ADDRESSES:
  [0]: ffff97107c600000
  [1]: ffff97107c640000


ptr = alloc_bootmem(size * NR_CPUS);
for (i = 0; i < NR_CPUS; i++, ptr += size) {
	__per_cpu_offset[i] = ptr - __per_cpu_start;
	memcpy(ptr, __per_cpu_start, __per_cpu_end - __per_cpu_start);
}
//__per_cpu_start表示静态分配的percpu起始地址。即节区".data..percpu"中起始地址。


//而pcpu_base_addr表示整个系统中percpu的起始内存地址.(属于全局变量可以 使用函数计算出来)
crash> p pcpu_base_addr 
pcpu_base_addr = $2 = (void *) 0xffff97107c600000
/*

crash> p tasklet_vec 
PER-CPU DATA TYPE:
  struct tasklet_head tasklet_vec;
PER-CPU ADDRESSES:
  [0]: ffff9f25bc60de00
  [1]: ffff9f25bc64de00


*/
/* alrighty, percpu areas up and running */
delta = (unsigned long)pcpu_base_addr - (unsigned long)__per_cpu_start;


__per_cpu_offset[0] 第0个CPU的 per-cpu的位置（偏移位置对于pcpu_base_addr）

__per_cpu_offset = $1 = 
 {18446628695794778112, 18446628695795040256, 18446628695795302400, 18446628695795564544, 
 18446628695795826688, 18446628695796088832, 18446628695796350976, 18446628695796613120, 
 18446628695796875264, 18446628695797137408, 18446628695797399552, 18446628695797661696,
 ...
 }
 

static DEFINE_PER_CPU(long,cpuvar) = 10;





