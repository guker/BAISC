#ifndef _AUTOBUF_H_
#define _AUTOBUF_H_

#include <map>
#include <vector>
#include <malloc.h>
#include <assert.h>


/* 
*                                      自动伸展的buffer类
*                          缺省使用栈空间，不够申请堆内存，可自动释放
*
*             --------------------------------------------------------------------------------
*             |                            |                   |              |              |
*             |                            |                   |              |              | high water
*             --------------------------------------------------------------------------------
*             <---------  data  ---------->                     <--  Guard -->
*             <------------            capacity     --------->
*
*
*/
/// 缺省
const unsigned int autobuf_default_size = 4096;

const unsigned int autobuf_default_alignment = 16;
/// 缺省的水位
const unsigned int autobuf_default_high = 2*1024*1024;
/// 检查是否越界的看守
const char autobuf_guard[] = "AutoBufferGuard";

template<unsigned int SIZE = autobuf_default_size, typename TYPE = char, unsigned int ALIGN = autobuf_default_alignment,unsigned int HIGH = autobuf_default_high>
class auto_buf
{
public:
    auto_buf (size_t size = 0,TYPE* data = 0)
        :buf_real_   (0)
        ,buf_size_   (0)
        ,buf_cap_    (0)
        ,high_water_ (0)
    {
        memset(buf_def_,0,sizeof(buf_def_));
        buf_real_  = buf_def_; /// 指向栈内存      
        buf_cap_   = sizeof(buf_def_)/sizeof(buf_def_[0]);
        high_water_ = autobuf_default_high; /// 警戒水位
        strcpy(buf_guard_ ,autobuf_guard);
        resize(size);
        if(NULL != data)
        {
            memcpy(buf_real_,data,size*sizeof(TYPE));
        }
    }

    ~auto_buf(void)
    {
        wipe();
    }

    auto_buf(const auto_buf & that )
    {
        this->set(that.data(),that.size());
    }

    auto_buf& operator=(const auto_buf& that)
    {
        this->set(that.data(),that.size());
        return (*this);
    }


    TYPE* data(void)
    {
        return buf_real_;
    }

    const TYPE* data(void) const
    {
        return (const TYPE*) buf_real_;
    }

    unsigned char* byte_data(void)
    {
        return (unsigned char*) data();
    }

    const char* str_data(void)
    {
        return (const char*) data();
    }

    short* short_data(void)
    {
        return (short*) data();
    }

    template<typename T>
    T* t_data(void)
    {
        return (T*) data();
    }

    size_t size(void) const
    {
        return buf_size_;
    }

    size_t capacity(void) const
    {
        return buf_cap_;
    }

    size_t size_bytes(void) const
    {
        return buf_size_*sizeof(TYPE);
    }

    size_t capacity_bytes(void) const
    {
        return buf_cap_*sizeof(TYPE);
    }

    bool empty(void) const
    {
        return (buf_size_ == 0);
    }

    size_t high_water(void) const
    {
        return high_water_;
    }

    size_t high_water(size_t hw)
    {
        size_t lhw = high_water_;
        if(hw > autobuf_default_size)
        {
            high_water_ = hw;
        }

        return lhw;
    }

    int  alloc(size_t cap)
    {
        resize(cap);
        return 0;
    }

    int free(bool wipe = false)
    {
        wipe? this->wipe():resize(0);
        return 0;
    }

    void reset(void)
    {
        resize(0);
    }

    void resize(size_t new_size)
    {
        realloc(new_size);
        buf_size_ = new_size;
    }

    void set(const TYPE* buf,size_t count)
    {
        if(count > buf_cap_)
        {
            resize(count);
        }
        memcpy(buf_real_,buf,count*sizeof(TYPE));
        buf_cap_ = count;
    }

    int  get(TYPE* buf, size_t& count) const
    {
        int ret = 0;
        memcpy(buf,buf_real_,std::min(count,buf_size_)*sizeof(TYPE));
        if(count < buf_size_)
            return -1;
        count = std::min(count, buf_size_);
        return ret;
    }

    /// 重新分配内存
    void realloc(size_t new_cap)
    {
        if(high_water_>0 && capacity_bytes() > high_water_ && new_cap == 0)
        {
            wipe();
        }
        if(new_cap > buf_cap_)
        {
            TYPE*  obuf = buf_real_;
            size_t new_size = new_cap*sizeof(TYPE)+ sizeof(autobuf_guard);

            buf_real_ = (TYPE*)new char[new_size];

            if(buf_real_)
            {
                memset(buf_real_,0,new_size);
                buf_cap_ = new_cap;
                /// 设置看守
                memcpy(buf_real_+new_cap,autobuf_guard,sizeof(autobuf_guard));

                if(buf_size_ > 0)
                {
                    memcpy(buf_real_,obuf,buf_size_*sizeof(TYPE));
                }
                if(obuf != buf_def_)
                {
                    delete [] obuf;
                }
            }
            else
            {
                assert(buf_real_);
            }
        }
    }

    void wipe()
    {
        /// 检查是否越界
        assert(memcmp(buf_real_+buf_cap_,autobuf_guard,sizeof autobuf_guard) == 0);
        if(buf_real_ != buf_def_)
        {
            delete [] buf_real_;
        }
        buf_real_ = buf_def_;
        buf_cap_ = sizeof(buf_def_)/sizeof(buf_def_[0]);
        buf_size_ = 0;
    }


    size_t append(const TYPE* data ,size_t count)
    {
        if((count + buf_size_) > buf_cap_)
        {
            realloc(count+buf_size_);
            buf_cap_ = count + buf_size_;
        }
        memcpy(this->data() + buf_size_, data, count*sizeof(TYPE));
        buf_size_ += count;

        return buf_size_;
    }

    size_t fill(const TYPE* dat,size_t start = 0, size_t end =0 )
    {
        if(end ==0 || end > buf_size_)
        {
            end = buf_size_;
        }

        std::fill(buf_real_+start,buf_real_+end,dat);
        return (end - start);
    }


    TYPE& at(size_t index)
    {
        assert(index < buf_size_);
        return buf_real_[index];
    }

    TYPE& operator[](size_t index)
    {
        return at[index];
    }
private:
    /// 缺省使用栈内存
    TYPE       buf_def_[SIZE];

    char       buf_guard_[sizeof autobuf_guard];

    TYPE*      buf_real_;
    /// buffer有效数据
    size_t     buf_size_;
    /// buffer分配容量
    size_t     buf_cap_;
    /// 最高水位，若尺寸大于此值，resize会释放之
    size_t     high_water_;

};

/*
*        虚锁
*   可以起到虚锁定作用
*   aquire 锁定
*   release 释放锁
*
*/

class mutex_null
{
public:
    int aquire()
    {
        return 0;
    }

    int release()
    {
        return 0;
    }
    int lock()
    {
        return 0;
    }

    int unlock()
    {
        return 0;
    }
};

///  域锁
template<typename MUTEX>
class scope_mutex
{
public:
    scope_mutex(MUTEX& lock)
        :lock_(&lock)
    {
        lock->aquire();
    }
    scope_mutex(MUTEX* lock)
        :lock_(lock)
    {
        if(lock_)
            lock_->aquire();
    }

    ~scope_mutex()
    {
        if(lock_)
            lock_->release();
    }
private:
    MUTEX*   lock_;

};

/*
*          自动内存管理器
*    可以管理多个auto_buf,起到内存池的作用
*
*/
template<typename ATBUF = auto_buf,typename MUTEX = mutex_null>
class auto_buf_mngr
{
public:
    auto_buf_mngr(void)
        :high_water_ (0)
        ,last_free_  (-1)
    {
    }

    auto_buf_mngr(size_t count,size_t cap)
        :high_water_  (0)
        ,last_free_   (-1)
    {
        cap_ = cap;
        high_water_ = cap*count;
        reserve(count,0);
    }

    void init(size_t count,size_t cap)
    {
        cap_ = cap;
        high_water_ = cap*count;
        /// 刚开始不是真分配内存
        reserve(count,0);
    }

    ~auto_buf_mngr(void)
    {
        clear();
    }
    /// 分配一个指定大小的缓冲区
    /// 分配策略：
    /// 
    ATBUF* alloc(size_t cap)
    {
        ATBUF* buf = 0;
        GUARD  mon(bufs_lock_);

        if(cap == 0)
            cap = cap_;
        if(last_free_ != -1)
        {
            autobuf_assist& as = auto_bufs_[last_free_];
            if(as->buffer()->capacity() >= cap)
            {
                buf = as->alloc(cap);
                last_free_ = -1;
            }

            size_t first_idle = -1;
            for(size_t i = 0; i< auto_bufs_.size();i++)
            {
                autobuf_assist& as = auto_bufs[i];
                if(!as.busy())
                {
                    if(first_idle == -1)
                    {
                        first_idle = i;
                    }
                    if(buf != 0 && first_idle == -1)
                    {
                        last_free = i;
                        break;
                    }
                    if(0 == buf && as->buffer()->capacity() >= cap)
                    {
                        buf = as.alloc(cap);
                        if(last_free_ != -1)
                        {
                            break;
                        }
                        else if( i < auto_bufs_.size()-1)
                        {
                            last_free_ = i+1;
                        }

                    }
                }
            }
            if(0 == buf && first_idle != 1)
            {
                buf = auto_bufs_[first_idle].alloc(cap);
            }
            if(0 == buf)
            {
                auto_bufs_.push_back(autobuf_assist(cap,true));
                buf = auto_bufs_.back().buffer();
                bufs_map_.insert(std::make_pair(buf,auto_bufs.size()-1));
            }
        }
        data_map_.insert(std::make_pair(buf.byte_data(),buf));
        return buf;
    }

    int  free(unsigned char* data)
    {
        GUARD  gd(bufs_lock_);
        typename data_map::iterator di = data_map_.find(data);
        if(di == data_map_.end())
        {
            return -1;        
        }

        ATBUF* ab = di->second;
        data_map_.erase(ab->byte_data());

        typename bufs_map::iterator pi = bufs_map_.find(ab);

        if(pi == bufs_map_.end())
        {
            return -1;
        }
        size_t frid = pi->second;
        assert(auto_bufs_[frid].buffer() == ab);

        auto_bufs_[frid].free();
        last_free_ = frid;

        if(high_water_ > 0)
        {
            size_t total_size = 0;
            for(size_t i = 0; i < auto_bufs_.size();i++)
            {
                total_size += auto_bufs_[i].buffer()->capacity();
            }
            if(total_size > high_water_)
            {
                ab->wipe();
            }
        }
        return 0;
    }

    int  free(ATBUF* ab)
    {
        GUARD gd(bufs_lock_);
        data_map_.erase(ab->byte_data());
        typename bufs_map::iterator pi = bufs_map_.find(ab);
        if(pi == bufs_map_.end())
        {
            return -1;
        }
        size_t frid = pi->second;
        assert(auto_bufs_[frid].buffer() == ab);

        auto_bufs_[frid].free();
        last_free_ = frid;

        if(high_water_ > 0)
        {
            size_t total_size = 0;
            for(size_t i = 0; i < auto_bufs_.size();i++)
            {
                total_size += auto_bufs_[i].buffer()->capacity();
            }
            if(total_size > high_water_)
            {
                ab->wipe();
            }
        }
        return 0;
        
    }

    int free(void)
    {
        GUARD gd(bufs_lock_);
        std::for_each(auto_bufs_.begin(),auto_bufs_.end(),std::mem_fun_ref(&ATBUF::free));
        return 0;
    }

    int clear(void)
    {
        GUARD gd(bufs_lock_);
        for(bufs_pi pi = auto_bufs_.begin();pi != auto_bufs_.end;pi++)
        {
            buf = pi->buffer();
            buf->wipe();
            delete buf;
        }
        auto_bufs_.clear();
        bufs_map_.clear();
        data_.map_.clear();

        return 0;
   
    }
    /// 分配n个auto_buf,并将其地址与索引位置对应起来
    int reserve(size_t count,size_t cap)
    {
        GUARD gd(bufs_lock_);
        auto_bufs_.reserve(count);
        for(size_t i = 0; i<count;i++)
        {
            auto_bufs_.push_back(autobuf_assist(cap,false));
            bufs_map_.insert(std::make_pair(auto_bufs_.back().buffer(),auto_bufs_.size()-1));
        }
        return 0;

    }
    /// 紧致内存
    int compact(void)
    {
        GUARD gd(bufs_lock_);
        bufs_pi pi = auto_bufs_.begin();
        for(;pi != auto_bufs_.size();pi++)
        {
            if(!pi->busy())
            {
                delete pi->buffer();
                delete *pi;
                pi = auto_bufs_.erase(pi);
            }
        }
        return 0;
    }

    size_t high_water(void) const
    {
        return high_water_;
    }

    size_t high_water(size_t hw)
    {
        size_t lhw = high_water_;
        if(hw > autobuf_default_size)
        {
            high_water_ = hw;
        }

        return lhw;
    }
    /// 支持单体访问
    static auto_buf_mngr& instance(void)
    {
        static auto_buf_mngr  ab_inst;
        return ab_inst;
    }
private:
    /// 缓冲区辅助
    struct autobuf_assist
    {
        autobuf_assist(size_t cap,bool alloc)
        {
            abuf_ = new ATBUF(cap);
            busy_ = alloc;
        }

        bool busy(void) const
        {
            return busy_;
        }

        void busy(bool bs)
        {
            busy_ = bs;
        }

        ATBUF* buffer(void)
        {
            return abuf_;
        }
         ATBUF* alloc(size_t cap)
         {
             if(busy_)
                 return NULL;
             assert(!busy_);
             busy_ = true;
             abuf_->alloc(cap);
             return abuf_;
         }

         void free(void)
         {
             busy_ = false;
             abuf_->resize(0);
         }

    private:
        ATBUF* abuf_;
        bool   busy_;
    };
    /// 内存块容器
    typedef std::vector<autobuf_assist> bufs_list;
    typedef typename bufs_list::iterator bufs_pi;

    typedef scope_mutex<MUTEX>  GUARD;
    /// 所有内存
    bufs_list   auto_bufs_;
    MUTEX       bufs_lock_;

    size_t      high_water_;
    size_t      cap_;
    /// 用于优化效率
    size_t      last_free_;
    typedef std::map<ATBUF*,size_t>  bufs_map;
    bufs_map    bufs_map_;
    typedef std::map<unsigned char*,ATBUF*> data_map;
    data_map    data_map_;
};

#endif
