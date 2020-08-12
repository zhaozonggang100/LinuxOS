
//等待队列结构体
struct __wait_queue_head {
	spinlock_t		lock;
	struct list_head	task_list;
};
typedef struct __wait_queue_head wait_queue_head_t;



//定义一个等待队列头 并初始化
#define DECLARE_WAIT_QUEUE_HEAD(name) \
	wait_queue_head_t name = __WAIT_QUEUE_HEAD_INITIALIZER(name)

#define __WAIT_QUEUE_HEAD_INITIALIZER(name) {				\
	.lock		= __SPIN_LOCK_UNLOCKED(name.lock),		\
	.task_list	= { &(name).task_list, &(name).task_list } }


//函数wait_event用于在某个线程中调用，当调用该函数时，如果参数中的条件不满足，
//则该线程会进入休眠状态。
//wait_event 本质上降进程设置为TASK_UNINTERRUPTIBLE，然后schedule()将进程切换出。
//为什么要设置一个不可中断的休眠状态
wait_event() 
#define wait_event(wq, condition)                    \
do {                                    \
    if (condition)                            \
        break;                            \
    __wait_event(wq, condition);                    \
} while (0)

#define __wait_event(wq, condition)                    \
    (void)___wait_event(wq, condition, TASK_UNINTERRUPTIBLE, 0, 0, schedule())


//用于对处于阻塞状态的线程进行唤醒，其参数就是队列头
//仅仅唤醒一个进程
wake_up()
#define wake_up(x)            __wake_up(x, TASK_NORMAL, 1, NULL)
__wake_up->__wake_up_common()->list_for_each_entry_safe()

注：个人见解
wake_up()//唤醒队列上的一个进程
signal_wake_up()//唤醒指定的进程




//例子
/*
说明：ts->t_can_run属于全局变量 当条件满足（ts->t_can_run =1）则执行
                                当条件满足（ts->t_can_run =0）挂起
*/
sevice.c
while (!kthread_should_stop()) {
		/*在这里等待事件， 线程被阻塞在这里。 
		wait_event()当条件不成立 把当前进程挂起(waiting)
		ts->t_can_run 不成立 进程挂起
		*/
		wait_event(wqh, ts->t_can_run || kthread_should_stop());
		ts->t_can_run = 0;
	}


client.c
while (!kthread_should_stop()) {
	index ++;
	if (index % 5 == 0) {
		ts->t_can_run = 1;
		wake_up(&wqh);
	}
}














#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
void call_sig(int sig)
{
	printf("sigid %d\n",sig);
}

 int main()
 {
	 signal(SIGCHLD ,call_sig);
	 pid_t pid = fork();
	 if(pid < 0)
	 {
		 exit(0);
	 }
	 else if(pid == 0)
	 {
		 printf("child %d\n",getpid());
		 setsid();//脱离终端
		 chdir("/");
		 //掩码
		 umask(0);
		 //关闭文件描述符
		 close(0);
		 close(1);
		 close(2);
		 //执行核心操作

		 //保证子进程运行
		 
		 while(1)
		 {

			 printf("JJJJ\n");
			 sleep(3);
		 }
		 
	 
	 }
	 else if(pid > 0)
	 {
		 while(1)
		 {
			printf("father %d  pid %d\n",getpid(),pid);
            sleep(3);			
		 }
	 }
	 
	 
	 return 0;
 }

》》》》》》》》》》》》新的见解》》》》》》》》》》》》》》》》》》》》

使用等待队列
  1: 创建一个工作队列头
  	wait_queue_head_t wqh;
		struct __wait_queue_head {
			spinlock_t		lock;
			struct list_head	task_list;
		};
		typedef struct __wait_queue_head wait_queue_head_t;

		
  2:初始化工作队列
  	//动态初始化
  	init_waitqueue_head(&wqh);
  		#define init_waitqueue_head(q)				\
		do {						\
			static struct lock_class_key __key;	\
								\
			__init_waitqueue_head((q), #q, &__key);	\
		} while (0)
		----------__init_waitqueue_head-------------------------------------------------------------------------------
  		void __init_waitqueue_head(wait_queue_head_t *q, const char *name, struct lock_class_key *key)
		{
			spin_lock_init(&q->lock);
			lockdep_set_class_and_name(&q->lock, key, name);
			INIT_LIST_HEAD(&q->task_list);//初始化链表
				------------------INIT_LIST_HEAD-------------------------
				static inline void INIT_LIST_HEAD(struct list_head *list)
				{
					list->next = list;
					list->prev = list;
				}
				------------------INIT_LIST_HEAD-------------------------
		}
		----------__init_waitqueue_head------------------------------------------------------------------------------
  3: 开始使用工作队列
    客户端每隔5s置一个变量，然后唤醒队列
  	client.c
		while (!kthread_should_stop()) {
			index ++;
			if (index % 5 == 0) {
				ts->t_can_run = 1;
				wake_up(&wqh);
					#define wake_up(x)			__wake_up(x, TASK_NORMAL, 1, NULL)
					-----__wake_up-----------------------------------------------------------
					void __wake_up(wait_queue_head_t *q, unsigned int mode,int nr_exclusive, void *key)
					{
						unsigned long flags;

						spin_lock_irqsave(&q->lock, flags);
						__wake_up_common(q, mode, nr_exclusive, 0, key);
						---------------__wake_up_common------------------------------------
							static void __wake_up_common(wait_queue_head_t *q, unsigned int mode,
									int nr_exclusive, int wake_flags, void *key)
							{
								wait_queue_t *curr, *next;

								/* 主要是这个循环，完成所有等待线程的唤醒， 这里关键是调用func */
								list_for_each_entry_safe(curr, next, &q->task_list, task_list) {
									unsigned flags = curr->flags;
									/* 这个函数是在init_wait_entry中初始化的，函数的名字是
                                     * autoremove_wake_function，主要完成线程唤醒的动作。 */
									if (curr->func(curr, mode, wake_flags, key) && (flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive)
										break;
								}
							}
						---------------__wake_up_common------------------------------------
						spin_unlock_irqrestore(&q->lock, flags);
					-----__wake_up-----------------------------------------------------------
					}
					EXPORT_SYMBOL(__wake_up);//导出符号表 供全局使用
			}
		}
		
	   sevice.c
		while (!kthread_should_stop()) {
				/*在这里等待事件， 线程被阻塞在这里。 
				wait_event()当条件不成立 把当前进程挂起(waiting)
				ts->t_can_run 不成立 进程挂起
				*/
				index ++;
				//wait_event 如果参数中的条件不满足，则该线程会进入休眠状态
				wait_event(wqh, ts->t_can_run || kthread_should_stop());
					--------------------------------------------------------------
					#define wait_event(wq, condition)					\
					do {									\
						might_sleep();							\
						if (condition)							\
							break;							\
						__wait_event(wq, condition);					\
							---------__wait_event------------------------------------------------------------------
							本质上就是将当前线程状态设置为TASK_UNINTERRUPTIBLE状态，然后调用schedule函数将本线程调度出去
							#define __wait_event(wq, condition) \
								(void)___wait_event(wq, condition, TASK_UNINTERRUPTIBLE, 0, 0, schedule())
									--------___wait_event--------------------------------------------------
									#define ___wait_event(wq, condition, state, exclusive, ret, cmd)	\
										({									\
											__label__ __out;						\
											wait_queue_t __wait; //定义一个节点实体			
											long __ret = ret;	/* explicit shadow */	
											
											 /* 这里初始化了 实体 第二个结构体，也就是等待队列项 */
											init_wait_entry(&__wait, exclusive ? WQ_FLAG_EXCLUSIVE : 0);
											for (;;) {	 
					
												/* 这个函数设置线程状态，并将等待队列项添加到等待队列中*/
												long __int = prepare_to_wait_event(&wq, &__wait, state);
													long prepare_to_wait_event(wait_queue_head_t *q, wait_queue_t *wait, int state)
													{
														unsigned long flags;
														long ret = 0;

														spin_lock_irqsave(&q->lock, flags);
														if (unlikely(signal_pending_state(state, current))) {
															list_del_init(&wait->task_list);
															ret = -ERESTARTSYS;
														} else {
															if (list_empty(&wait->task_list)) {
																if (wait->flags & WQ_FLAG_EXCLUSIVE)
																	__add_wait_queue_tail(q, wait);
																else
																	__add_wait_queue(q, wait);
															}
															set_current_state(state);
														}
														spin_unlock_irqrestore(&q->lock, flags);

														return ret;
													}
													EXPORT_SYMBOL(prepare_to_wait_event);
												 /* 满足条件的情况下退出等待 */    					 
												if (condition)				 
													break;				 
																	 
												if (___wait_is_interruptible(state) && __int) {		 
													__ret = __int;			 
													goto __out;				 
												}					 
															 
												cmd;				 
											}		
											 /*将状态重新设置为TASK_RUNNING，并将队列项移出 */
											finish_wait(&wq, &__wait);					\
										__out:	__ret;								\
										})
									--------___wait_event--------------------------------------------------
							---------__wait_event------------------------------------------------------------------
					} while (0)
					
				printk(KERN_NOTICE "server event over!\n");
				ts->t_can_run = 0;
		}














