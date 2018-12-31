#ifndef __XBOT_MATLOGGER2_BUFFER_BLOCK__
#define __XBOT_MATLOGGER2_BUFFER_BLOCK__

#include <string>
#include <eigen3/Eigen/Dense>

#include <matlogger2/utils/boost/spsc_queue_logger.hpp>

namespace XBot 
{

    class VariableBuffer 
    {
    
    public:
        
        typedef std::function<void(int, int)> CallbackType;
        
        VariableBuffer(std::string name, int dim_rows, int dim_cols, int block_size);
        
        void set_on_block_available(CallbackType callback);
        
        const std::string& get_name() const;
        
        std::pair<int, int> get_dimension() const;
        
        template <typename Derived>
        bool add_elem(const Eigen::MatrixBase<Derived>& data);
        
        bool read_block(Eigen::MatrixXd& data, int& valid_elements);
        
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

    static_assert(Derived::ColsAtCompileTime == 1, 
                  "add() method only supports elements with single column \
                   consider wrapping your data into an Eigen::Map<Eigen::VectorXd> object");
    
    _buf.col(_write_idx) = data.template cast<double>();
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