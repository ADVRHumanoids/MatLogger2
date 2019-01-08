#include <matlogger2/utils/matlogger_manager.h>
#include <matlogger2/matlogger2.h>

#include <algorithm>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace
{
    template <typename Func>
    double measure_sec(Func f)
    {
        auto tic = std::chrono::high_resolution_clock::now();
        
        f();
        
        auto toc = std::chrono::high_resolution_clock::now();
        
        return std::chrono::duration_cast<std::chrono::nanoseconds>(toc-tic).count()*1e-9;
    }
}

namespace XBot  
{

struct MatLoggerManager::Impl
{
    // weak pointers to all registered loggers
    std::vector<MatLogger2::WeakPtr> _loggers;
    
    void flush_thread_main();
    int  flush_available_data_all();
    void on_block_available(VariableBuffer::BufferInfo buf_info);
    
    int _available_bytes;
    
    std::unique_ptr<std::thread> _flush_thread;
    std::mutex _cond_mutex;
    std::condition_variable _cond;
    std::atomic<bool> _flush_thread_wake_up;
    std::atomic<bool> _flush_thread_run;
    
    Impl();
    
};

MatLoggerManager::Ptr MatLoggerManager::MakeInstance()
{
    return Ptr(new MatLoggerManager);
}

MatLoggerManager::Impl::Impl():
    _flush_thread_wake_up(false),
    _available_bytes(0),
    _flush_thread_run(false)
{

}

    
MatLoggerManager::Impl& MatLoggerManager::impl()
{
    return *_impl;
}

void MatLoggerManager::Impl::on_block_available(VariableBuffer::BufferInfo buf_info)
{
    const int NOTIFY_THRESHOLD_BYTES = 30e6;
    const double NOTIFY_THRESHOLD_SPACE_AVAILABLE = 0.5;
    
    _available_bytes += buf_info.new_available_bytes;
    
    if(_available_bytes > NOTIFY_THRESHOLD_BYTES || 
        buf_info.variable_free_space < NOTIFY_THRESHOLD_SPACE_AVAILABLE)
    {
        _available_bytes = 0;
        _flush_thread_wake_up = true;
        _cond.notify_one();
    }
}

MatLoggerManager::MatLoggerManager()
{
    _impl = std::make_unique<Impl>();
}


bool MatLoggerManager::add_logger(std::shared_ptr<MatLogger2> logger)
{
    if(!logger)
    {
        fprintf(stderr, "Error in %s: null pointer provided as argument\n", __PRETTY_FUNCTION__);
        return false;
    }
    
    auto predicate = [logger](const auto& elem)
                     {
                         bool ret = false;
                         auto sp = elem.lock();
                         
                         if(sp && sp.get() == logger.get())
                         {
                             ret = true;
                         }
                         
                         return ret;
                         
                     };
    
    auto it = std::find_if(impl()._loggers.begin(), 
                           impl()._loggers.end(), 
                           predicate);
    
    if(it != impl()._loggers.end())
    {
        fprintf(stderr, "Error in %s: trying to add same logger twice\n", __PRETTY_FUNCTION__);
        return false;
    }
    
    namespace pl = std::placeholders;
    
    //!!! This is the main synchronization point between loggers and the flusher
    // thread !!! 
    // All loggers keep a weak pointer to the logger manager; the on_block_available()
    // callback is invoked only if the manager is alive; moreover, while the callback
    // is being invoked, the manager is kept alive by calling weak_ptr<>::lock()
    std::weak_ptr<MatLoggerManager> self = shared_from_this();
    
    // set on_block_available() as callback, taking care of possible death of
    // the manager while the callback is being processed
    logger->set_on_data_available_callback(
        [this, self](VariableBuffer::BufferInfo buf_info)
        {
            // lock weak-ptr by creating a shared pointer
            auto self_shared_ptr = self.lock();
            
            // if we managed to lock the manager, it'll be kept alive till
            // the current scope exit
            if(self_shared_ptr)
            {
                impl().on_block_available(buf_info);
            }
        }
    );
    
    // register the logger
    impl()._loggers.emplace_back(logger);
    
    return true;
}

void MatLoggerManager::start_flush_thread()

{
    impl()._flush_thread_run = true;
    impl()._flush_thread.reset(new std::thread(&MatLoggerManager::Impl::flush_thread_main, 
                                               _impl.get()
                                              )
                              );
}

int MatLoggerManager::Impl::flush_available_data_all()
{
    int bytes = 0;
    for(auto& logger_weak : _loggers)
    {
        MatLogger2::Ptr logger = logger_weak.lock();
        if(!logger)
        {
            continue;
        }
        bytes += logger->flush_available_data();
    }
    return bytes;
}


void MatLoggerManager::Impl::flush_thread_main()
{
    uint64_t total_bytes = 0;
    double work_time_total = 0;
    double sleep_time_total = 0;
    
    while(_flush_thread_run)
    {
        
        int bytes = 0;
        double work_time = measure_sec([this, &bytes, &total_bytes](){
            bytes = flush_available_data_all();
            total_bytes += bytes;
        });
        
        printf("Worked for %.2f sec (%.1f MB flushed)..", 
               work_time, bytes*1e-6);
        printf("..average load is %.2f \n", 1.0/(1.0+sleep_time_total/work_time_total));
        
        std::unique_lock<std::mutex> lock(_cond_mutex);
        double sleep_time = measure_sec([this, &lock](){
            _cond.wait(lock, [this]{ return _flush_thread_wake_up.load(); });
        });
        
        _flush_thread_wake_up = false;
        
        
        work_time_total += work_time;
        sleep_time_total += sleep_time;
        
    }
    
    printf("Flusher thread exiting.. Written %.1f MB\n", total_bytes*1e-6);
}


MatLoggerManager::~MatLoggerManager()
{
    printf("%s\n", __PRETTY_FUNCTION__);
    
    {
        std::lock_guard<std::mutex> lock(impl()._cond_mutex);
        impl()._flush_thread_run = false;
        impl()._flush_thread_wake_up = true;
        impl()._cond.notify_one();
    }
    
    impl()._flush_thread->join();
    
}




}
