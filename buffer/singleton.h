#ifndef _SINGLETON_H_
#define _SINGLETON_H_

/// ֧�ֵ������
/// sgleton.instance()
template<class T>
class singleton
{
public:
    static T& instance(void)
    {
        static T  inst;  ///����������Ǿ�̬������ֻ�е���һ�ε���instance�����Ż�ִ�У�������������֤ȫ��ֻ��һ��ʵ��
        return inst;
    }
};

#endif