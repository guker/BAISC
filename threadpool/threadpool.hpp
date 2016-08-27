/*
*                      与平台无关的线程池管理类
*
*               信号量实现互斥锁，域锁，以及同步信号量（条件变量）
*     信号量的数据类型为结构体sem_t,本质上是长整型的数，并包含一链表，记录阻塞的进程
*     P原子语句：wait(s)
*               {
*                    while(s<=0); /// 阻塞
*                    s--;
*               }
*     V原子语句：signal(s)
*               {
*                    s++;        /// 通知阻塞进程继续执行
*               }
*
*   互斥锁，条件变量均可使用PV操作来实现，详见[操作系统原理]
*
*       /// int sem_init __P ((sem_t *__sem, int __pshared, unsigned int __value))
*       /// sem为指向信号量结构的一个指针
*       /// pshared不为0表示信号量在进程间共享，否则只能为当前进程的所有线程共享
*       /// value给出信号量的初始值
*       /// sem_wait函数作用是信号量减一，但它会先等待该信号量为一个非零值才会减1；
*       /// sem_post函数作用是信号量加一。
*       /// 这些函数执行成功的返回值都为0；
*/
#ifndef _THREAD_POOL_
#define _THREAD_POOL_

#define WIN32
#ifdef  WIN32
#       include <windows.h>
#       include <process.h>
#else
#       include<pthread.h>
#       include<semaphore.h>
#       include<unistd.h>
#       include<errno.h>
#       include<limits.h>

#endif

#include<vector>
#include<queue>
#include<stdio.h>


namespace thr
{

class lock_i
{
public:
    virtual bool  lock(void) = 0;
	virtual void  unlock(void) = 0;
};

/*
*    @class   mutex_lock 
*
*    @brief  互斥锁
*
*/
class mutex_lock
    : public lock_i
{
public:
    mutex_lock(void)
    {
#ifdef WIN32
        lock_ = CreateSemaphore(0, 1, 1, 0);
#else 
        lock_ = sem_init(&lock_, 0, 1);
#endif       
    }

    ~mutex_lock(void)
    {
#ifdef WIN32
        if( lock_)
        {
            CloseHandle(lock_);
            lock_ = 0;
        }
#else
        sem_destory(&lock);
#endif
    }

    void unlock(void)
    {
#ifdef WIN32
        ReleaseSemaphore(lock_, 1, 0);
#else
        sem_post(&lock_);
#endif
    }

    bool lock(void)
    {
#ifdef WIN32
        DWORD ret = WaitForSingleObject(lock_,INFINITE);
        return ret == WAIT_OBJECT_0;
#else
        return sem_wait(&lock_) == 0;
#endif
    }

private:
#ifdef WIN32
    HANDLE          lock_;
#else
    sem_t           lock_;   ///  整型信号量
#endif
};

/*
*    @class   scope_mutex 
*
*    @brief  域锁，构造时加锁，析构时解锁
*
*/
class scope_mutex
    :public lock_i
{
private:
    lock_i&  lock_; ///此处只是定义类型，引用类型可以不用初始化,实现多态

public:
    scope_mutex(lock_i& lock)
        :lock_(lock)
    {
        lock_.lock();
    }

    ~scope_mutex(void)
    {
        lock_.unlock();
    }

    bool lock(void)
    {
        return lock_.lock();
    }

    void unlock(void)
    {
        return lock_.unlock();
    }

};


/*
*    @class   thr_sem 
*
*    @brief  同步信号量
*    
*   lock里面有信号量，条件变量也有信号量，两者互不干扰，
*   因为是不同的信号量，无非为PV两个操作
*
*/

class thr_sem
{
public:
    thr_sem(int init_value = 0, int max_value = INT_MAX)
    {
#ifdef WIN32
        sem_ = CreateSemaphore(0, init_value, max_value, 0);
#else
        sem_init(&sem_,0,0);    /// 这里初值为0
#endif  
    }

    ~thr_sem(void)
    {
#ifdef WIN32
        if(sem_)
        {
            CloseHandle(sem_);
            sem_ = 0;
        }
#else
        sem_destroy(&sem_);
#endif
    }

    void notify_one(void)
    {
#ifdef WIN32
        ReleaseSemaphore(sem_,1,0);
#else
        sem_post(&sem_);
#endif
    }

    template <class LOCK_TYPE>
    bool timed_wait(LOCK_TYPE & lock,int msec)
    {
        lock.unlock();

        bool bret = false;
#ifdef WIN32
        DWORD ret = WaitForSingleObject(sem_,msec);
        bret = (ret == WAIT_OBJECT_0);
#else
        struct timeval tt;
        gettimeofday(&tt, NULL);
        timespec tv;
        tv.tv_sec = tt.tv.sec;
        tv.tv_nsec = tt.tv_usec*1000 + msec*1000*1000;
        tv.tv_sec += tt.tv_nsec/(1000*1000*1000);
        tv.tv_nsec %=(1000*1000*1000);
        bret = (sem_timedwatit(&sem_,&tv) == 0);
#endif
        lock.lock();
        return bret;
    }

    template<class LOCK_TYPE >
    bool wait(LOCK_TYPE& lock)
    {
        lock.unlock();
        bool bret = false;
#ifdef WIN32
        DWORD ret = WaitForSingleObject(sem_, INFINITE);
        bret = (ret == WAIT_OBJECT_0);
#else
        int ret = sem_wait(&sem_);
#endif
        lock.lock();
        return bret;
    }

    bool wait(void)
    {
        bool bret = false;
#ifdef WIN32
        DWORD ret = WaitForSingleObject(sem_,INFINITE);
        bret = (ret == WAIT_OBJECT_0);
#else
        int ret = sem_wait(&sem_);
        bret = (ret == 0);
#endif
        return bret;
    }


private:
#ifdef WIN32
    HANDLE          sem_;
#else
    sem_t           sem_;
#endif

};

class thr_event :public thr_sem
{
public:
    thr_event()
        :thr_sem(0,1)
    {}
};

/*
*    @class   thr_impl_i
*
*    @brief   线程抽象基类
*
*/
class thr_impl_i
{
public:
#ifdef WIN32
    typedef HANDLE   thr_id_t;
#else
    typedef pthread_t thr_id_t;
#endif

public:
    virtual int start(void)
    {
        thr_id_ = create_thread(this);
        return 0;
    }
    /// 执行体
    virtual void svc(void) = 0;

    virtual void stop()
    {
        join();
    }

    virtual int join(void)  /// 阻塞主线程，直到调用join的线程运行结束，使得main()函数会一直等待线程运行结束
    {
        return join_os(thr_id_);
    }

    virtual thr_id_t create_thread(thr_impl_i* thr_unit) = 0;
    virtual int join_os(thr_id_t) = 0;
private:
    thr_id_t  thr_id_;

};

/* window 线程实现 */
#ifdef WIN32
class thr_impl_win : public thr_impl_i
{
protected:
    virtual thr_id_t create_thread(thr_impl_i* thr_unit)
    {
        return (thr_id_t)_beginthreadex(NULL,0,thr_entry, thr_unit,0,NULL);
    }

    virtual int join_os(thr_id_t tid)
    {
        HANDLE hThread = (HANDLE)tid;
        return WaitForSingleObject(hThread,INFINITE);
    }

private:
    static unsigned int _stdcall thr_entry(void* args)
    {
        thr_impl_i* unit = (thr_impl_i*)args;
        unit->svc();
        return 0;
    }
};

typedef thr_impl_win   thr_impl_os;

/* linux 线程实现  */
#else
class thr_impl_nix : public thr_impl_i
{
protected:
    virtual thr_id_t create_thread(thr_impl_i* thr_unit)
    {
        pthread_t tid = 0;
        int ret = pthread_create(&tid,NULL,thr_entry,thr_unit);
        if(ret != 0)
            return errno;
        return tid;
    }

    virtual int join_os(thr_id_t tid)
    {
        int ret = pthread_join(tid,NULL);
        if(ret != 0)
            return errno;
        return ret;
    }

private:
    static void* thr_entry(void* args)
    {
        thr_impl_i* unit =(thr_impl_i*)args;
        unit->svc();
        return NULL;
    }
};
typedef thr_impl_nix thr_impl_os;
#endif

/**
*    @class  threadpool
*
*    @brief  线程池管理模板类
*    
*    线程池一般有4个组成部分
*    1.线程管理器，用于创建并管理线程池
*    2.工作线程：线程池中实际执行任务的线程，在初始化时会预先创建好固定数目的线程在池中
*               这些初始化的线程一般处于空闲状态，不占用CPU
                每个工作线程循环
*    3.任务接口：每个任务必须实现的接口，当线程池的任务队列中有可执行的任务时，被空闲的
*               工作线程调取执行，线程的闲与忙通过互斥量实现，把任务抽象出来形成接口，
*               可以做到线程池与具体的任务无关
*    4.任务队列：用来存放没有处理的任务，可以使用队列，也可以使用链表
*
*    @说明：thr_svc为任务接口，thr_impl_os为不同平台的线程，thr_sem为条件变量
*
*
*/

template <class thr_svc_t = thr_svc,class thr_impl = thr_impl_os, class sem_t = thr_sem>
class threadpool
{
public:
     enum
     {
         max_thr_count = 256
     };
public:
    enum thr_stat
    {
        thr_stat_idle = 0, // 空闲
        thr_stat_busy
    };

    ///  工作线程单元类
    /// 作用: 循环检测条件变量的通知，取任务
    template< class thrunit_svc_t = thr_svc>
    class thr_unit
        :public thr_impl_os
    {
    public:
        thr_unit(threadpool* thr_pool)
            :thr_pool_(thr_pool)
            ,cur_stat_(thr_stat_idle)
        {
            // 
        }
        thr_unit(const thr_unit & src_unit)
        {
            thr_pool_ = src_unit.thr_pool_;
            cur_stat_ = src_unit.cur_stat_;
        }
        thr_unit & operator=(const thr_unit & src_unit)
        {
            /// 此处直接访问私有变量，封装是编译期的概念，是针对类型而非对象的，
            /// 在类的成员函数中可以访问同类型实例对象的私有成员变量
            thr_pool_ = src_unit.thr_pool_; 
            cur_stat_ = src_unit.cur_stat_;
            return *this;
        }


        void set_cur_state(thr_stat  stat)
        {
            stat_mutex_.lock();
            cur_stat_ = stat;
            stat_mutex_.unlock();
        }

        bool is_idle(void)
        {
            return cur_stat_ == thr_stat_idle;
        }
    protected:
       
        virtual void svc(void)  
        {
            /// 不停地循环从任务队列中取任务并执行
            thrunit_svc_t* services;
            set_cur_state(thr_stat_idle);
            
            while(!thr_pool_->stoped())
            {
                {
                    scope_mutex auto_lock(thr_pool_->svc_que_lock_);

                    if(!thr_pool_->svc_que_sem_.timed_wait(auto_lock,200))
                        continue;
                    set_cur_state(thr_stat_busy);
                    services = thr_pool_->thr_svc_queue_.front();
                    thr_pool_->thr_svc_queue_.pop();
                }
                services->svc(); /// 执行函数
                delete services;
            }
        }

    private:
        threadpool* thr_pool_;
        thr_stat    cur_stat_;
        mutex_lock   stat_mutex_;
    };
    
    /// 工作线程
    typedef std::vector<thr_unit<thr_svc_t> > thr_unit_arr_t;
    thr_unit_arr_t thr_unit_arr_; 

    /// 任务队列
    typedef std::queue<thr_svc_t*> thr_svc_queue_t;
    thr_svc_queue_t thr_svc_queue_;
    

    bool thr_stop_;

public:
    threadpool()
        :thr_count_(0)
        ,thr_stop_(false)
    {
    }

    ~threadpool()
    {
    }

    /// 启动线程池
    void start(unsigned int thr_count)
    {
        thr_unit_arr_.reserve(max_thr_count);

        thr_count_ = thr_count > max_thr_count ? max_thr_count:thr_count;
        for(unsigned int i = 0; i < thr_count_; ++i)
       { 
           thr_unit_arr_.push_back(thr_unit<thr_svc_t>(this));
           thr_unit_arr_.back().start();/// 创建线程干活
       }
    }

    void stop(void)
    {
        if(thr_stop_)
            return;

        thr_stop_ = true;
        join_all();
        thr_unit_arr_.clear();

    }

  
    int apply_for(thr_svc_t* thr_ctx)
    {
        {
            /// 访问任务队列 ，上锁
            scope_mutex auto_lock(svc_que_lock_);
            thr_svc_queue_.push(thr_ctx);
            svc_que_sem_.notify_one();
        }
        return 0;
    }

protected:

    void add_new_thrunit(void)
    {
        thr_unit_arr_.push_back(thr_unit<thr_svc*>(this));
        thr_unit_arr_.back().start();
    }

    bool all_thr_busy(void)
    {
        for(typename thr_unit_arr_t::itertor it = thr_unit_arr_.begin();it != thr_unit_arr_.end();it++)
        {
            if(it->is_idle())
                return false;
        }
        return true;
    }
    void join_all(void)
    { 
        typename thr_unit_arr_t::iterator it = thr_unit_arr_.begin();
        for(; it != thr_unit_arr_.end();++it)
        {
            it->stop();
        }

    }

    bool stoped(void)
    {
        return thr_stop_;
    }

private:
    sem_t             svc_que_sem_;             /// 同步信号量
    mutex_lock        svc_que_lock_;            /// 队列互斥锁
    unsigned int      thr_count_;               /// 线程计数
};

/// 任务接口

class thr_svc
{
public:
    virtual void svc() = 0;
};

}

#endif
