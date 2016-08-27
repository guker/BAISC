#include<stdio.h>
#include<iostream>
#include<stdlib.h>
#include<time.h>
#include"threadpool.h"
using namespace std;


using namespace thr;

/// 定义具体任务
class task_1 : public thr_svc
{
public:
    void svc()
    {
        printf("hello!\n");
    }
};

class task_2:public thr_svc
{
public:
    void svc()
    {
        printf("world！\n");
    }

};
int main()
{
    threadpool<thr_svc,thr_impl_os,thr_sem> threadpools;
    unsigned int nthread = 2;
    threadpools.start(nthread);
    for(int i =0; i < 100; i++)
    {
        srand((unsigned)time(NULL));
        int res = rand()%2;
        thr_svc* work;
        if(res)
        {
              work = new task_1;
        }
        else
        {
             work = new task_2; 
        }

        threadpools.apply_for(work);
    }
    while(1);

    return 0;
}

