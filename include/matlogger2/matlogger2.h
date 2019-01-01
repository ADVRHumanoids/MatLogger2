#ifndef __XBOT_MATLOGGER2_H__
#define __XBOT_MATLOGGER2_H__

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

#include <eigen3/Eigen/Dense>

#include <matlogger2/utils/var_buffer.h>

class _mat_t;
typedef struct _mat_t mat_t;

namespace XBot 
{
    class MatLogger2 
    {
        
    public:
        
        typedef std::weak_ptr<MatLogger2> WeakPtr;
        typedef std::shared_ptr<MatLogger2> Ptr;
        
        template <typename... Args>
        static Ptr MakeLogger(Args... args);
        
        void set_on_data_available_callback(VariableBuffer::CallbackType callback);
        void set_on_stop_callback(std::function<void(void)> stop_callback);
    
        bool create(const std::string& var_name, int rows, int cols, int buffer_size = -1);
        
        template <typename Derived>
        bool add(const std::string& var_name, const Eigen::MatrixBase<Derived>& data);
        
        template <typename Scalar>
        bool add(const std::string& var_name, const std::vector<Scalar>& data);
        
        bool add(const std::string& var_name, double data);
        
        int flush_available_data();
        
        ~MatLogger2();
        
    private:
        
        MatLogger2(std::string file);
        
        void flush_thread_main();
        bool flush_to_queue_all();
  
        std::mutex _vars_mutex;
        std::unordered_map<std::string, VariableBuffer> _vars;
        std::function<void(void)> _notify_logger_finished;
        VariableBuffer::CallbackType _on_block_available;
        
        std::string _file_name;
        mat_t * _mat_file;

        
    };
    


}

template <typename... Args>
inline XBot::MatLogger2::Ptr XBot::MatLogger2::MakeLogger(Args... args)
{
    return Ptr(new MatLogger2(args...));
}
        
template <typename Derived>
inline bool XBot::MatLogger2::add(const std::string& var_name, const Eigen::MatrixBase< Derived >& data)
{
    auto it = _vars.find(var_name);
    
    if(it == _vars.end())
    {
        fprintf(stderr, "Variable '%s' does not exist\n", var_name.c_str());
        return false;
    }
    
    it->second.add_elem(data);
    
    return true;
    
}

template <typename Scalar>
inline bool XBot::MatLogger2::add(const std::string& var_name, const std::vector<Scalar>& data)
{
    Eigen::Map<const Eigen::Matrix<Scalar,-1,1>> map(data.data(), data.size());
    return add(var_name, map);
}


#endif
