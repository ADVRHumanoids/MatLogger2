#include "matlogger2/matlogger2.h"
#include "matlogger2/utils/mat_appender.h"

#include <unistd.h>

int log_data(const auto& data, const auto& vars, auto& logger)
{
    for(auto& v : vars)
    {
        logger->add(v, data);
    }
}

int main()
{
    std::vector<std::string> vars;
    auto logger = XBot::MatLogger2::MakeLogger("/tmp/profile_log");
    logger->set_buffer_mode(XBot::VariableBuffer::Mode::circular_buffer);
    
    const int VAR_SIZE = 45;
    for(int i = 0; i < 1; i++)
    {
        vars.emplace_back("my_var_" + std::to_string(i+1));
        logger->create(vars.back(), VAR_SIZE, 1, 2e4 + 100);
    }
    
    Eigen::VectorXd data(VAR_SIZE);
    
    for(int i = 0; i < 8e4; i++)
    {
        data.setConstant(VAR_SIZE, i);
        log_data(data, vars, logger);
    }
    
    
}
