#include "matlogger2/utils/mat_appender.h"
#include "matlogger2/matlogger2.h"

#include <algorithm>
#include <atomic>
#include <list>

#include "thread.h"

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

using namespace XBot::matlogger2;

namespace XBot  
{

struct MatAppender::Impl
{
    // weak pointers to all registered loggers
    std::list<MatLogger2::WeakPtr> _loggers;
    
    // mutex for protecting _loggers
    MutexType _loggers_mutex;
    
    // main function for the flusher thread
    void flush_thread_main();
    
    // call flush_available_data() on all alive loggers
    int  flush_available_data_all();
    
    // callback that notifies when a enough data is available
    void on_block_available(VariableBuffer::BufferInfo buf_info);
    
    // bytes available on the queue
    int _available_bytes;
    
    // pointer flusher thread
    std::unique_ptr<ThreadType> _flush_thread;
    
    // mutex and condition variable for flusher thread
    MutexType _cond_mutex;
    CondVarType _cond;
    
    // loggers use this flag to wake up the flusher thread
    std::atomic<bool> _flush_thread_wake_up;
    
    // flag specifying if the flusher thread should exit
    std::atomic<bool> _flush_thread_run;
    
    Impl();
    
};

MatAppender::Ptr MatAppender::MakeInstance()
{
    return Ptr(new MatAppender);
}

MatAppender::Impl::Impl():
    _available_bytes(0),
    _flush_thread_wake_up(false),
    _flush_thread_run(false)
{

}

    
MatAppender::Impl& MatAppender::impl()
{
    return *_impl;
}

void MatAppender::Impl::on_block_available(VariableBuffer::BufferInfo buf_info)
{
    /* This callback is invoked whenever a new block is pushed into the queue
     * on any registered logger
     */
    
    const int    NOTIFY_THRESHOLD_BYTES = 30e6;
    const double NOTIFY_THRESHOLD_SPACE_AVAILABLE = 0.5;
    
    // increase available bytes count
    _available_bytes += buf_info.new_available_bytes;
    
    // if enough new data is available, or the queue is getting full, notify 
    // the flusher thread
    if(_available_bytes > NOTIFY_THRESHOLD_BYTES || 
        buf_info.variable_free_space < NOTIFY_THRESHOLD_SPACE_AVAILABLE)
    {
        std::lock_guard<MutexType> lock(_cond_mutex);
        
        _available_bytes = 0;
        _flush_thread_wake_up = true; 
        _cond.notify_one(); 
        
    }
}

MatAppender::MatAppender()
{
    _impl = std::make_unique<Impl>();
}


bool MatAppender::add_logger(std::shared_ptr<MatLogger2> logger)
{
    // check for nullptr
    if(!logger)
    {
        fprintf(stderr, "error in %s: null pointer provided as argument\n", __PRETTY_FUNCTION__);
        return false;
    }
    
    // acquire exclusive access to registered loggers list
    std::lock_guard<MutexType> lock(impl()._loggers_mutex);
    
    // predicate that is used to tell if the provided logger is already registered
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
    
    // check if the provided logger is already registered
    auto it = std::find_if(impl()._loggers.begin(), 
                           impl()._loggers.end(), 
                           predicate);
    
    if(it != impl()._loggers.end())
    {
        fprintf(stderr, "error in %s: trying to add same logger twice\n", __PRETTY_FUNCTION__);
        return false;
    }
    
    namespace pl = std::placeholders;
    
    //!!! This is the main synchronization point between loggers and the flusher
    // thread !!! 
    // All loggers keep a weak pointer to the logger manager; the on_block_available()
    // callback is invoked only if the manager is alive; moreover, while the callback
    // is being invoked, the manager is kept alive by calling weak_ptr<>::lock()
    std::weak_ptr<MatAppender> self = shared_from_this();
    
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

int MatAppender::flush_available_data()
{
    return impl().flush_available_data_all();
}

void MatAppender::start_flush_thread()

{
    impl()._flush_thread_run = true;
    impl()._flush_thread.reset(new ThreadType(&MatAppender::Impl::flush_thread_main, 
                                               _impl.get()
                                              )
                              );
}

int MatAppender::Impl::flush_available_data_all()
{
    int bytes = 0;
    
    // acquire exclusive access to registered loggers list
    std::lock_guard<MutexType> lock(_loggers_mutex);
    
    // define function that processes a single registered logger, 
    // and marks it for removal if it is expired
    auto process_or_remove = [&bytes](auto& logger_weak)
    {
        // !!! this is the main synchronization point that prevents loggers 
        // to be destruced while their data is being flushed to disk !!!
        // We try to lock the current logger by creating a shared pointer
        MatLogger2::Ptr logger = logger_weak.lock();
        
        // If we managed to lock it, it'll be kept alive till the scope exit
        if(!logger)
        {
            #ifdef MATLOGGER2_VERBOSE
            printf("MatAppender: removing expired logger..\n");
            #endif
            return true; // removed expired logger
        }
        
        bytes += logger->flush_available_data();
        
        return false; // don't remove
    };
    
    // process all loggers, remove those that are expired
    _loggers.remove_if(process_or_remove);
    
    return bytes;
}


void MatAppender::Impl::flush_thread_main()
{
    uint64_t total_bytes = 0;
    double work_time_total = 0;
    double sleep_time_total = 0;
    
    // call flush_available_data() an all alive loggers, then wait for notifications
    while(_flush_thread_run)
    {
        
        int bytes = 0;
        double work_time = measure_sec([this, &bytes, &total_bytes](){
            bytes = flush_available_data_all();
            total_bytes += bytes;
        });
        
        #ifdef MATLOGGER2_VERBOSE
        printf("Worked for %.2f sec (%.1f MB flushed)..", 
               work_time, bytes*1e-6);
        printf("..average load is %.2f \n", 1.0/(1.0+sleep_time_total/work_time_total));
        #endif
        
        std::unique_lock<MutexType> lock(_cond_mutex);
        double sleep_time = measure_sec([this, &lock](){
            _cond.wait(lock, [this]{ return _flush_thread_wake_up.load(); });
        });
        
        // reset condition
        _flush_thread_wake_up = false;
        
        
        work_time_total += work_time;
        sleep_time_total += sleep_time;
        
    }
    
    #ifdef MATLOGGER2_VERBOSE
    printf("Flusher thread exiting.. Written %.1f MB\n", total_bytes*1e-6);
    #endif
}


MatAppender::~MatAppender()
{    
    if(!impl()._flush_thread)
    {
        return;
    }
    
    // force the flusher thread to exit
    {
        std::lock_guard<MutexType> lock(impl()._cond_mutex);
        impl()._flush_thread_run = false;
        impl()._flush_thread_wake_up = true;
        impl()._cond.notify_one();
    }
    
    // join with the flusher thread
    impl()._flush_thread->join();
    
}




}
