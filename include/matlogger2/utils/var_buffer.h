#ifndef __XBOT_MATLOGGER2_BUFFER_BLOCK__
#define __XBOT_MATLOGGER2_BUFFER_BLOCK__

#include <string>
#include <memory>

#include <Eigen/Dense>

#include "matlogger2/utils/visibility.h"

namespace XBot 
{

    /**
    * @brief The VariableBuffer class implements a memory buffer for
    * a single logged variable. This is an internal library component,
    * and it is not meant for direct use.
    * 
    * The memory buffer is splitted into a fixed number of blocks, that make
    * up a "pool" of available memory. When a block is full, it is pushed into
    * a lockfree queue, so that it is available for the consumer thread.
    * As soon as the block is consumed, it is returned back to the pool via 
    * another lockfree queue.
    * 
    * Apart from the lockfree queues, no other data is shared between add_elem()
    * and read_block(). So, they can be called concurrently without further 
    * synchronization.
    * 
    * Because the lockfree queue is of the Single-Producer-Single-Consumer
    * type, a single thread is allowed to call add_elem and read_block,
    * concurrently.
    * 
    */
    class MATL2_API VariableBuffer
    {
    
    public:
        
        /**
        * @brief Enum for specifying the type of buffer
        */
        enum class Mode
        {
            // producer-consumer mode (possibly dual thread):
            // the user (or the provided MatAppender)
            // is responsible for keeping the buffer free                   
            producer_consumer,  
            
            // circular buffer mode (single thread)
            circular_buffer    
        };
        
        struct BufferInfo
        {
            // name of the variable that the new data refers to
            const char * variable_name;
            
            // bytes available to be flushed to disk
            int new_available_bytes;
            
            // free space available in buffer (0..1, 0 indicates full buffer)
            double variable_free_space;
        };
        
        typedef std::function<void(BufferInfo)> CallbackType;
        
        /**
        * @brief Constructor
        * 
        * @param name Variable name
        * @param dim_rows Sample rows number
        * @param dim_cols Sample columns number
        * @param block_size Number of samples that make up a block
        */
        VariableBuffer(std::string name, 
                       int dim_rows, int dim_cols, 
                       int block_size);
        
        /**
        * @brief Sets a callback that is used to notify that a new block
        * has been pushed into the queue.
        */
        void set_on_block_available(CallbackType callback);
        
        /**
        * @brief Set whether this buffer should be treated as a (possibly dual threaded) 
        * producer-consumer queue, or as a single-threaded circular buffer.
        * By default, the producer_consumer mode is used.
        * 
        * NOTE: only call this method before starting using the logger!!
        */
        void set_buffer_mode(VariableBuffer::Mode mode);
        
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
        * The block is then returned to the pool.
        * 
        * Only a single consumer thread is allowed to concurrently call this 
        * method.
        * 
        * @param data Matrix which is filled with the read block (unless the function returns false)
        * @param valid_elements Number of valid elements contained in the block. This means that only 
        * data.leftCols(valid_elements) contains valid data.
        * @return True if valid_elements > 0
        */
        bool read_block(Eigen::MatrixXd& data, 
                        int& valid_elements);
        
        /**
        * @brief Writes current block to the queue. If a callback was registered through
        * set_on_block_available(), it is called on success.
        * 
        * @return True on success (queue was not full)
        */
        bool flush_to_queue();
        
        static int NumBlocks();
        
        ~VariableBuffer();
        
    private:
        
        /**
        * @brief The BufferBlock class represents a block of memory that
        * can hold block_size samples of a logged variable, stored
        * column-wise.
        */
        class BufferBlock
        {
            
        public:
            
            typedef std::shared_ptr<BufferBlock> Ptr;
            
            BufferBlock();
            
            /**
            * @param dim number of elements of the sample (rows*cols)
            * @param block_size number of samples that the block will hold
            */
            BufferBlock(int dim, int block_size);
            
            
            /**
            * @brief Add one sample to the block, unless the block is full
            * 
            * @param Derived Eigen-type of the sample
            * @param data Sample to be added
            * @return True on success, false if the block is full
            */
            template <typename Derived>
            bool add(const Eigen::MatrixBase<Derived>& data);
            
            
            /**
            * @brief Reset the buffer to an empty condition.
            */
            void reset();
            
            /**
            * @brief Returns a reference to the memory block. Note that the last
            * columns may be invalid: only the first get_valid_elements() columns 
            * contain valid data.
            */
            const Eigen::MatrixXd& get_data() const;
            
            
            /**
            * @brief Number of elements that have been written to the block from
            * when it was reset.
            */
            int get_valid_elements() const;
            
            int get_size() const;
            int get_size_bytes() const;
            
            
        private:
            
            // current write index (also equals the number of valid elements)
            int _write_idx; 
            
            // memory for get_size() elements, stored column-wise
            Eigen::MatrixXd _buf;
            
        };
        
        // mode
        Mode _buffer_mode;
        
        // variable name
        std::string _name;
        
        // variable dimensions
        int _rows;
        int _cols;
        
        // current block
        BufferBlock::Ptr _current_block;
        
        // fifo spsc queue of blocks 
        class QueueImpl;
        std::unique_ptr<QueueImpl> _queue;
        
        // function to be called when a block is pushed into the queue
        CallbackType _on_block_available;
        
    };
    

 
}



template <typename Derived>
inline bool XBot::VariableBuffer::BufferBlock::add(const Eigen::MatrixBase<Derived>& data)
{
    // check if the block is full, and return false
    if(_write_idx == get_size())
    {
        return false;
    }

    // pointer to the _write_idx-th element (column of _buf)
    double * col_ptr = _buf.data() + _write_idx*_buf.rows();
    
    // Eigen-view on the column to be written
    Eigen::Map<Eigen::MatrixXd> elem_map(col_ptr, 
                                         data.rows(), data.cols());
    
    // cast data do double and write it to the current element
    elem_map.noalias() = data.template cast<double>();

    // increase _write_idx 
    _write_idx++;
    
    // if the block is not full, return true
    return true;
}


template <typename Derived>
inline bool XBot::VariableBuffer::add_elem(const Eigen::MatrixBase<Derived>& data)
{
    // check data size correctness
    if( data.size() != _rows*_cols )
    {
        fprintf(stderr, "Unable to add element to variable '%s': \
size does not match (%d vs %d)\n",
                _name.c_str(), (int)data.size(), _rows*_cols);
        
        return false;
    }
    
    // if current block is full, we push it into the queue, and try again
    if(!_current_block->add(data))
    {
        
        // write current block to queue
        flush_to_queue();
        
        // reset current block
        _current_block->reset();
        
        return add_elem(data);
    }
    
    return true;
}



#endif
