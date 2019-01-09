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

TEST_F(TestApi, usageExample)
{
    
    std::vector<XBot::MatLogger2::Ptr> loggers;
    auto appender = XBot::MatLoggerManager::MakeInstance();
    
    for(int i = 1; i <= 10; i++)
    {
        std::string name = "/tmp/my_log_" + std::to_string(i);
        if(i%2==0)
        {
            name += ".mat";
        }
        loggers.emplace_back(XBot::MatLogger2::MakeLogger(name));
        appender->add_logger(loggers.back());
    }

    appender->start_flush_thread();
    
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
    std::chrono::seconds test_duration(5);
    while(true)
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

TEST_F(TestApi, checkTypes)
{
    auto logger = XBot::MatLogger2::MakeLogger("/tmp/checkTypes_logger.mat");
    ASSERT_TRUE(logger->create("scalar_var"  , 1));
    ASSERT_FALSE(logger->create("scalar_var" , 1));
    ASSERT_FALSE(logger->create("invalid_var", 0));
    ASSERT_FALSE(logger->create("invalid_var", 1, 0));
    ASSERT_FALSE(logger->create("invalid_var", 1, 1, 0));
    ASSERT_TRUE(logger->create("vector_var"  , 10));
    ASSERT_TRUE(logger->create("mat_var"     , 10, 10));
    
    ASSERT_TRUE(logger->add("scalar_var", (uint)  1));
    ASSERT_TRUE(logger->add("scalar_var", (int)   1));
    ASSERT_TRUE(logger->add("scalar_var", (float) 1));
    ASSERT_TRUE(logger->add("scalar_var", (double)1));
    
    
    
    
    
    Eigen::VectorXi vector_valid_i(10);
    Eigen::VectorXd vector_valid_d(10);
    Eigen::VectorXf vector_valid_f(10);
    
    
    std::cout << malloc(19) << std::endl;
    
    ASSERT_TRUE(logger->add("vector_var", vector_valid_i));
    ASSERT_TRUE(logger->add("vector_var", vector_valid_f));
    ASSERT_TRUE(logger->add("vector_var", vector_valid_d));
    
    Eigen::MatrixXd matrix_valid_d(10, 10);
    Eigen::MatrixXf matrix_valid_f(10, 10);
    Eigen::MatrixXi matrix_valid_i(10, 10);
    
    ASSERT_TRUE(logger->add("vector_var", matrix_valid_d.col(0)));
    ASSERT_TRUE(logger->add("vector_var", matrix_valid_f.col(0)));
    ASSERT_TRUE(logger->add("vector_var", matrix_valid_i.col(0)));
    
    ASSERT_TRUE(logger->add("vector_var", matrix_valid_d.row(0)));
    ASSERT_TRUE(logger->add("vector_var", matrix_valid_f.row(0)));
    ASSERT_TRUE(logger->add("vector_var", matrix_valid_i.row(0)));
    
    ASSERT_TRUE(logger->add("mat_var", matrix_valid_d));
    ASSERT_TRUE(logger->add("mat_var", matrix_valid_f));
    ASSERT_TRUE(logger->add("mat_var", matrix_valid_i));
    
    ASSERT_FALSE(logger->add("vector_var", matrix_valid_d));
    ASSERT_FALSE(logger->add("vector_var", matrix_valid_f));
    ASSERT_FALSE(logger->add("vector_var", matrix_valid_i));
    
    ASSERT_FALSE(logger->add("mat_var", vector_valid_i));
    ASSERT_FALSE(logger->add("mat_var", vector_valid_f));
    ASSERT_FALSE(logger->add("mat_var", vector_valid_d));
    
    ASSERT_FALSE(logger->add("invalid_var", vector_valid_i));
    
    std::vector<int>    stdvec_i(10);
    std::vector<uint>   stdvec_u(10);
    std::vector<float>  stdvec_f(10);
    std::vector<double> stdvec_d(10);
    
    
    ASSERT_TRUE(logger->add("vector_var", stdvec_i));
    ASSERT_TRUE(logger->add("vector_var", stdvec_f));
    ASSERT_TRUE(logger->add("vector_var", stdvec_d));
    ASSERT_TRUE(logger->add("vector_var", stdvec_u));
    
    
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
