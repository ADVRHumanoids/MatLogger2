#ifndef __XBOT_MATLOGGER2_APPENDER_H__
#define __XBOT_MATLOGGER2_APPENDER_H__

#include <memory>
#include "matlogger2/utils/visibility.h"

namespace XBot 
{
    class MatLogger2;
    
    /**
    * @brief The MatAppender class allows to flush data to disk from
    * multiple MatLogger2 loggers at once, both synchronously and from a 
    * separate flusher thread.
    * 
    * Construction:
    * use the factory method MakeInstance()
    * 
    * Usage:
    * register instances of MatLogger2 type with the add_logger() method.
    * Then, either call flush_available_data() in a loop, or call 
    * start_flush_thread().
    */
    class MATL2_API MatAppender : public std::enable_shared_from_this<MatAppender>
    {
        
    public:
        
        typedef std::weak_ptr<MatAppender> WeakPtr;
        typedef std::shared_ptr<MatAppender> Ptr;
        typedef std::unique_ptr<MatAppender> UniquePtr;
        
        /**
         * @brief Returns a shared pointer to a new MatAppender object
         */
        static Ptr MakeInstance();
        
        /**
         * @brief Register a MAT-logger to the appender. Note that
         * this class will internally store a **weak** pointer to the 
         * provided logger. This means that the logger may be destructed
         * any time without problems.
         * 
         * @return True if the logger is not null and it was not already registered.
         */
        bool add_logger(std::shared_ptr<MatLogger2> logger);
        
        /**
         * @brief Flush buffers from all registered loggers to disk. 
         * Caution: do NOT call this method if start_flush_thread() was called!
         * 
         * @return Amount of flushed bytes
         */
        int flush_available_data();
        
        /**
         * @brief Spawn a thread that will automatically flush data to disk whenever
         * enough data is available, or some buffer is about to fill.
         */
        void start_flush_thread();
        
        /**
         * @brief Destructor will join with the flusher thread if it was spawned
         * by the user.
         */
        ~MatAppender();

    private:

        
        MATL2_LOCAL MatAppender();
        
        struct MATL2_LOCAL Impl;
        
        MATL2_LOCAL Impl& impl();
        
        std::unique_ptr<Impl> _impl;
        
    };
}

#endif
