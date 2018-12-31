#include <matlogger2/matlogger2.h>
#include <matio-cmake/matio/src/matio.h>
#include <iostream>

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
    if(_current_block.get_valid_elements() == 0)
    {
        return true;
    }
    
    if(!_queue.push(_current_block))
    {
        fprintf(stderr, "Failed to push block for variable '%s'\n", _name.c_str());
        return false;
    }
    
    _current_block.reset();
    
    if(_on_block_available)
    {
        _on_block_available(_current_block.get_size_bytes(), 
                            _queue.write_available()
                            );
    }
    
    return true;
        
}


const std::string& VariableBuffer::get_name() const
{
    return _name;
}



MatLogger2::MatLogger2(std::string file):
    _file_name(file),
    _mat_file(nullptr)
{
    if(!_mat_file)
    {
        _mat_file = Mat_CreateVer(_file_name.c_str(), nullptr, MAT_FT_MAT73);
    }
    
    if(!_mat_file)
    {
        throw std::runtime_error("Mat_CreateVer: error in creating file");
    }
}

void MatLogger2::set_on_data_available_callback(VariableBuffer::CallbackType callback)
{
    
    for(auto& p : _vars)
    {
        p.second.set_on_block_available(callback);
    }
    
    _on_block_available = callback;
}

void MatLogger2::set_on_stop_callback(std::function< void(void) > stop_callback)
{
    _notify_logger_finished = stop_callback;
}


bool MatLogger2::create(const std::string& var_name, int rows, int cols, int buffer_size)
{
    std::lock_guard<std::mutex> lock(_vars_mutex);    
    
    auto it = _vars.find(var_name);
    
    if(it != _vars.end())
    {
        fprintf(stderr, "Variable '%s' already exists\n", var_name.c_str());
        return false;
    }
    
    if(buffer_size <= 0)
    {
        buffer_size = 1e4;
    }
    
    int block_size = buffer_size / VariableBuffer::NUM_BLOCKS;
    
    printf("Created variable '%s' (%d blocks, %d elem each)\n", 
           var_name.c_str(), VariableBuffer::NUM_BLOCKS, block_size);
    
    _vars.emplace(std::piecewise_construct,
                  std::forward_as_tuple(var_name),
                  std::forward_as_tuple(var_name, rows, cols, block_size));
    
    
    _vars.at(var_name).set_on_block_available(_on_block_available);
    
    return true;
}

bool MatLogger2::add(const std::string& var_name, double data)
{
    auto it = _vars.find(var_name);
    
    if(it == _vars.end())
    {
        fprintf(stderr, "Variable '%s' does not exist\n", var_name.c_str());
        return false;
    }
    
    Eigen::Matrix<double, 1, 1> elem(data);
    it->second.add_elem(elem);
    
    return true;
}


namespace
{
    matvar_t* create_var(const double * data, int r, int c, int s, const char* name)
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
    int bytes = 0;
    std::lock_guard<std::mutex> lock(_vars_mutex);    
    for(auto& p : _vars)
    {
        Eigen::MatrixXd block;
        int valid_elems = 0;
        while(p.second.read_block(block, valid_elems))
        {
            auto dims = p.second.get_dimension();
            int rows = -1;
            int cols = -1;
            int slices = -1;
            bool is_vector = false;
            
            if(dims.second == 1)
            {
                rows = dims.first;
                cols = valid_elems;
                slices = 1;
                is_vector = true;
            }
            else
            {
                rows = dims.first;
                cols = dims.second;
                slices = valid_elems;
                is_vector = false;
            }
                
            matvar_t * var = create_var(block.data(),
                                        rows, cols, slices, 
                                        p.second.get_name().c_str()); 
            
            int dim_append = is_vector ? 2 : 3;
            int ret = Mat_VarWriteAppend(_mat_file, var, MAT_COMPRESSION_NONE, dim_append);
            
            if(ret != 0)
            {
                fprintf(stderr, "Mat_VarWriteAppend failed with code %d", ret);
            }

            Mat_VarFree(var);
            
            bytes += block.size() * sizeof(double);
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
    printf("%s\n", __PRETTY_FUNCTION__);
    
    if(_notify_logger_finished)
    {
        _notify_logger_finished();
    }
    
    set_on_data_available_callback(VariableBuffer::CallbackType());
    
    while(!flush_to_queue_all())
    {
        flush_available_data();
    }
    
    while(flush_available_data() > 0);
    
    printf("Flushed all data for file '%s'\n", _file_name.c_str());
    
    Mat_Close(_mat_file);

    _mat_file = nullptr;
}



#define ADD_EXPLICIT_INSTANTIATION(EigenType) \
template bool MatLogger2::add(const std::string&, const Eigen::MatrixBase<EigenType>&); \
template bool MatLogger2::add(const std::string&, const Eigen::Matrix<EigenType::Scalar, EigenType::RowsAtCompileTime, EigenType::ColsAtCompileTime>&); \
template bool VariableBuffer::add_elem(const Eigen::MatrixBase<EigenType>&); \
template bool VariableBuffer::BufferBlock::add(const Eigen::MatrixBase<EigenType>&);

#define ADD_EXPLICIT_INSTANTIATION_MAT(EigenType) \
template bool MatLogger2::add(const std::string&, const Eigen::Matrix<EigenType::Scalar, EigenType::RowsAtCompileTime, EigenType::ColsAtCompileTime>&); \

#define ADD_EXPLICIT_INSTANTIATION_STD_VECTOR(Scalar) \
template bool MatLogger2::add(const std::string&, const std::vector<Scalar>&); \


template <typename Scalar>
using Vector6 = Eigen::Matrix<Scalar,6,1>;

template <typename Scalar>
using MapX = Eigen::Map<Eigen::Matrix<Scalar, -1, 1>>;

ADD_EXPLICIT_INSTANTIATION_MAT(Eigen::MatrixXd)
ADD_EXPLICIT_INSTANTIATION_MAT(Eigen::Matrix2d)
ADD_EXPLICIT_INSTANTIATION_MAT(Eigen::Matrix3d)
ADD_EXPLICIT_INSTANTIATION_MAT(Eigen::Matrix4d)

ADD_EXPLICIT_INSTANTIATION_MAT(Eigen::MatrixXf)
ADD_EXPLICIT_INSTANTIATION_MAT(Eigen::Matrix2f)
ADD_EXPLICIT_INSTANTIATION_MAT(Eigen::Matrix3f)
ADD_EXPLICIT_INSTANTIATION_MAT(Eigen::Matrix4f)

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
