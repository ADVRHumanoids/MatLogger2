#include <matlogger2/matlogger2.h>
#include <matio-cmake/matio/src/matio.h>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <matlogger2/utils/thread.h>

using namespace matlogger2;

using namespace XBot;

namespace
{
    template <typename Func>
    double measure_sec(Func f)
    {
        auto tic = std::chrono::high_resolution_clock::now();
        
        f();
        
        auto toc = std::chrono::high_resolution_clock::now();
        
        return std::chrono::duration_cast<std::chrono::nanoseconds>(toc-tic).count()*1e-9;
    }
}

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




VariableBuffer::VariableBuffer(std::string name, 
                               int dim_rows,
                               int dim_cols, 
                               int block_size):
    _name(name),
    _current_block(dim_rows*dim_cols, block_size),
    _rows(dim_rows),
    _cols(dim_cols)
{
    // allocate memory for the whole queue
    _queue.reset(_current_block);
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
    // except consuming elements from _queue
    
    int ret = 0;
    
    _queue.consume_one
    (
        [&ret, &data](const BufferBlock& block)
        {
            data = block.get_data();
            ret = block.get_valid_elements();
        }
    );
    
    valid_elements = ret;
    
    return ret > 0;
}

bool VariableBuffer::flush_to_queue()
{
    // no valid elements in the current block, we just return true
    if(_current_block.get_valid_elements() == 0)
    {
        return true;
    }
    
    // queue is full, return false and print to stderr
    if(!_queue.push(_current_block))
    {
        fprintf(stderr, "Failed to push block for variable '%s'\n", _name.c_str());
        return false;
    }
    // we managed to push a block into the queue
    
    // reset current block, so that new elements can be written to it
    _current_block.reset();
    
    // if a callback was registered, we call it
    if(_on_block_available)
    {
        BufferInfo buf_info;
        buf_info.new_available_bytes = _current_block.get_size_bytes();
        buf_info.variable_free_space = _queue.write_available() / (double)VariableBuffer::NUM_BLOCKS;
        buf_info.variable_name = _name.c_str();
        
        _on_block_available(buf_info);
    }
    
    return true;
        
}


class MatLogger2::MutexImpl
{
public:
    
    matlogger2::MutexType& get()
    {
        return _mutex;
    }
    
private:
    
    matlogger2::MutexType _mutex;
};

const std::string& VariableBuffer::get_name() const
{
    return _name;
}

namespace{
    
    std::string get_file_extension(std::string file)
    {
        std::vector<std::string> token_list;
        boost::split(token_list, file, [](char c){return c == '.';});
        
        if(token_list.size() > 1)
        {
            return token_list.back();
        }
        
        return "";
    }
    
    std::string date_time_as_string()
    {
        time_t rawtime;
        struct tm * timeinfo;
        char buffer [80];
        memset(buffer, 0, 80*sizeof(buffer[0]));

        std::time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, 80, "%Y_%m_%d__%H_%M_%S", timeinfo);
        
        return std::string(buffer);
    }
    
}

MatLogger2::MatLogger2(std::string file):
    _file_name(file),
    _mat_file(nullptr),
    _vars_mutex(new MutexImpl)
{
    // get the file extension, or empty string if there is none
    std::string extension = get_file_extension(file);
    
    if(extension == "") // no extension, append date/time + .mat
    {
        _file_name += "__" + date_time_as_string() + ".mat";
    }
    else if(extension != "mat") // extension different from .mat, error
    {
        throw std::invalid_argument("MAT-file name should either have .mat \
extension, or no extension at all");
    }
    
    // create mat file (erase if existing already)
    _mat_file = Mat_CreateVer(_file_name.c_str(), nullptr, MAT_FT_MAT73);
    
    if(!_mat_file)
    {
        throw std::runtime_error("Mat_CreateVer: error in creating file");
    }
}

void MatLogger2::set_on_data_available_callback(VariableBuffer::CallbackType callback)
{
    std::lock_guard<MutexType> lock(_vars_mutex->get());    
    
    for(auto& p : _vars)
    {
        p.second.set_on_block_available(callback);
    }
    
    _on_block_available = callback;
}

bool MatLogger2::create(const std::string& var_name, int rows, int cols, int buffer_size)
{
    
    if(!(rows > 0 && cols > 0 && buffer_size > 0))
    {
        fprintf(stderr, "Unable to create variable '%s': invalid parameters \
(rows=%d, cols=%d, buf_size=%d)\n",
                var_name.c_str(), rows, cols, buffer_size);
        return false;
    }
    
    std::lock_guard<MutexType> lock(_vars_mutex->get());    
    
    // check if variable is already defined (in which case, return false)
    auto it = _vars.find(var_name);
    
    if(it != _vars.end())
    {
        fprintf(stderr, "Variable '%s' already exists\n", var_name.c_str());
        return false;
    }
    
    // compute block size from required buffer_size and number of blocks in
    // queue
    int block_size = buffer_size / VariableBuffer::NUM_BLOCKS;
    
    printf("Created variable '%s' (%d blocks, %d elem each)\n", 
           var_name.c_str(), VariableBuffer::NUM_BLOCKS, block_size);
    
    // insert VariableBuffer object inside the _vars map
    _vars.emplace(std::piecewise_construct,
                  std::forward_as_tuple(var_name),
                  std::forward_as_tuple(var_name, rows, cols, block_size));
    
    // set callback: this will be called whenever a new data block is 
    // available in the variable queue
    _vars.at(var_name).set_on_block_available(_on_block_available);
    
    return true;
}

bool MatLogger2::add(const std::string& var_name, double data)
{
    // search variable
    auto it = _vars.find(var_name);
    
    // check for existance
    if(it == _vars.end())
    {
        fprintf(stderr, "Variable '%s' does not exist\n", var_name.c_str());
        return false;
    }
    
    // turn scalar into a 1x1 matrix, and add it to the buffer
    Eigen::Matrix<double, 1, 1> elem(data);
    it->second.add_elem(elem);
    
    return true;
}


namespace
{
    /**
    * @brief Utility function that creates Mat variable from raw buffers,
    * without copying it
    * 
    * @param data pointer to data
    * @param r rows
    * @param c cols
    * @param s slices
    * @param name name
    * @return pointer to the constructed variable (must be Mat_VarFree-ed by 
the user
    */
    matvar_t* create_var(const double * data, int r, int c, int s, const char* 
name)
    {
        int n_dims = s == 1 ? 2 : 3;
        std::size_t dims[3];
        dims[0] = r;
        dims[1] = c;
        dims[2] = s;

        return Mat_VarCreate(name,
                            MAT_C_DOUBLE,
                            MAT_T_DOUBLE,
                            n_dims,
                            dims,
                            (void *)data,
                            MAT_F_DONT_COPY_DATA);
    }
}

int MatLogger2::flush_available_data()
{
    // number of flushed bytes is returned on exit
    int bytes = 0;
    
    // acquire exclusive access to the _vars object
    std::lock_guard<MutexType> lock(_vars_mutex->get());    
    for(auto& p : _vars)
    {
        Eigen::MatrixXd block;
        int valid_elems = 0;
        
        // while there are blocks available for reading..
        while(p.second.read_block(block, valid_elems))
        {
            // get rows & cols for a single sample
            auto dims = p.second.get_dimension();
            
            int rows = -1;
            int cols = -1;
            int slices = -1;
            bool is_vector = false;
            
            
            if(dims.second == 1) // the variable is a vector
            {
                rows = dims.first;
                cols = valid_elems;
                slices = 1;
                is_vector = true;
            }
            else // the variable is a matrix
            {
                rows = dims.first;
                cols = dims.second;
                slices = valid_elems;
                is_vector = false;
            }
        
            // create mat variable
            matvar_t * var = create_var(block.data(),
                                        rows, cols, slices, 
                                        p.second.get_name().c_str()); 
            
            // if vector we append along columns, otherwise along slices
            int dim_append = is_vector ? 2 : 3;
            int ret = Mat_VarWriteAppend(_mat_file, 
                                         var, 
                                         MAT_COMPRESSION_NONE, 
                                         dim_append); // TBD compression from 
                                                      // user
            
            if(ret != 0)
            {
                fprintf(stderr, "Mat_VarWriteAppend failed with code %d", ret);
            }

            // free mat variable
            Mat_VarFree(var);
            
            // update bytes computation
            bytes += block.rows() * valid_elems * sizeof(double);
        }
    }
    
    return bytes;
}

bool MatLogger2::flush_to_queue_all()
{
    bool ret = true;
    for(auto& p : _vars)
    {
        ret &= p.second.flush_to_queue();
    }
    return ret;
}

MatLogger2::~MatLogger2()
{
    /* inside this constructor, we have the guarantee that the flusher thread
    * (if running) is not using this object
    */
    
    
    printf("%s\n", __PRETTY_FUNCTION__);
    
    // de-register any callback
    set_on_data_available_callback(VariableBuffer::CallbackType());
    
    // flush to queue and then flush to disk till all buffers are empty
    while(!flush_to_queue_all())
    {
        flush_available_data();
    }
    
    // flush to disk remaining data from queues
    while(flush_available_data() > 0);
    
    printf("Flushed all data for file '%s'\n", _file_name.c_str());
    
    // close MAT-file
    Mat_Close(_mat_file);

    _mat_file = nullptr;
}



#define ADD_EXPLICIT_INSTANTIATION(EigenType) \
template bool MatLogger2::add(const std::string&, const Eigen::MatrixBase<EigenType>&); \
template bool VariableBuffer::add_elem(const Eigen::MatrixBase<EigenType>&); \
template bool VariableBuffer::BufferBlock::add(const Eigen::MatrixBase<EigenType>&);

#define ADD_EXPLICIT_INSTANTIATION_STD_VECTOR(Scalar) \
template bool MatLogger2::add(const std::string&, const std::vector<Scalar>&); \


template <typename Scalar>
using Vector6 = Eigen::Matrix<Scalar,6,1>;

template <typename Scalar>
using MapX = Eigen::Map<Eigen::Matrix<Scalar, -1, 1>>;

ADD_EXPLICIT_INSTANTIATION(Eigen::MatrixXd)
ADD_EXPLICIT_INSTANTIATION(Eigen::Matrix2d)
ADD_EXPLICIT_INSTANTIATION(Eigen::Matrix3d)
ADD_EXPLICIT_INSTANTIATION(Eigen::Matrix4d)

ADD_EXPLICIT_INSTANTIATION(Eigen::MatrixXf)
ADD_EXPLICIT_INSTANTIATION(Eigen::Matrix2f)
ADD_EXPLICIT_INSTANTIATION(Eigen::Matrix3f)
ADD_EXPLICIT_INSTANTIATION(Eigen::Matrix4f)

ADD_EXPLICIT_INSTANTIATION(Eigen::VectorXd)
ADD_EXPLICIT_INSTANTIATION(Eigen::Vector2d)
ADD_EXPLICIT_INSTANTIATION(Eigen::Vector3d)
ADD_EXPLICIT_INSTANTIATION(Eigen::Vector4d)
ADD_EXPLICIT_INSTANTIATION(Vector6<double>)

ADD_EXPLICIT_INSTANTIATION(Eigen::VectorXf)
ADD_EXPLICIT_INSTANTIATION(Eigen::Vector2f)
ADD_EXPLICIT_INSTANTIATION(Eigen::Vector3f)
ADD_EXPLICIT_INSTANTIATION(Eigen::Vector4f)
ADD_EXPLICIT_INSTANTIATION(Vector6<float>)

ADD_EXPLICIT_INSTANTIATION(Eigen::VectorXi)
ADD_EXPLICIT_INSTANTIATION(Eigen::Vector2i)
ADD_EXPLICIT_INSTANTIATION(Eigen::Vector3i)
ADD_EXPLICIT_INSTANTIATION(Eigen::Vector4i)
ADD_EXPLICIT_INSTANTIATION(Vector6<int>)

ADD_EXPLICIT_INSTANTIATION_STD_VECTOR(int)
ADD_EXPLICIT_INSTANTIATION_STD_VECTOR(unsigned int)
ADD_EXPLICIT_INSTANTIATION_STD_VECTOR(float)
ADD_EXPLICIT_INSTANTIATION_STD_VECTOR(double)
