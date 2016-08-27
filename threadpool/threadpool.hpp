/*
*                      ��ƽ̨�޹ص��̳߳ع�����
*
*               �ź���ʵ�ֻ��������������Լ�ͬ���ź���������������
*     �ź�������������Ϊ�ṹ��sem_t,�������ǳ����͵�����������һ������¼�����Ľ���
*     Pԭ����䣺wait(s)
*               {
*                    while(s<=0); /// ����
*                    s--;
*               }
*     Vԭ����䣺signal(s)
*               {
*                    s++;        /// ֪ͨ�������̼���ִ��
*               }
*
*   ��������������������ʹ��PV������ʵ�֣����[����ϵͳԭ��]
*
*       /// int sem_init __P ((sem_t *__sem, int __pshared, unsigned int __value))
*       /// semΪָ���ź����ṹ��һ��ָ��
*       /// pshared��Ϊ0��ʾ�ź����ڽ��̼乲������ֻ��Ϊ��ǰ���̵������̹߳���
*       /// value�����ź����ĳ�ʼֵ
*       /// sem_wait�����������ź�����һ���������ȵȴ����ź���Ϊһ������ֵ�Ż��1��
*       /// sem_post�����������ź�����һ��
*       /// ��Щ����ִ�гɹ��ķ���ֵ��Ϊ0��
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
*    @brief  ������
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
    sem_t           lock_;   ///  �����ź���
#endif
};

/*
*    @class   scope_mutex 
*
*    @brief  ����������ʱ����������ʱ����
*
*/
class scope_mutex
    :public lock_i
{
private:
    lock_i&  lock_; ///�˴�ֻ�Ƕ������ͣ��������Ϳ��Բ��ó�ʼ��,ʵ�ֶ�̬

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
*    @brief  ͬ���ź���
*    
*   lock�������ź�������������Ҳ���ź��������߻������ţ�
*   ��Ϊ�ǲ�ͬ���ź������޷�ΪPV��������
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
        sem_init(&sem_,0,0);    /// �����ֵΪ0
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
*    @brief   �̳߳������
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
    /// ִ����
    virtual void svc(void) = 0;

    virtual void stop()
    {
        join();
    }

    virtual int join(void)  /// �������̣߳�ֱ������join���߳����н�����ʹ��main()������һֱ�ȴ��߳����н���
    {
        return join_os(thr_id_);
    }

    virtual thr_id_t create_thread(thr_impl_i* thr_unit) = 0;
    virtual int join_os(thr_id_t) = 0;
private:
    thr_id_t  thr_id_;

};

/* window �߳�ʵ�� */
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

/* linux �߳�ʵ��  */
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
*    @brief  �̳߳ع���ģ����
*    
*    �̳߳�һ����4����ɲ���
*    1.�̹߳����������ڴ����������̳߳�
*    2.�����̣߳��̳߳���ʵ��ִ��������̣߳��ڳ�ʼ��ʱ��Ԥ�ȴ����ù̶���Ŀ���߳��ڳ���
*               ��Щ��ʼ�����߳�һ�㴦�ڿ���״̬����ռ��CPU
                ÿ�������߳�ѭ��
*    3.����ӿڣ�ÿ���������ʵ�ֵĽӿڣ����̳߳ص�����������п�ִ�е�����ʱ�������е�
*               �����̵߳�ȡִ�У��̵߳�����æͨ��������ʵ�֣��������������γɽӿڣ�
*               ���������̳߳������������޹�
*    4.������У��������û�д�������񣬿���ʹ�ö��У�Ҳ����ʹ������
*
*    @˵����thr_svcΪ����ӿڣ�thr_impl_osΪ��ͬƽ̨���̣߳�thr_semΪ��������
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
        thr_stat_idle = 0, // ����
        thr_stat_busy
    };

    ///  �����̵߳�Ԫ��
    /// ����: ѭ���������������֪ͨ��ȡ����
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
            /// �˴�ֱ�ӷ���˽�б�������װ�Ǳ����ڵĸ����������Ͷ��Ƕ���ģ�
            /// ����ĳ�Ա�����п��Է���ͬ����ʵ�������˽�г�Ա����
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
            /// ��ͣ��ѭ�������������ȡ����ִ��
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
                services->svc(); /// ִ�к���
                delete services;
            }
        }

    private:
        threadpool* thr_pool_;
        thr_stat    cur_stat_;
        mutex_lock   stat_mutex_;
    };
    
    /// �����߳�
    typedef std::vector<thr_unit<thr_svc_t> > thr_unit_arr_t;
    thr_unit_arr_t thr_unit_arr_; 

    /// �������
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

    /// �����̳߳�
    void start(unsigned int thr_count)
    {
        thr_unit_arr_.reserve(max_thr_count);

        thr_count_ = thr_count > max_thr_count ? max_thr_count:thr_count;
        for(unsigned int i = 0; i < thr_count_; ++i)
       { 
           thr_unit_arr_.push_back(thr_unit<thr_svc_t>(this));
           thr_unit_arr_.back().start();/// �����̸߳ɻ�
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
            /// ����������� ������
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
    sem_t             svc_que_sem_;             /// ͬ���ź���
    mutex_lock        svc_que_lock_;            /// ���л�����
    unsigned int      thr_count_;               /// �̼߳���
};

/// ����ӿ�

class thr_svc
{
public:
    virtual void svc() = 0;
};

}

#endif
