#ifndef __XBOT_MATLOGGER2_APPENDER_H__
#define __XBOT_MATLOGGER2_APPENDER_H__

#include <memory>

namespace XBot 
{
    class MatLogger2;
    
    /**
    * @brief The MatLoggerManager class allows to flush data to disk from
    * multiple MatLogger2 loggers at once, both synchronously and from a 
    * separate flusher thread.
    * 
    * Construction:
    * use the factory method MakeInstance()
    * 
    * Usage:
    * register instances of MatLogger2 type with the add_logger() method.
    * Then, either call flush_available_data() in a loop, or 
call start_flush_thread()
    */
    class MatLoggerManager : public 
std::enable_shared_from_this<MatLoggerManager>
    {
        
    public:
        
        typedef std::weak_ptr<MatLoggerManager> WeakPtr;
        typedef std::shared_ptr<MatLoggerManager> Ptr;
        
        static Ptr MakeInstance();
        
        bool add_logger(std::shared_ptr<MatLogger2> logger);
        
        int flush_available_data();
        
        void start_flush_thread();
        
        ~MatLoggerManager();
        
        
    private:
        
        MatLoggerManager();
        
        struct Impl;
        
        Impl& impl();
        
        std::unique_ptr<Impl> _impl;
        
    };
}

#endif
