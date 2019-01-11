#include "thread_replacement.h"
#include <unistd.h>

bool condition = false;
matlogger2::mutex mutex;
matlogger2::condition_variable cond;

void rt_main()
{
    std::unique_lock<matlogger2::mutex> lock(mutex);
    
    printf("rt going to sleep\n");
    cond.wait(lock, [](){return condition;});
    printf("rt exit..\n");
}

void nrt_main()
{
    sleep(2);
    
    std::lock_guard<matlogger2::mutex> lock(mutex);
    condition = true;
    cond.notify_one();
}

int main()
{
    matlogger2::thread  rt(SCHED_FIFO , 60, &rt_main );
    matlogger2::thread nrt(SCHED_OTHER, 10, &nrt_main);
    
    rt.join();
    nrt.join();
}

