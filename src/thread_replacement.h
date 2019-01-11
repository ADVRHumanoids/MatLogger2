#ifndef __XBOT_MATLOGGER2_UTIL_THREAD_REPLACEMENT_H__
#define __XBOT_MATLOGGER2_UTIL_THREAD_REPLACEMENT_H__

/* Define minimalistic C++-like thread, mutex and condition_variable
 * using POSIX */

#include <pthread.h>
#include <functional>
#include <mutex>

namespace matlogger2
{
    class thread
    {
        
    public:
        
        template <typename Callable, typename... Args>
        thread(Callable&& callable, Args&&... args):
            _main_func(std::bind(callable, args...))
        {
            create(SCHED_OTHER, 10);
        }
        
        template <typename Callable, typename... Args>
        thread(int sched_policy, int prio, Callable&& callable, Args&&... args):
            _main_func(std::bind(callable, args...))
        {
            create(sched_policy, prio);
        }
        
        void join()
        {
            int ret = pthread_join(_handle, nullptr);
            
            if(ret != 0)
            {
                throw std::runtime_error("error while joining thread");
            }
        }
        
    private:
        
        thread(const thread&) = delete;
        thread& operator=(const thread&) = delete;
        
        void create(int sched_policy, int prio)
        {
            pthread_attr_t attr;
            pthread_attr_init (&attr);
            pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
            pthread_attr_setschedpolicy (&attr, sched_policy);
            sched_param schedparam;
            schedparam.sched_priority = prio;
            pthread_attr_setschedparam(&attr, &schedparam);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            
            int ret = pthread_create(&_handle, &attr, &thread::thread_main, this);
            
            if(ret != 0)
            {
                throw std::runtime_error("unable to start thread");
            }
            
            pthread_attr_destroy(&attr);
        }
        
        static void * thread_main(void * arg)
        {
            ((thread *)arg)->_main_func();
            return nullptr;
        }
        
        pthread_t _handle;
        std::function<void(void)> _main_func;
        
    };
    
    class mutex 
    {           
        
    public:

        mutex()
        {
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
            int ret = pthread_mutex_init(&_handle, &attr);
            if(ret != 0){
                throw std::runtime_error("error initializing the mutex (" + std::to_string(ret) + ")");
            }
            
            pthread_mutexattr_destroy(&attr);
        }
        
        void lock()
        {
            int ret = pthread_mutex_lock(&_handle);
            if(ret != 0){
                throw std::runtime_error("error acquiring the mutex (" + std::to_string(ret) + ")");
            }
        }
        
        bool try_lock()
        {
            int ret = pthread_mutex_trylock(&_handle);
            if(ret == EBUSY){
                return false;
            }
            if(ret != 0){
                throw std::runtime_error("error acquiring the mutex (" + std::to_string(ret) + ")");
            }
            return true;
        }
        
        void unlock()
        {
            int ret = pthread_mutex_unlock(&_handle);
            if(ret != 0){
                throw std::runtime_error("error releasing the mutex (" + std::to_string(ret) + ")");
            }
        }
        
        pthread_mutex_t * get_native_handle()
        {
            return &_handle;
        }
        
        
        
    private:
        
        mutex(const mutex&) = delete;
        mutex(const mutex&&) = delete;
        mutex& operator=(const mutex&) = delete;
        mutex& operator=(const mutex&&) = delete;
        
        pthread_mutex_t _handle;
        
        
    };
    
    
    class condition_variable
    {
        
    public:
        
        condition_variable()
        {
            
            pthread_condattr_t attr;
            pthread_condattr_init(&attr);
            int ret_1 = pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
            if(0 != ret_1)
            {
                throw std::runtime_error("error in pthread_condattr_setpshared (" + std::to_string(ret_1) + ")");
            }
            
            int ret = pthread_cond_init(&_handle, &attr);
            if(ret != 0){
                throw std::runtime_error("error initializing condition_variable (" + std::to_string(ret) + ")");
            }
        }
        
        template <typename Predicate>
        void wait(std::unique_lock<mutex>& lock, const Predicate& pred)
        {
            pthread_mutex_t * mutex = lock.mutex()->get_native_handle();
            
            while(!pred())
            {
                printf("going to sleed..\n");
                int ret = pthread_cond_wait(&_handle, mutex);
                if(ret != 0){
                    throw std::runtime_error("error in pthread_cond_wait (" + std::to_string(ret) + ")");
                }
                printf("woken up..\n");
            }
        }
        
        void notify_one()
        {
            printf("waking up thread..\n");
            int ret = pthread_cond_signal(&_handle);
            if(ret != 0){
                throw std::runtime_error("error in pthread_cond_signal (" + std::to_string(ret) + ")");
            }
        }
        
    private:
        
        pthread_cond_t _handle;
        
        
    };
}

#endif
