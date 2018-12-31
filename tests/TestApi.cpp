#include <gtest/gtest.h>
#include <matlogger2/matlogger2.h>
#include <matlogger2/utils/matlogger_manager.h>
#include <signal.h>

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

class TestApi: public ::testing::Test {
    

protected:

     TestApi(){
         
     }

     virtual ~TestApi() {
     }

     virtual void SetUp() {
         
     }

     virtual void TearDown() {
     }
     
     
};

volatile sig_atomic_t run_ok = 1;

void sig_handler(int sig)
{
    run_ok = 0;
}

TEST_F(TestApi, usageExample)
{
    signal(SIGINT, sig_handler);
    
    
    std::vector<XBot::MatLogger2::Ptr> loggers;
    auto appender = XBot::MatLoggerManager::MakeInstance();
    
    for(int i = 1; i <= 10; i++)
    {
        std::string name = "/tmp/my_log_" + std::to_string(i) + ".mat";
        loggers.emplace_back(XBot::MatLogger2::MakeLogger(name));
        appender->add_logger(loggers.back());
    }

    appender->start_thread();
    
    std::vector<std::string> vars;
    const int VAR_SIZE = 25;
    for(int i = 0; i < 35; i++)
    {
        vars.emplace_back("my_var_" + std::to_string(i+1));
        for(auto& logger : loggers)
            logger->create(vars.back(), VAR_SIZE, 1, 1e4 + i*40);
    }
    
    for(auto& logger : loggers)
        logger->create("my_mat_var", 10, 10);
    
    for(auto& logger : loggers)
        logger->create("wr_time", 1, 1, 350*1e4);
    
    int i = 0; 
    uint64_t bytes = 0;
    uint64_t bytes_last = 0;
    double write_time = 0.0;
    Eigen::VectorXd data(VAR_SIZE);
    auto tic = std::chrono::high_resolution_clock::now();
    auto t_last = std::chrono::high_resolution_clock::now();
    std::chrono::seconds test_duration(60);
    while(run_ok)
    {
        for(auto v : vars)
        {
            data.setConstant(data.size(), i);
            
            double time = measure_sec
            (
                [&]()
                {
                    for(auto& logger : loggers)
                        logger->add(v, data);
                }
            );
            
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
            
            if(now - tic > test_duration) break;
        }
        

        usleep(1000);
    }
    auto toc = std::chrono::high_resolution_clock::now();
    int sec = std::chrono::duration_cast<std::chrono::seconds>(toc-tic).count();
    
    
    printf("Written %d MB in %d sec (write rata = %.1f GB/s, last data = %d)\n", 
           int(bytes*1e-6), sec, bytes/write_time*1e-9, i-1);
    
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
