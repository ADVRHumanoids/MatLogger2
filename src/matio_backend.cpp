#include "matio_backend.h"

using namespace XBot::matlogger2;


extern "C" MATL2_API Backend * create_instance()
{
    return new MatioBackend;
}

bool MatioBackend::init(std::string logger_name, 
                        bool enable_compression)
{
    _mat_file = Mat_CreateVer(logger_name.c_str(), 
                              nullptr, 
                              MAT_FT_MAT73);
    
    _compression = enable_compression ? MAT_COMPRESSION_ZLIB : MAT_COMPRESSION_NONE;
    
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
                                 _compression, 
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

matvar_t * make_matvar(const std::string& name, const MatData& matdata);

struct create_matvar_visitor : boost::static_visitor<matvar_t *>
{
    std::string _name;

    matvar_t * operator()(const Eigen::MatrixXd& mat)
    {
        const int mat_rank = 2;
        std::size_t dims[mat_rank];
        dims[0] = mat.rows();
        dims[1] = mat.cols();

        auto * mat_var = Mat_VarCreate(_name.c_str(),
                                       MAT_C_DOUBLE,
                                       MAT_T_DOUBLE,
                                       mat_rank,
                                       dims,
                                       (void *)mat.data(),
                                       MAT_F_DONT_COPY_DATA);

        return mat_var;
    }

    matvar_t * operator()(const std::string& text)
    {
        const int mat_rank = 2;
        std::size_t dims[mat_rank];
        dims[0] = 1;
        dims[1] = text.size();

        auto * mat_var = Mat_VarCreate(_name.c_str(),
                                       MAT_C_CHAR,
                                       MAT_T_UTF8,
                                       mat_rank,
                                       dims,
                                       (void *)text.data(),
                                       MAT_F_DONT_COPY_DATA);

        return mat_var;
    }

    matvar_t * operator()(double value)
    {
        const int mat_rank = 2;
        std::size_t dims[mat_rank];
        dims[0] = 1;
        dims[1] = 1;

        auto * mat_var = Mat_VarCreate(_name.c_str(),
                                       MAT_C_DOUBLE,
                                       MAT_T_DOUBLE,
                                       mat_rank,
                                       dims,
                                       (void *)&value,
                                       0);

        return mat_var;
    }
};

matvar_t * make_scalar_matvar(const std::string& name, const MatData& scalar_matdata)
{
    create_matvar_visitor visitor;
    visitor._name = name;
    return boost::apply_visitor(visitor, scalar_matdata.value());
}

matvar_t * make_struct_matvar(const std::string& name, const MatData& struct_matdata)
{
    std::vector<const char *> field_names;

    for(auto& elem : struct_matdata.asStruct())
    {
        field_names.push_back(elem.first.c_str());
    }

    // create outer struct
    const int struct_rank = 2;
    size_t struct_dim[struct_rank] = {1, 1}; // create scalar struct
    matvar_t * mat_struct = Mat_VarCreateStruct(name.c_str(),
                                               struct_rank,
                                               struct_dim,
                                               field_names.data(),
                                               field_names.size());

    // iterate over fields
    for(auto& elem : struct_matdata.asStruct())
    {
        auto elem_matvar = make_matvar(elem.first, elem.second);

        Mat_VarSetStructFieldByName(mat_struct,
                                    elem.first.c_str(),
                                    0, // note: only support scalar structs
                                    elem_matvar);
    }


    return mat_struct;

}

matvar_t * make_cell_matvar(const std::string& name, const MatData& cell_matdata)
{
    size_t cell_dim[2] = {cell_matdata.asCell().size(), 1};

    matvar_t* outercell = Mat_VarCreate(NULL,
                                        MAT_C_CELL,
                                        MAT_T_CELL,
                                        2,
                                        cell_dim,
                                        NULL,
                                        0);

    int idx = 0;
    for(auto& elem : cell_matdata.asCell())
    {
        matvar_t * elem_matvar = make_matvar("", elem);
        Mat_VarSetCell(outercell, idx, elem_matvar); // set struct into outer cell
        idx++;
    }

    return outercell;
}

matvar_t * make_matvar(const std::string& name, const MatData& matdata)
{
    matvar_t * elem_matvar = nullptr;

    if(matdata.is_scalar())
    {
        elem_matvar = make_scalar_matvar(name, matdata);
    }
    else if(matdata.is_struct())
    {
        elem_matvar = make_struct_matvar(name, matdata);
    }
    else if(matdata.is_cell())
    {
        elem_matvar = make_cell_matvar(name, matdata);
    }

}

bool XBot::matlogger2::MatioBackend::write_container(const char * name,
                                                     const MatData & data)
{

    matvar_t * mat_var = make_matvar(name, data);

    auto ret = Mat_VarWrite(_mat_file, mat_var, _compression);

    Mat_VarFree(mat_var);

    return ret == 0;
}

