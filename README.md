# MatLogger2
Library for logging of numeric data to HDF5 MAT-files, which is RT-safe and multithreaded.

## Building MatLogger2 from source
Standard cmake workflow:
 - git clone the repo to *SOURCE_DIR*
 - create a build folder *BUILD_DIR*
 - cd *BUILD_DIR* && cmake *SOURCE_DIR*
 - make
 - make install
 
## Installing the binary release
  - cd *SOURCE_DIR*/release
  - sudo dpkg -i matlogger2-X.Y.Z-Linux.deb
  
The binary release has been tested only with Ubuntu 16.04

## Linking against MatLogger2
```cmake
find_package(matlogger2 REQUIRED)
target_link_libraries(mytarget matlogger2::matlogger2)
```

 ## Quick start
 ### Circular-buffer mode
 ```c++
 #include <matlogger2/matlogger2.h>
 
 int main()
 {
    auto logger = XBot::MatLogger2::MakeLogger("/tmp/my_log");
    logger->set_buffer_mode(XBot::VariableBuffer::Mode::circular_buffer);
    
    for(int i = 0; i < 1e5; i++)
    {
        Eigen::VectorXd vec(10);
        Eigen::MatrixXd mat(6,8);
        
        logger->add("my_vec_var", vec);
        logger->add("my_scalar_var", M_PI);
        logger->add("my_mat_var", mat);
    }
    
 } // destructor flushes to disk
 ```
 
  ### Producer-consumer mode (default)
 ```c++
 #include <matlogger2/matlogger2.h>
 #include <matlogger2/utils/mat_appender.h>
 
 int main()
 {
    auto logger = XBot::MatLogger2::MakeLogger("/tmp/my_log");
    auto appender = XBot::MatAppender::MakeInstance();
    appender->add_logger(logger);
    appender->start_flush_thread();
    
    for(int i = 0; i < 1e5; i++)
    {
        Eigen::VectorXd vec(10);
        Eigen::MatrixXd mat(6,8);
        
        logger->add("my_vec_var", vec);
        logger->add("my_scalar_var", M_PI);
        logger->add("my_mat_var", mat);
        
        usleep(1000);
    }
    
 }
 ```
 
 ## Documentation
 Header files are documented with doxygen!
