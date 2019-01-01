#ifndef __XBOT_MATLOGGER2_APPENDER_H__
#define __XBOT_MATLOGGER2_APPENDER_H__

#include <memory>

namespace XBot 
{
    class MatLogger2;
    
    class MatLoggerManager : public std::enable_shared_from_this<MatLoggerManager>
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
        
        std::function<void(void)> get_flush_thread_main() const;
        
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
