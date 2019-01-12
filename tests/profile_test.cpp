#include <matlogger2/matlogger2.h>
#include <matlogger2/utils/matlogger_manager.h>

#include <unistd.h>

XBot::MatLogger2::Ptr logger;
std::vector<std::string> vars;

int log_data(const Eigen::VectorXd& data)
{
    for(auto& v : vars)
    {
        logger->add(v, data);
    }
}

int main()
{
//     auto manager = XBot::MatLoggerManager::MakeInstance();
    auto _logger = XBot::MatLogger2::MakeLogger("/tmp/profile_log");
    logger = _logger;
//     manager->add_logger(logger);
    
    const int VAR_SIZE = 45;
    for(int i = 0; i < 35; i++)
    {
        vars.emplace_back("my_var_" + std::to_string(i+1));
        logger->create(vars.back(), VAR_SIZE, 1, 1e4 + i*40);
    }
    
    Eigen::VectorXd data(VAR_SIZE);
    
    for(int i = 0; i < 2e4; i++)
    {
        log_data(data);
    }
    
    logger.reset();
    
}