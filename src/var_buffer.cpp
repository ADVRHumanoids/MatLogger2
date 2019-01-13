#include <matlogger2/utils/var_buffer.h>

#include "boost/spsc_queue_logger.hpp"

using namespace XBot;

VariableBuffer::BufferBlock::BufferBlock():
    _write_idx(0),
    _buf(0,0)
{

}

VariableBuffer::BufferBlock::BufferBlock(int dim, int block_size):
    _write_idx(0),
    _buf(dim, block_size)
{

}

int VariableBuffer::BufferBlock::get_size() const
{
    return _buf.cols();
}

const Eigen::MatrixXd& VariableBuffer::BufferBlock::get_data() const
{
    return _buf;
}

int VariableBuffer::BufferBlock::get_valid_elements() const
{
    return _write_idx;
}

void VariableBuffer::BufferBlock::reset()
{
    _write_idx = 0;
}

namespace lf = boost::lockfree;

/**
 * @brief The QueueImpl class implements the buffering strategy
 * for a single logged variable. This consists of:
 *  - a pool of available blocks, accessed only by the producer thread
 *  - a "read queue": produces pushes ready-to-consume blocks into it
 *  - a "write queue": consumed blocks are pushed into the queue in 
 *    and finally return inside the pool
 * 
 */
class VariableBuffer::QueueImpl
{
public:
    
    // fixed number of blocks
    static const int NUM_BLOCKS = 20;
    
    template <typename T>
    using LockfreeQueue = lf::spsc_queue<T, lf::capacity<NUM_BLOCKS>>;
    
    QueueImpl(int elem_size, int buffer_size)
    {
        // allocate all blocks and push them into the pool
        for(int i = 0; i < NUM_BLOCKS; i++)
        {
            _block_pool.push_back(std::make_shared<BufferBlock>(elem_size, buffer_size));
        }
        
        // pre allocate queues
        _read_queue.reset(BufferBlock::Ptr());
        _write_queue.reset(BufferBlock::Ptr());
    }
    
    
    /**
     * @brief Get a new block from the pool, if available
     * 
     * @return a shared pointer to a block, or a nullptr if non is available
     */
    BufferBlock::Ptr get_new_block()
    {
        // update pool with elements from write queue
        _write_queue.consume_all(
            [this](BufferBlock::Ptr block)
            {
                _block_pool.push_back(block);
            }
        );
        
        if(_block_pool.empty())
        {
            return nullptr;
        }
        
        auto ret = _block_pool.back();
        ret->reset();
        _block_pool.pop_back();
        
        return ret;
    }
    
    /**
     * @brief Handle to the read queue
     */
    LockfreeQueue<BufferBlock::Ptr>& get_read_queue()
    {
        return _read_queue;
    }
    
    /**
     * @brief Handle to the write queue
     */
    LockfreeQueue<BufferBlock::Ptr>& get_write_queue()
    {
        return _write_queue;
    }
    
    static int Size()
    {
        return NUM_BLOCKS;
    }
    
private:
    
    // pool of available blocks
    std::vector<BufferBlock::Ptr> _block_pool;
    
    // queue for blocks that are ready to be flushed by consumer
    LockfreeQueue<BufferBlock::Ptr> _read_queue;
    
    // queue for blocks that are ready to be filled by producer
    LockfreeQueue<BufferBlock::Ptr> _write_queue;
};

VariableBuffer::VariableBuffer(std::string name, 
                               int dim_rows,
                               int dim_cols, 
                               int block_size):
    _name(name),
    _rows(dim_rows),
    _cols(dim_cols),
    _queue(new QueueImpl(dim_rows*dim_cols, block_size))
{
    // intialize current block 
    _current_block = _queue->get_new_block();
}

std::pair< int, int > VariableBuffer::get_dimension() const
{
    return std::make_pair(_rows, _cols);
}

void VariableBuffer::set_on_block_available(CallbackType callback)
{
    _on_block_available = callback;
}

int VariableBuffer::BufferBlock::get_size_bytes() const
{
    return _buf.size() * sizeof(_buf[0]);
}

bool VariableBuffer::read_block(Eigen::MatrixXd& data, int& valid_elements)
{
    // this function is not allowed to use class members, 
    // except consuming elements from read queue
    // and pushing elements into write queue
    
    int ret = 0;
    
    BufferBlock::Ptr block;
    if(_queue->get_read_queue().pop(block))
    {
        // copy data from block to output buffer
        data = block->get_data();
        ret = block->get_valid_elements();
        
        // reset block and send it back to producer thread
        block->reset();
        _queue->get_write_queue().push(block);
    }
    
    valid_elements = ret;
    
    return ret > 0;
}

bool VariableBuffer::flush_to_queue()
{
    // no valid elements in the current block, we just return true
    if(_current_block->get_valid_elements() == 0)
    {
        return true;
    }
    
    // queue is full, return false and print to stderr
    if(!_queue->get_read_queue().push(_current_block))
    {
        fprintf(stderr, "Failed to push block for variable '%s'\n", _name.c_str());
        return false;
    }
    // we managed to push a block into the queue
    
    // if a callback was registered, we call it
    if(_on_block_available)
    {
        BufferInfo buf_info;
        buf_info.new_available_bytes = _current_block->get_size_bytes();
        buf_info.variable_free_space = _queue->get_read_queue().write_available() / (double)VariableBuffer::NumBlocks();
        buf_info.variable_name = _name.c_str();
        
        _on_block_available(buf_info);
    }
    
    // ask for a new block from the pool, so that new elements can be written to it
    _current_block = _queue->get_new_block();
    
    if(!_current_block)
    {
        fprintf(stderr, "Failed to get new block for variable '%s'\n", _name.c_str());
        throw std::runtime_error("TBD handle this in some way");
        return false;
    }
    
    return true;
        
}

int VariableBuffer::NumBlocks()
{
    return QueueImpl::Size();
}

VariableBuffer::~VariableBuffer()
{
    // this is needed to avoid the compiler generate a default destructor,
    // which is incompatible with the forward-declaration of QueueImpl
}


