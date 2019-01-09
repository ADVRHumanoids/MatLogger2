#ifndef __XBOT_MATLOGGER2_UTIL_THREAD_H__
#define __XBOT_MATLOGGER2_UTIL_THREAD_H__

/* Conditionally define mutex, condition_variable and thread classes based on
 * whether we're compiling for Xenomai3 or not */

    #include <mutex>
    #include <condition_variable>
    #include <thread>


    namespace matlogger2 {
    
        typedef std::mutex MutexType;
        typedef std::condition_variable CondVarType;
        typedef std::thread ThreadType;
        
    }

    #include <pthread.h>
    #include <functional>
    
    namespace matlogger2
    {
        class thread
        {
            
        public:
            
            template <typename Callable, typename... Args>
            thread(Callable&& callable, Args&&... args):
                _main_func(std::bind(callable, args...))
            {
                
                pthread_attr_t attr;
                pthread_attr_init (&attr);
                pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
                pthread_attr_setschedpolicy (&attr, SCHED_OTHER);
                sched_param schedparam;
                schedparam.sched_priority = 10;
                pthread_attr_setschedparam(&attr, &schedparam);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
                
                int ret = pthread_create(&_handle, &attr, &thread::thread_main, this);
                
                if(ret != 0)
                {
                    throw std::system_error("unable to start thread");
                }
                
                pthread_attr_destroy ( &attr );
                
            }
            
            void join()
            {
                int ret = pthread_join(&_handle);
                
                if(ret != 0)
                {
                    throw std::system_error("error while joining thread");
                }
            }
            
        private:
            
            thread(const thread&) = delete;
            thread& operator=(const thread&) = delete;
            
            static void * thread_main(void * arg)
            {
                ((thread *)arg)->_main_func();
            }
            
            pthread_t _handle;
            std::function<void*(void)> _main_func;
            
        };
        
        class mutex 
        {           
            
        public:

            mutex()
            {
                pthread_mutexattr_t attr;
                pthread_mutexattr_init(&attr);
                
                pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
                pthread_mutex_init(&_handle, &attr);
                
                pthread_mutexattr_destroy(&attr);
            }
            
            void lock()
            {
                int ret = pthread_mutex_lock(&_handle);
                if(ret != 0){
                    throw std::system_error("error acquiring the mutex (" + std::to_string(ret) + ")");
                }
            }
            
            bool try_lock()
            {
                int ret = pthread_mutex_trylock(&_handle);
                if(ret == EBUSY){
                    return false;
                }
                if(ret != 0){
                    throw std::system_error("error acquiring the mutex (" + std::to_string(ret) + ")");
                }
                return true;
            }
            
            void unlock()
            {
                int ret = pthread_mutex_unlock(&_handle);
                if(ret != 0){
                    throw std::system_error("error releasing the mutex (" + std::to_string(ret) + ")");
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
            
            condition_variable();
            
            template <typename Predicate>
            void wait(std::unique_lock<mutex>& lock, const Predicate& pred)
            {
                pthread_mutex_t * mutex = lock.mutex();
                
                while(!pred())
                {
                    
                }
            }
            
            void notify_one()
            {
                
            }
            
        private:
            
            
            
        };
    }


#endif 
