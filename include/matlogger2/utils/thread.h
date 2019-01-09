#ifndef __XBOT_MATLOGGER2_UTIL_THREAD_H__
#define __XBOT_MATLOGGER2_UTIL_THREAD_H__

/* Conditionally define mutex, condition_variable and thread classes based on
 * whether we're compiling for Xenomai3 or not */

#ifndef __COBALT__

    #include <mutex>
    #include <condition_variable>
    #include <thread>


    namespace matlogger2 {
    
        typedef std::mutex MutexType;
        typedef std::condition_variable CondVarType;
        typedef std::thread ThreadType;
        
    }

#else

    #error "Cobalt support under development"

#endif

#endif 
