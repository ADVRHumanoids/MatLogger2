#include "thread_replacement.h"
#include <unistd.h>

bool condition = false;
XBot::matlogger2::mutex mutex;
XBot::matlogger2::condition_variable cond;

void rt_main()
{
    std::unique_lock<XBot::matlogger2::mutex> lock(mutex);
    
    printf("rt going to sleep\n");
    cond.wait(lock, [](){return condition;});
    printf("rt exit..\n");
}

void nrt_main()
{
    sleep(2);
    
    std::lock_guard<XBot::matlogger2::mutex> lock(mutex);
    condition = true;
    cond.notify_one();
}

int main()
{
    XBot::matlogger2::thread  rt(SCHED_FIFO , 60, &rt_main );
    XBot::matlogger2::thread nrt(SCHED_OTHER, 10, &nrt_main);
    
    rt.join();
    nrt.join();
}

