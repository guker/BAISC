#ifndef _SINGLETON_H_
#define _SINGLETON_H_

/// 支持单体访问
/// sgleton.instance()
template<class T>
class singleton
{
public:
    static T& instance(void)
    {
        static T  inst;  ///这里申请的是静态变量，只有当第一次调用instance函数才会执行，否则跳过，保证全局只有一个实例
        return inst;
    }
};

#endif