#include <matlogger2/matlogger2.h>
#include <signal.h>

volatile sig_atomic_t main_run = 1;

void signal_handler(int sig)
{
    main_run = 0;
}

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

int main()
{
    signal(SIGINT, signal_handler);
    
    XBot::MatLogger2 logger("mylog.mat");
    logger.flush_thread_start();
    
    std::vector<std::string> vars;
    const int VAR_SIZE = 25;
    for(int i = 0; i < 350; i++)
    {
        vars.emplace_back("my_var_" + std::to_string(i+1));
        logger.create(vars.back(), VAR_SIZE, 1);
    }
    
    logger.create("wr_time", 1, 1, 1e5);
    
    int i = 0; 
    uint64_t bytes = 0;
    uint64_t bytes_last = 0;
    double write_time = 0.0;
    Eigen::VectorXd data(VAR_SIZE);
    auto tic = std::chrono::high_resolution_clock::now();
    auto t_last = std::chrono::high_resolution_clock::now();
    while(main_run)
    {
        for(auto v : vars)
        {
            data.setConstant(data.size(), i);
            
            double time = measure_sec
            (
                [&]()
                {
                    logger.add(v, data);
                }
            );
            
            logger.add("wr_time", time);
            write_time += time;
            bytes += data.size() * sizeof(data[0]);
        }
        
        i++;
            
        if(i%1000 == 0)
        {
            auto now = std::chrono::high_resolution_clock::now();
            double sec = std::chrono::duration_cast<std::chrono::microseconds>(now-t_last).count()*1e-6;
            std::cout << "Writing " << (bytes-bytes_last)/sec*1e-6 << " MB/s" << std::endl;
            bytes_last = bytes;
            t_last = now;
        }
        

        usleep(1000);
    }
    auto toc = std::chrono::high_resolution_clock::now();
    int sec = std::chrono::duration_cast<std::chrono::seconds>(toc-tic).count();
    
    
    printf("Written %d MB in %d sec (write rata = %.1f GB/s, last data = %d)\n", 
           int(bytes*1e-6), sec, bytes/write_time*1e-9, i);
    
}

