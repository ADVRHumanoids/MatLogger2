#ifndef __XBOT_MATLOGGER2_BUFFER_BLOCK__
#define __XBOT_MATLOGGER2_BUFFER_BLOCK__

#include <string>
#include <eigen3/Eigen/Dense>

#include <matlogger2/utils/boost/spsc_queue_logger.hpp>

namespace XBot 
{

    /**
    * @brief The VariableBuffer class implements a memory buffer for
    * a single logged variable. This is an internal library component,
    * and it is not meant for direct use.
    * 
    * The memory buffer is splitted into a fixed number of blocks.
    * The VariableBuffer keeps an internal lockfree queue of such blocks,
    * plus another one that is used for writing logged data samples.
    * When this block is full, it is pushed into the queue.
    * 
    * Because the lockfree queue is of the Single-Producer-Single-Consumer
    * type, a single thread is allowed to call add_elem and read_block,
    * concurrently.
    * 
    */
    class VariableBuffer 
    {
    
    public:
        
        typedef std::function<void(int, int)> CallbackType;
        
        /**
        * @brief Constructor
        * 
        * @param name Variable name
        * @param dim_rows Sample rows number
        * @param dim_cols Sample columns number
        * @param block_size Number of samples that make up a block
        */
        VariableBuffer(std::string name, int dim_rows, int dim_cols, int block_size);
        
        /**
        * @brief Sets a callback that is used to notify that a new block
        * has been pushed into the queue.
        */
        void set_on_block_available(CallbackType callback);
        
        const std::string& get_name() const;
        
        std::pair<int, int> get_dimension() const;
        
        /**
        * @brief Add an element to the buffer. If there is no space inside the 
        * current block, this is pushed into the queue by calling flush_to_queue().
        * 
        * Only a single producer thread is allowed to concurrently call this
        * method.
        * 
        * @param Derived Data type
        * @param data Data value
        * @return False if data could not be saved inside the buffer
        */
        template <typename Derived>
        bool add_elem(const Eigen::MatrixBase<Derived>& data);
        
        /**
        * @brief Reads a whole block from the queue, if one is available.
        * 
        * Only a single consumer thread is allowed to concurrently call this 
        * method.
        * 
        * @param data Matrix which is filled with the read block (unless the function returns false)
        * @param valid_elements Number of valid elements contained in the block. This means that only 
        * data.leftCols(valid_elements) contains valid data.
        * @return True if valid_elements > 0
        */
        bool read_block(Eigen::MatrixXd& data, int& valid_elements);
        
        /**
        * @brief Writes current block to the queue.
        * 
        * @return True on success (queue was not full)
        */
        bool flush_to_queue();
        
        static const int NUM_BLOCKS = 40;
        
    private:
        
        class BufferBlock
        {
            
        public:
            
            BufferBlock();
            BufferBlock(int dim, int block_size);
            
            template <typename Derived>
            bool add(const Eigen::MatrixBase<Derived>& data);
            void reset();
            
            const Eigen::MatrixXd& get_data() const;
            int get_valid_elements() const;
            int get_size() const;
            int get_size_bytes() const;
            
        private:
            
            int _write_idx;
            Eigen::MatrixXd _buf;
            
        };
        
        std::string _name;
        int _rows;
        int _cols;
        
        template <typename T, int N>
        using LockfreeQueue = boost::lockfree::spsc_queue<T, boost::lockfree::capacity<N>>;
        
        BufferBlock _current_block;
        LockfreeQueue<BufferBlock, NUM_BLOCKS> _queue;
        CallbackType _on_block_available;
        
    };
    

 
}



template <typename Derived>
inline bool XBot::VariableBuffer::BufferBlock::add(const Eigen::MatrixBase<Derived>& data)
{
    if(_write_idx == get_size())
    {
        return false;
    }

    double * col_ptr = _buf.data() + _write_idx*_buf.rows();
    
    Eigen::Map<Eigen::MatrixXd> elem_map(col_ptr, 
                                         data.rows(), data.cols());
    
    elem_map.noalias() = data.template cast<double>();

    _write_idx++;
    
    return true;
}


template <typename Derived>
inline bool XBot::VariableBuffer::add_elem(const Eigen::MatrixBase<Derived>& data)
{
    if( data.size() != _rows*_cols )
    {
        fprintf(stderr, "Unable to add element to variable '%s': \
size does not match (%d vs %d)\n",
                _name.c_str(), (int)data.size(), _rows*_cols);
        
        return false;
    }
    
    
    if(!_current_block.add(data))
    {
        
        flush_to_queue();
        
        _current_block.reset();
        
        return add_elem(data);
    }
    
    return true;
}



#endif
