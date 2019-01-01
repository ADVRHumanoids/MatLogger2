#include <matlogger2/utils/matlogger_manager.h>
#include <matlogger2/matlogger2.h>

#include <algorithm>
#include <thread>
#include <condition_variable>
#include <mutex>

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
    std::vector<MatLogger2::WeakPtr> _loggers;
    
    void flush_thread_main();
    int  flush_available_data_all();
    void on_block_available(int bytes, int n_free_blocks);
    void on_logger_finished(int logger_idx);
    void disconnect(int logger_idx);
    
    std::unique_ptr<std::thread> _flush_thread;
    std::mutex _cond_mutex;
    std::condition_variable _cond;
    bool _flush_thread_wake_up;
    int _available_bytes;
    std::atomic<bool> _flush_thread_run;
    
    Impl();
    
};

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

void MatLoggerManager::Impl::on_block_available(int bytes, int n_free_blocks)
{
    std::lock_guard<std::mutex> lock(_cond_mutex);
    _available_bytes += bytes;
    if(_available_bytes > 30e6 || n_free_blocks < VariableBuffer::NUM_BLOCKS / 2)
    {
        _flush_thread_wake_up = true;
        _cond.notify_one();
    }
}

void MatLoggerManager::Impl::on_logger_finished(int logger_idx)
{
    disconnect(logger_idx);
}

void MatLoggerManager::Impl::disconnect(int logger_idx)
{
    _loggers.at(logger_idx).reset();
    printf("Disconnected logger %d\n", logger_idx);
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
    
    std::weak_ptr<MatLoggerManager> self = shared_from_this();
    
    logger->set_on_data_available_callback(
        [this, self](int bytes, int n_free_blocks)
        {
            if(!self.expired())
            {
                impl().on_block_available(bytes, n_free_blocks);
            }
        }
                                        );
    
    int logger_idx = impl()._loggers.size();
    
    logger->set_on_stop_callback(std::bind(&Impl::on_logger_finished, _impl.get(), logger_idx));
    
    impl()._loggers.emplace_back(logger);
    
    return true;
}

void MatLoggerManager::start_thread()

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
        
        
        std::unique_lock<std::mutex> lock(_cond_mutex);
        _available_bytes -= bytes;
        double sleep_time = measure_sec([this, &lock](){
            _cond.wait(lock, [this]{ return _flush_thread_wake_up; });
        });
        
        _flush_thread_wake_up = false;
        
        
        work_time_total += work_time;
        sleep_time_total += sleep_time;
        
        printf("Worked for %.2f sec, slept for %.2f sec (%.1f MB flushed)\n", 
               work_time, sleep_time, bytes*1e-6);
        
        printf("Average load is %.2f \n", 1.0/(1.0+sleep_time_total/work_time_total));
        
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
