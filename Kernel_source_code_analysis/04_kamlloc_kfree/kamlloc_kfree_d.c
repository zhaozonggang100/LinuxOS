>>>kmallc调用关系图
kmalloc()
	static __always_inline void *kmalloc(size_t size, gfp_t flags)
	{
		if (__builtin_constant_p(size)) {//检查size的合理性
			if (size > KMALLOC_MAX_CACHE_SIZE)       //KMALLOC_MAX_CACHE_SIZE = 8k 
				return kmalloc_large(size, flags);    //大块内存的申请 	    //kmalloc_large()->kmalloc_order_trace()->kmalloc_order()->alloc_pages()
					static __always_inline void *kmalloc_large(size_t size, gfp_t flags)
					{
						unsigned int order = get_order(size);
						return kmalloc_order_trace(size, flags, order);
							void *kmalloc_order_trace(size_t size, gfp_t flags, unsigned int order)
							{
								void *ret = kmalloc_order(size, flags, order);
									void *kmalloc_order(size_t size, gfp_t flags, unsigned int order)
									{
										void *ret;
										struct page *page;

										flags |= __GFP_COMP;
										page = alloc_pages(flags, order);//最最重要的函数 分配2^order <连续>的物理页面
										ret = page ? page_address(page) : NULL;
										kmemleak_alloc(ret, size, 1, flags);
										kasan_kmalloc_large(ret, size, flags);
										return ret;
									}
									EXPORT_SYMBOL(kmalloc_order);
								trace_kmalloc(_RET_IP_, ret, size, PAGE_SIZE << order, flags);
								return ret;
							}
							EXPORT_SYMBOL(kmalloc_order_trace);
					}
			
	#ifndef CONFIG_SLOB
			if (!(flags & GFP_DMA)) {
				int index = kmalloc_index(size); ////计算出相应index
				    static __always_inline int kmalloc_index(size_t size)
					{
						if (!size)
							return 0;

						if (size <= KMALLOC_MIN_SIZE)
							return KMALLOC_SHIFT_LOW;

						if (KMALLOC_MIN_SIZE <= 32 && size > 64 && size <= 96)
							return 1;
						if (KMALLOC_MIN_SIZE <= 64 && size > 128 && size <= 192)
							return 2;
						if (size <=          8) return 3;
						if (size <=         16) return 4;
						if (size <=         32) return 5;
						if (size <=         64) return 6;
						if (size <=        128) return 7;
						if (size <=        256) return 8;
						if (size <=        512) return 9;
						if (size <=       1024) return 10;
						if (size <=   2 * 1024) return 11;
						if (size <=   4 * 1024) return 12;
						if (size <=   8 * 1024) return 13;
						if (size <=  16 * 1024) return 14;
						if (size <=  32 * 1024) return 15;
						if (size <=  64 * 1024) return 16;
						if (size <= 128 * 1024) return 17;
						if (size <= 256 * 1024) return 18;
						if (size <= 512 * 1024) return 19;
						if (size <= 1024 * 1024) return 20;
						if (size <=  2 * 1024 * 1024) return 21;
						if (size <=  4 * 1024 * 1024) return 22;
						if (size <=  8 * 1024 * 1024) return 23;
						if (size <=  16 * 1024 * 1024) return 24;
						if (size <=  32 * 1024 * 1024) return 25;
						if (size <=  64 * 1024 * 1024) return 26;
						BUG();

						/* Will never be reached. Needed because the compiler may complain */
						return -1;
					}

				if (!index)
					return ZERO_SIZE_PTR;

				return kmem_cache_alloc_trace(kmalloc_caches[index],flags, size);
					void *kmem_cache_alloc_trace(struct kmem_cache *s, gfp_t gfpflags, size_t size)
					{
						void *ret = slab_alloc(s, gfpflags, _RET_IP_);
							static __always_inline void *slab_alloc(struct kmem_cache *s,gfp_t gfpflags, unsigned long addr)
							{
								return slab_alloc_node(s, gfpflags, NUMA_NO_NODE, addr);//从指定缓存中 获取一个对象
							}
						trace_kmalloc(_RET_IP_, ret, size, s->size, gfpflags);
						kasan_kmalloc(s, ret, size, gfpflags);
						return ret;
					}
					EXPORT_SYMBOL(kmem_cache_alloc_trace);
					
			}
	#endif
		}
		return __kmalloc(size, flags);
	}

>>kfree函数
kfree(const void * ptr)

kfree(ptr)


新添加一个函数解释下：
//从指定缓存中 回去一个对象
kmem_cache_alloc(struct kmem_cache * cachep, int flags)
	void *kmem_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
	{
		return slob_alloc_node(cachep, flags, NUMA_NO_NODE);
	}

//创建一个固定缓存
kmem_cache_create(const char * name, size_t size, size_t align, unsigned long flags, void(* ctor)(void *))



