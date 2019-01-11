#include "matio_backend.h"

using namespace XBot::matlogger2;

extern "C" Backend * create_instance()
{
    return new MatioBackend;
}

bool MatioBackend::init(std::string logger_name)
{
    _mat_file = Mat_CreateVer(logger_name.c_str(), 
                              nullptr, 
                              MAT_FT_MAT73);
    
    return _mat_file;
}

bool MatioBackend::write(const char* name, const double* data, int rows, int cols, int slices)
{
    int n_dims = slices == 1 ? 2 : 3;
    std::size_t dims[3];
    dims[0] = rows;
    dims[1] = cols;
    dims[2] = slices;

    auto * mat_var = Mat_VarCreate(name,
                                   MAT_C_DOUBLE,
                                   MAT_T_DOUBLE,
                                   n_dims,
                                   dims,
                                   (void *)data,
                                   MAT_F_DONT_COPY_DATA);
    
    int dim_append = slices == 1 ? 2 : 3;
    int ret = Mat_VarWriteAppend(_mat_file, 
                                 mat_var, 
                                 MAT_COMPRESSION_NONE, 
                                 dim_append); // TBD compression from 
                                              // user
    
    if(ret != 0)
    {
        fprintf(stderr, "Mat_VarWriteAppend failed with code %d \n", ret);
    }

    // free mat variable
    Mat_VarFree(mat_var);
    
    return ret == 0;
    
}

bool MatioBackend::close()
{
    return 0 == Mat_Close(_mat_file);
}

