#include "matio_backend.h"

#include <fstream>
#include <stdio.h>
#include <string.h>

inline bool file_exists(const std::string& name) 
{
    std::ifstream f(name.c_str());
    return f.good();
}

using namespace XBot::matlogger2;


extern "C" MATL2_API Backend * create_instance()
{
    return new MatioBackend;
}

bool MatioBackend::init(std::string logger_name, 
                        bool enable_compression)
{
    int err = 0;

    // if file already exists, delete it
    if(file_exists(logger_name))
    {
        if(remove(logger_name.c_str()) != 0)
        {
            fprintf(stderr, 
                    "file '%s' already exists and could not be removed: %s \n",
                    logger_name.c_str(), strerror(errno));

            return false;
        }
    }

    _mat_file = Mat_CreateVer(logger_name.c_str(), 
                              nullptr, 
                              MAT_FT_MAT73);
    
    if ( _mat_file == NULL ) { // check if mat file object is empty

        fprintf(stderr, "MatioBackend::load: Failed to create mat file.\n");

        err++;

        return 0 == err;

    }

    _compression = enable_compression ? MAT_COMPRESSION_ZLIB : MAT_COMPRESSION_NONE;
    
    return bool(_mat_file);
}

bool MatioBackend::load(std::string matfile_path, bool enable_writing_access = false)
{
    _mat_access_mode = enable_writing_access ? MAT_ACC_RDWR : MAT_ACC_RDONLY;
   
    int err = 0;

    _mat_file = Mat_Open(matfile_path.c_str(), _mat_access_mode);

    if ( _mat_file == NULL ) { // empty mat file

        fprintf(stderr, "MatioBackend::load: Failed to load mat file based on the provided path.\n");

        err++;

        return 0 == err;

    }

    if ( MAT_FT_UNDEFINED == Mat_GetVersion(_mat_file) ) { // undefined mat file version

        fprintf(stderr, "MatioBackend::load: Undefined mat file version.\n");

        err++;
    }

    const char *header = Mat_GetHeader(_mat_file);

    if ( NULL != header && strlen(header) > _max_header_bytes ) { // check header

        fprintf(stderr, "MatioBackend::load: Header exists, but exceeds maximum header byte length (%i bytes).\n", _max_header_bytes);

        err++;
    }
    

    return 0 == err;

}

bool MatioBackend::get_var_names(std::vector<std::string>& var_names)
{
    int err = 0;
    size_t n1;

    if ( _mat_file == NULL ) { // check if mat file object exists

        fprintf(stderr, "MatioBackend::get_var_names: Failed to find mat object. Did you remember to call either the init() or load() methods first? \n");

        err++;

        return 0 == err;

    }

    char **names = Mat_GetDir(_mat_file, &n1); // get variables names

    if ( NULL == names ) {

        fprintf(stderr, "MatioBackend::get_var_names: mat object exists, but could not retrieve any variable name.\n");

        err++;

        return 0 == err;

    }

    size_t i, n2 = n1 + 1;

    for ( i = 0; i < n1; ++i ) { // assigning variable names to output vector

        if ( NULL == names[i] ) { // check if one of the names is empty

            err++;

            return 0 == err;

        }

        var_names.push_back(names[i]);

    }

    return 0 == err;
}

bool MatioBackend::write(const char* var_name, const double* data, int rows, int cols, int slices, int append_dim)
{
    int err = 0;
    
    if ( _mat_file == NULL ) { // check if mat file object exists

        fprintf(stderr, "MatioBackend::write: Failed to find mat object. Did you remember to call either the init() or load() methods first? \n");

        err++;

        return 0 == err;

    }

    if ( _mat_access_mode == MAT_ACC_RDONLY){ // check if we are trying to write on a mat file opened in read-only mode

        fprintf(stderr, "MatioBackend::write: Cannot write to a mat file opened in MAT_ACC_RDONLY mode.\n");

        err++;

        return 0 == err;

    }

    if (slices <= 0){ // when adding data, allow only up to 2D data to be appended.

        fprintf(stderr, "MatioBackend::write: Not valid value %d for slices parameter. It must be a strictly positive integer.\n", slices);
        err++;
        return 0 == err;

    }

    std::size_t dims[3];
    int n_dims = slices == 1 ? 2 : 3; 
    dims[0] = rows;
    dims[1] = cols;
    dims[2] = slices;

    // See if the variable already exists
    matvar_t* mat_var_prev = Mat_VarRead(_mat_file, var_name);

    if (mat_var_prev == NULL){ // variable does not exist 
        
        append_dim = n_dims; // overwrites user input, in case it was provided

    }
    else{ // variables already exists
        
        int rows_prev = mat_var_prev->dims[0];
        int cols_prev = mat_var_prev->dims[1];
        int rank_prev = mat_var_prev->rank;

        if (slices > 1){append_dim = 3;} // if the data to be appended is 3D, append it to the third dimension (if compatible)

        // Checks on append_dim:

        if (append_dim != 1 && append_dim != 2 && append_dim != 3){ // append_dim not valid

            fprintf(stderr, "MatioBackend::write: Please provide a valid append direction (provided is %d).\n Allowed values are: 1(rows-wise), 2(column-wise), 3(slice-wise).\n", append_dim);
            err++;
            return 0 == err;

        } 
    
        if (append_dim == 3){ // check input data compatibility for 3rd dimension append operation

            if (!(rows_prev == rows && cols_prev == cols)){

                fprintf(stderr, "MatioBackend::write: Cannot append data to existing variable along 3rd dimension (slice-wise).\n New data has dimensions (%d, %d),  while the preexisting data (%d, %d).\n",
                        rows, rows_prev, cols, cols_prev);
                err++;
                return 0 == err;

            }

            if (rank_prev != 3){ // trying to append 2D data along the 3rd dimension of a previously existing variable with slices = 0. Not allowed by MatIO. 

                fprintf(stderr, "MatioBackend::write: Cannot append new 2D data to already existent 2D variable %s along dimension %d, since it does not have one. Not allowed by MatIO. \n", var_name, append_dim);
                err++;
                return 0 == err;

            }

            n_dims = 3; // trying to append 2D data in the third dimension --> overwriting n_dims(which otherwise would be 2), to allow MatIO to add this data.
        }
        if (append_dim == 2){ // check input data compatibility for 2nd dimension append operation

            if (!(rows_prev == rows)){ // rows do not match

                fprintf(stderr, "MatioBackend::write: Cannot append data to existing variable along 2rd dimension (column-wise).\n New data has (%d) rows, while the preexisting  one has (%d) rows.\n",
                        rows, rows_prev);
                err++;
                return 0 == err;

            }
        }
        if (append_dim == 1){ // check input data compatibility for 1st dimension append operation

            if (!(cols_prev == cols)){

                fprintf(stderr, "MatioBackend::write: Cannot append data to existing variable along 1st dimension (row-wise).\n New data has %d columns,  while the preexisting one has %d columns.\n",
                        cols, cols_prev);
                err++;
                return 0 == err;

            }
        }

        // append_dim is now (reasonably) valid

    }

    // free previous mat variable
    Mat_VarFree(mat_var_prev);

    matvar_t* mat_var = Mat_VarCreate(var_name,
                                MAT_C_DOUBLE,
                                MAT_T_DOUBLE,
                                n_dims,
                                dims,
                                (void *)data,
                                MAT_F_DONT_COPY_DATA);
    
    if ( mat_var == NULL ) { // check if mat file object exists

        fprintf(stderr, "MatioBackend::write: Call to Mat_VarCreate failed. \n");

        err++;

        return 0 == err;

    }

    int ret = Mat_VarWriteAppend(_mat_file, 
                                mat_var, 
                                _compression, 
                                append_dim); // TBD compression from 
                                            // user
    
    if(ret != 0)
    {
        fprintf(stderr,
                "Mat_VarWriteAppend failed with code %d "
                "while writing variable '%s' (%d x %d x %d) \n",
                ret, var_name, rows, cols, slices);
        err++;
        return 0 == err;
    }

    // free mat variable
    Mat_VarFree(mat_var); 

    return 0 == err;
    
}

bool MatioBackend::readvar(const char* var_name, double** data, int& rows, int& cols, int& slices)
{
    int err = 0;  
    
    if ( _mat_file == NULL ) { // check if mat file object exists

        fprintf(stderr, "MatioBackend::readvar: Failed to find mat object. Did you remember to call either the init() or load() methods first? \n");
        err++;
        return 0 == err;

    }

    matvar_t* mat_var = Mat_VarRead(_mat_file, var_name);

    if ( mat_var == NULL ) { // variable empty (reading failed)

        fprintf(stderr, "MatioBackend::readvar: Failed to read the required variable. Check that you have provided a valid variable name. \n");
        err++;
        return 0 == err;
    }

    if ( mat_var->data == NULL ) { // empty data

        fprintf(stderr, "MatioBackend::readvar: Variable read, but empty data field. \n");
        err++;
        return 0 == err;
    }

    *data = (double*) mat_var->data;
    rows = mat_var->dims[0];
    cols = mat_var->dims[1];
    int rank = mat_var->rank;
    
    if (rank != 3){ // 2D data
        slices = 1; // MatIO assigns slices = 0 for 2D data; here setting it to 1 to avoid issues when using this number externally.
        return 0 == err;
    }

    slices = mat_var->dims[2];

    // free mat variable
    Mat_VarFree(mat_var); // causes memory leak for some reason

    return 0 == err;
    
}

bool MatioBackend::delvar(const char* var_name)
{
    int err = 0;

    if ( _mat_file == NULL ) { // check if mat file object exists

        fprintf(stderr, "MatioBackend::delvar: Failed to find mat object. Did you remember to call either the init() or load() methods first? \n");
        err++;
        return 0 == err;

    }

    int ret = Mat_VarDelete(_mat_file, var_name);

    if(ret != 0) // removal operation failed
    {

        fprintf(stderr, "MatioBackend::delvar: Mat_VarDelete failed with code %d \n", ret);
        err++;
        return 0 == err;

    }

    return 0 == err;
}

bool MatioBackend::get_matpath(const char** matname)
{   
    int err = 0;

    if ( _mat_file == NULL ) { // check if mat file object exists

        fprintf(stderr, "MatioBackend::get_matpath: Failed to find mat object. Did you remember to call either the init() or load() methods first? \n");
        err++;
        return 0 == err;

    }

    *matname = Mat_GetFilename(_mat_file);

    if ( matname == NULL ) {

        fprintf(stderr, "MatioBackend::get_matpath: Mat_GetFilename failed to retrieve the file name.\n");
        err++;
        return 0 == err;

    }
    
    return 0 == err;
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

    matvar_t* outercell = Mat_VarCreate(name.c_str(),
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

    return elem_matvar;

}

bool XBot::matlogger2::MatioBackend::write_container(const char * name,
                                                     const MatData & data)
{

    matvar_t * mat_var = make_matvar(name, data);

    auto ret = Mat_VarWrite(_mat_file, mat_var, _compression);

    Mat_VarFree(mat_var);

    return ret == 0;
}

