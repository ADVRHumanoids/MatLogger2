#ifndef __XBOT_MATLOGGER2_UTIL_THREAD_H__
#define __XBOT_MATLOGGER2_UTIL_THREAD_H__

/* Conditionally define mutex, condition_variable and thread classes based on
 * whether we're compiling for Xenomai3 or not */

#ifndef MATLOGGER2_USE_POSIX_THREAD

    /* Use threads from C++ library */

    #include <mutex>
    #include <condition_variable>
    #include <thread>

    namespace matlogger2 {
    
        typedef std::mutex MutexType;
        typedef std::condition_variable CondVarType;
        typedef std::thread ThreadType;
        
    }
    
#else

    #include "thread_replacement.h"
    
    namespace matlogger2 {
    
        typedef matlogger2::mutex MutexType;
        typedef matlogger2::condition_variable CondVarType;
        typedef matlogger2::thread ThreadType;
        
    }

#endif

#endif 
