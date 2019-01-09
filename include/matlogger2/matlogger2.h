#ifndef __XBOT_MATLOGGER2_H__
#define __XBOT_MATLOGGER2_H__

#include <string>
#include <memory>
#include <unordered_map>

#include <eigen3/Eigen/Dense>

#include <matlogger2/utils/thread.h>
#include <matlogger2/utils/var_buffer.h>

class _mat_t;
typedef struct _mat_t mat_t;

namespace XBot 
{
    /**
    * @brief The MatLogger2 class allows the user to save numeric variables
    * (scalars, vectors, matrices) to HDF5 MAT-files. 
    * 
    * Output formatting: 
    *  - scalars are appended to form a column vector
    *  - vectors are appended to form a sample_size x num_samples matrix 
    *  - matrices are stacked to form a rows x cols x num_samples cube
    * 
    * Construction: 
    * via the MakeLogger(..) factory method (see the defined private
    * constructors for the available signatures, 
    * e.g. MatLogger2(std::string filename).
    * 
    * Usage: 
    * variables are created via the create() method. 
    * Elements are added to a variable with the add() method. 
    * Different overloads are provided for Eigen3 types, std::vector,
    * and scalars. 
    * Such elements are stored inside an internal buffer, which can be flushed 
    * to disk by calling flush_available_data(). It is possible to be notified 
    * whenever new data is available for flushing, by registering a callback 
    * through the set_on_data_available_callback() method.
    * 
    * Multithreaded usage:
    * The MatLogger2 class can be used inside a multi-threaded environment, 
    * provided that the following constraints are satisfied.
    *   1) only one "producer" thread shall call create() and add() concurrently
    *   2) only one "consumer" thread shall call flush_available_data() concurrently
    * The MatLoggerManager class (see matlogger2/utils/matlogger_manager.h) provides a 
    * ready-to-use consumer thread that periodically writes available data to disk.
    */
    class MatLogger2 
    {
        
    public:
        
        typedef std::weak_ptr<MatLogger2> WeakPtr;
        typedef std::shared_ptr<MatLogger2> Ptr;
        
        /**
        * @brief Default buffer size for logged variables,
        * expressed as the number of samples that the buffer
        * can hold.
        */
        static const int DEFAULT_BUF_SIZE = 1e4;
        
        
        /**
        * @brief Factory method that must be used to construct a 
        * MatLogger2 instance.
        * If no exception is thrown, it returns a shared pointer 
        * to the constructed instance. 
        * 
        * The function internally calls one of the available constructor overload.
        */
        template <typename... Args>
        static Ptr MakeLogger(Args... args);
        
        /**
        * @brief Set the callback that is invoked whenever new data is available 
        * for writing to disk.
        */
        void set_on_data_available_callback(VariableBuffer::CallbackType callback);
    
        /**
        * @brief Create a logged variable from its name as it will appear 
        * inside the MAT-file, dimensions, and buffer size.
        * 
        * @param var_name Variable name
        * @param rows Sample row size
        * @param cols Sample column size (defaults to 1)
        * @param buffer_size Buffer size in terms of number of elements 
        * (defaults to DEFAULT_BUF_SIZE)
        * @return True on success (variable name is unique, 
        * dimensions and buffer_size are > 0)
        */
        bool create(const std::string& var_name, 
                    int rows, int cols = 1, 
                    int buffer_size = DEFAULT_BUF_SIZE);
        
        
        /**
        * @brief Add an element to an existing variable
        * @return True on success (variable exists)
        */
        
        template <typename Derived>
        bool add(const std::string& var_name, const Eigen::MatrixBase<Derived>& data);
        
        template <typename Scalar>
        bool add(const std::string& var_name, const std::vector<Scalar>& data);
        
        bool add(const std::string& var_name, double data);
        
        /**
        * @brief Flush available data to disk.
        * 
        * @return The number of bytes that were written to disk.
        */
        int flush_available_data();
        
        ~MatLogger2();
        
    private:
        
        /**
        * @brief Construct from path to mat-file, which is erased if already
        * existing.
        * 
        * @param file Path to mat-file. If no extension is provided, 
        * a timestamp is automatically appended.
        */
        MatLogger2(std::string file);
        
        /**
        * @brief Force all variables to write their current block into their queue 
        * 
        * @return True if all variables succeed
        */
        bool flush_to_queue_all();
        
        
        // protect non-const access to _vars
        // producer must hold it during create(), and 
        // set_on_data_available_callback()
        // consumer must hold it during flush_available_data()
        matlogger2::MutexType _vars_mutex;
        
        // map of all defined variables 
        std::unordered_map<std::string, VariableBuffer> _vars;
        
        // callback that all variables shall use to notify that a new block is available 
        VariableBuffer::CallbackType _on_block_available;
        
        // path to mat-file
        std::string _file_name;
        
        // handle to mat-file
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
    
    return it->second.add_elem(data);;
    
}

template <typename Scalar>
inline bool XBot::MatLogger2::add(const std::string& var_name, const std::vector<Scalar>& data)
{
    Eigen::Map<const Eigen::Matrix<Scalar,-1,1>> map(data.data(), data.size());
    return add(var_name, map);
}


#endif
