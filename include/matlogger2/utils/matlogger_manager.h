#ifndef __XBOT_MATLOGGER2_APPENDER_H__
#define __XBOT_MATLOGGER2_APPENDER_H__

#include <memory>

namespace XBot 
{
    class MatLogger2;
    
    class MatLoggerManager : std::enable_shared_from_this<MatLoggerManager>
    {
        
    public:
        
        typedef std::weak_ptr<MatLoggerManager> WeakPtr;
        typedef std::shared_ptr<MatLoggerManager> Ptr;
        
        template <typename... Args>
        static Ptr MakeInstance(Args... args)
        {
            return Ptr(new MatLoggerManager(args...));
        }
        
        bool add_logger(std::shared_ptr<MatLogger2> logger);
        
        int flush_available_data();
        
        void start_thread();
        
        ~MatLoggerManager();
        
        
    private:
        
        MatLoggerManager();
        
        class Impl;
        
        Impl& impl();
        
        std::unique_ptr<Impl> _impl;
        
    };
}

#endif
