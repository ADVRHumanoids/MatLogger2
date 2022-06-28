#ifndef __XBOT_MATLOGGER2_H__
#define __XBOT_MATLOGGER2_H__

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <queue>
#include <Eigen/Dense>

#include "matlogger2/utils/var_buffer.h"
#include "matlogger2/mat_data.h"

#include "matlogger2/utils/visibility.h"

namespace XBot 
{
    namespace matlogger2 
    {
        class MATL2_API Backend;
    }

    /**
    * @brief The MatLogger2 class allows the user to save numeric variables
    * (scalars, vectors, matrices), structures and cell arrays to HDF5 MAT-files. 
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
    * Standard numeric variables(scalars, vectors and matrices) are created via the create() method. 
    * Standard numeric elements are added to a variable with the add() method, while structures and arrays with the create() one. 
    * Different overloads are provided for Eigen3 types, std::vector,
    * and scalars. 
    * Such elements are stored inside an internal buffer, which can be flushed 
    * to disk by calling flush_available_data(). It is possible to be notified 
    * whenever new data is available for flushing, by registering a callback 
    * through the set_on_data_available_callback() method.
    * 
    * Different functioning mode are available, see also set_buffer_mode()
    * 
    * Producer-consumer multi-threaded usage:
    * Logged samples are pushed into a queue. The MatLogger2 class expects the 
    * user to periodically free such a queue by consuming its elements 
    * (flush_available_data()). If the buffer becomes full, some data can go lost.
    * In producer-consumer mode, the MatLogger2 class can be used inside a 
    * multi-threaded environment, provided that the following constraints are satisfied:
    *   1) only one "producer" thread shall call create() and add()
    *   2) only one "consumer" thread shall call flush_available_data()
    * The MatAppender class (see matlogger2/utils/mat_appender.h) provides a 
    * ready-to-use consumer thread that periodically writes available data to disk.
    * 
    * Circular-buffer single-threaded usage:
    * Logged samples are pushed into a circular buffer. In circular-buffer mode,
    * the user CAN NOT call flush_available_data(). 
    * If the buffer becomes full, older samples are overwritten.
    * In circular-buffer mode, the MatLogger2 class can NOT be used inside a 
    * multi-threaded environment (i.e. additional synchronization must be provided
    * by the user)
    */
    class MATL2_API MatLogger2
    {
        
    public:
        
        
        typedef std::weak_ptr<MatLogger2> WeakPtr;
        typedef std::shared_ptr<MatLogger2> Ptr;
        typedef std::unique_ptr<MatLogger2> UniquePtr;
        
        struct MATL2_API Options
        {
            bool enable_compression = false;
            bool load_file_from_path = false; // option to load an already existing mat file, instead of creating it
            int default_buffer_size;
            int default_buffer_size_max_bytes;
            
            Options();
        };
        
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
         * @brief Returns the full path associated with this logger object
         */
        const std::string& get_filename() const;
        
        /**
         * @brief Returns the MatLogger2::Options struct associated with this 
         * object
         */
        Options get_options() const;
        
        /**
        * @brief Set the callback that is invoked whenever new data is available 
        * for writing to disk.
        */
        void set_on_data_available_callback(VariableBuffer::CallbackType callback);
        
        /**
        * @brief Set whether this buffer should be treated as a (possibly dual threaded) 
        * producer-consumer queue, or as a single-threaded circular buffer.
        * By default, the producer_consumer mode is used.
        * 
        * NOTE: only call this method before starting using the logger!
        */
        void set_buffer_mode(VariableBuffer::Mode buffer_mode);
    
        /**
        * @brief Create a logged variable from its name as it will appear 
        * inside the MAT-file, dimensions, and buffer size.
        * 
        * @param var_name Variable name
        * @param rows Sample row size
        * @param cols Sample column size (defaults to 1)
        * @param buffer_size Buffer size in terms of number of elements 
        * (defaults to get_options().default_buffer_size)
        * @return True on success (variable name is unique, 
        * dimensions and buffer_size are > 0)
        */
        bool create(const std::string& var_name, 
                    int rows, int cols = 1, 
                    int buffer_size = -1);
        
        
        /**
        * @brief Add an element to an existing variable
        * @return True on success (variable exists)
        */
        
        template <typename Derived>
        bool add(const std::string& var_name, const Eigen::MatrixBase<Derived>& data);
        
        template <typename Scalar>
        bool add(const std::string& var_name, const std::vector<Scalar>& data);
        
        template <typename Iterator>
        // this overload may allocate a temporary vector!
        bool add(const std::string& var_name, Iterator begin, Iterator end);
        
        bool add(const std::string& var_name, double data);

        bool save(const std::string& var_name,
                  const matlogger2::MatData& var_data);

        bool save(const std::string& var_name,
                  matlogger2::MatData&& var_data);
        
        bool readvar(const std::string& var_name, 
                     Eigen::MatrixXd& mat_data,
                     int& slices);

        bool read_container(const std::string& var_name, matlogger2::MatData& matdata); // double scalar types are automatically casted to MatrixXd upon reading (MatIO does not distinguish between matrices and scalars)

        bool delvar(const std::string& var_name);

        bool get_mat_var_names(std::vector<std::string>& var_names);

        /**
        * @brief Flush available data to disk.
        * 
        * @return The number of bytes that were written to disk.
        */
        int flush_available_data();
        
        /**
         * @brief Destructor flushes all buffers to disk, then releases
         * any resource connected with the underlying MAT-file
         */
        ~MatLogger2();
        
    private:
        
        /**
        * @brief Construct from path to mat-file, which is erased if already
        * existing.
        * 
        * @param file Path to mat-file. If no extension is provided, 
        * a timestamp is automatically appended.
        */
        MatLogger2(std::string file, 
                   Options opt = Options());
        
        /**
        * @brief Force all variables to write their current block into their queue 
        * 
        * @return True if all variables succeed
        */
        MATL2_LOCAL bool flush_to_queue_all();
        
        /**
        * @brief Return a pointer to the requested variable. If it does 
        * not exist, it tries to create one with the provided dimentions.
        */
        VariableBuffer * find_or_create(const std::string& var_name,
                                        int rows, int cols
                                        );
        
        
        // option struct
        Options _opt;
        
        // protect non-const access to _vars
        // producer must hold it during create(), and 
        // set_on_data_available_callback()
        // consumer must hold it during flush_available_data()
        class MATL2_LOCAL MutexImpl;
        std::unique_ptr<MutexImpl> _vars_mutex;
        
        // map of all defined variables 
        std::unordered_map<std::string, VariableBuffer> _vars;
        
        // buffer mode
        VariableBuffer::Mode _buffer_mode;
        
        // callback that all variables shall use to notify that a new block is available 
        VariableBuffer::CallbackType _on_block_available;
        
        // path to mat-file
        std::string _file_name;
        
        // handle to backend object
        std::unique_ptr<matlogger2::Backend> _backend;

        std::unique_ptr<MutexImpl> _matdata_queue_mutex;
        std::queue<std::pair<std::string, matlogger2::MatData>> _matdata_queue;
        
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
    VariableBuffer * vbuf = find_or_create(var_name, data.rows(), data.cols());
    
    return vbuf && vbuf->add_elem(data);
    
}

template<typename Iterator> 
inline bool XBot::MatLogger2::add(const std::string& var_name, Iterator begin, Iterator end)
{
    static std::vector<double> tmp;
    tmp.clear();
    tmp.assign(begin, end);
    
    return add(var_name, tmp);
    
}


template <typename Scalar>
inline bool XBot::MatLogger2::add(const std::string& var_name, const std::vector<Scalar>& data)
{
    Eigen::Map<const Eigen::Matrix<Scalar,-1,1>> map(data.data(), data.size());
    return add(var_name, map);
}


#endif
