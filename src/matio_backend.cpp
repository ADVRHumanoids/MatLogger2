#include "matio_backend.h"

#include <fstream>
#include <stdio.h>
#include <string.h>

#include <Eigen/Dense>

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

        fprintf(stderr, "MatioBackend::init: Failed to create mat file.\n");

        err++;

        return 0 == err;

    }

    _compression = enable_compression ? MAT_COMPRESSION_ZLIB : MAT_COMPRESSION_NONE;
    
    return bool(_mat_file);
} // Initialize backend, given a name for the mat file.

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

} // Load an already existent mat file inside the backend

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
} // Retrieve all variable names 

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
} // Method for retrieving the absolute path of the mat file loaded in the current instance of the backend

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
    matvar_t* mat_var = Mat_VarRead(_mat_file, var_name);

    if (mat_var == NULL){ // variable does not exist 

        mat_var = Mat_VarCreate(var_name,
                                MAT_C_DOUBLE,
                                MAT_T_DOUBLE,
                                n_dims,
                                dims,
                                (void *)data,
                                MAT_F_DONT_COPY_DATA); // create variable and assign a pointer to it

        if ( mat_var == NULL ) { // creation of variable failed

            fprintf(stderr, "MatioBackend::write: Call to Mat_VarCreate failed. \n");

            err++;

            // free pointer to mat variable
            Mat_VarFree(mat_var);

            return 0 == err;

        }

        append_dim = n_dims; // overwrites user input, in case it was provided

    }
    else{ // variables already exists
        
        int rows_prev = mat_var->dims[0];
        int cols_prev = mat_var->dims[1];
        int rank_prev = mat_var->rank;

        if (append_dim != 1 && append_dim != 2 && append_dim != 3){ // append_dim not valid

            fprintf(stderr, "MatioBackend::write: Please provide a valid append direction (provided is %d).\n Allowed values are: 1(rows-wise), 2(column-wise), 3(slice-wise).\n", append_dim);
            
            err++;

            // free pointer to previous mat variable
            Mat_VarFree(mat_var);
            
            return 0 == err;

        } 

        if (slices > 1){ append_dim = 3; } // if the data to be appended is 3D, append it (if possible) to the third dimension. 

        // At this point, the program will enter one of the following 3 if blocks, based on the desired append direction

        if (append_dim == 3){ // check rows and cols compatibility for 3rd dimension append operation

            if (!(rows_prev == rows && cols_prev == cols)){

                fprintf(stderr, "MatioBackend::write: Cannot append data to existing variable along 3rd dimension (slice-wise).\n New data has slice dimensions (%d, %d),  while the preexisting data (%d, %d).\n",
                        rows, rows_prev, cols, cols_prev);
                
                err++;

                // free pointer to previous mat variable
                Mat_VarFree(mat_var);

                return 0 == err;

            }

            if (rank_prev != 3){ 

                fprintf(stderr, "MatioBackend::write: Cannot append new 2D data to already existent 2D variable %s along dimension %d, since it does not have one. Not allowed by MatIO. \n", var_name, append_dim);
               
                err++;

                // free pointer to previous mat variable
                Mat_VarFree(mat_var);

                return 0 == err;

            }

            n_dims = 3; // trying to append data in the third dimension --> overwriting n_dims, to allow MatIO to add this data also in case of input 2D data.
        }

        if (append_dim == 2){ // check input data compatibility for 2nd dimension append operation

            if (!(rows_prev == rows)){ // rows do not match

                fprintf(stderr, "MatioBackend::write: Cannot append data to existing variable along 2rd dimension (column-wise).\n New data has (%d) rows, while the preexisting  one has (%d) rows.\n",
                        rows, rows_prev);
                
                err++;
                
                // free pointer to previous mat variable
                Mat_VarFree(mat_var);

                return 0 == err;

            }
        }

        if (append_dim == 1){ // check input data compatibility for 1st dimension append operation

            if (!(cols_prev == cols)){

                fprintf(stderr, "MatioBackend::write: Cannot append data to existing variable along 1st dimension (row-wise).\n New data has %d columns,  while the preexisting one has %d columns.\n",
                        cols, cols_prev);
                
                err++;

                // free pointer to previous mat variable
                Mat_VarFree(mat_var);

                return 0 == err;

            }
        }

        // After these checks, the input data is appendable

    }
    
    int ret = Mat_VarWriteAppend(_mat_file, 
                                mat_var, 
                                _compression, 
                                append_dim); // TBD compression from 
                                            // user
    
    // free pointer to mat variable
    Mat_VarFree(mat_var); 

    if(ret != 0)
    {
        fprintf(stderr,
                "Mat_VarWriteAppend failed with code %d "
                "while writing variable '%s' (%d x %d x %d) \n",
                ret, var_name, rows, cols, slices);

        err++;

        return 0 == err;
    }

    return 0 == err;
    
} // Method for writing/appending basic numeric variables to file (i.e. matrices)

bool MatioBackend::readvar(const char* var_name, Eigen::MatrixXd& mat_data, int& slices)
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

        Mat_VarFree(mat_var);

        return 0 == err;
    }

    if ( mat_var->data == NULL ) { // empty data

        fprintf(stderr, "MatioBackend::readvar: Variable read, but empty data field. \n");
        
        err++;

        Mat_VarFree(mat_var);

        return 0 == err;
    }

    // int data_size = mat_var->data_size;
    // memmove(*data, const void* mat_var->data, data_size * mat_var->dims[0] * mat_var->dims[1]); // copying data field to memory pointed by the output data pointer to avoid losing data upon variable deletion
    
    typedef Eigen::Map<Eigen::MatrixXd> EigenMap;

    int rows = mat_var->dims[0];
    int cols = mat_var->dims[1];
    int rank = mat_var->rank;
    
    // mat_var->mem_conserve = 1;// this allows to remove all memory associated with the variable, except for the data field

    slices = rank != 3 ? 1 : mat_var->dims[2];

    mat_data = EigenMap((double*) mat_var->data, rows, cols * (slices)); // mapping variable data to an Eigen Matrix

    Mat_VarFree(mat_var); // free all the memory allocated for the variable

    return 0 == err;
    
} // Method for reading basic numeric variable (i.e. matrices)

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
} // Method for deleting a specific variable

bool MatioBackend::close()
{
    return 0 == Mat_Close(_mat_file);
}

//////////////// Methods for container writing (parsing of a MatData into a matvar_t object) ///////////////////

matvar_t* make_matvar(const std::string& name, const MatData& matdata); // forward declaration

struct create_matvar_visitor : boost::static_visitor< matvar_t* >
{ 
    std::string _name;

    matvar_t* operator()(const Eigen::MatrixXd& mat)
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

    matvar_t* operator()(const std::string& text)
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

    matvar_t* operator()(double value)
    {
        const int mat_rank = 2;
        std::size_t dims[mat_rank];
        dims[0] = 1;
        dims[1] = 1;

        auto* mat_var = Mat_VarCreate(_name.c_str(),
                                       MAT_C_DOUBLE,
                                       MAT_T_DOUBLE,
                                       mat_rank,
                                       dims,
                                       (void *)&value,
                                       0);

        return mat_var;
    }
}; // Visitor for creating MatIO variables based on their base scalar type (MatrixXd, string, double). Interfaces directly with MatIO

matvar_t* make_scalar_matvar(const std::string& name, const MatData& scalar_matdata)
{

    create_matvar_visitor visitor;
    visitor._name = name;
    return boost::apply_visitor(visitor, scalar_matdata.value());

} // The recursive call to make_cell_matvar and make_struct_matvar methods 
// will eventually decay into a make_scalar_matvar() call and break the recursion.
// make_scalar_matvar assigns data to the output matvar_t object, based on the type of scalar_matdata.

matvar_t* make_struct_matvar(const std::string& name, const MatData& struct_matdata)
{

    std::vector<const char *> field_names;

    for(auto& elem : struct_matdata.asStruct())
    {
        field_names.push_back(elem.first.c_str());
    }

    // create outer struct
    const int struct_rank = 2;
    size_t struct_dim[struct_rank] = {1, 1}; // create scalar struct
    matvar_t* mat_struct = Mat_VarCreateStruct(name.c_str(),
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

} // Creates the outer shell of the structure, and then calls make_matvar recursively for each element
// until the bottom data layer is reached. At that point, make_scalar_matvar is called and the MatIO final matvar_t* is returned.

matvar_t* make_cell_matvar(const std::string& name, const MatData& cell_matdata)
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

} // Creates the outer cell and than calls recursively make_matvar for each element of the cell until the bottom data layer is reached.
// At that point, make_scalar_matvar is called and the MatIO matvar_t* is returned.

matvar_t* make_matvar(const std::string& name, const MatData& matdata)
{

    matvar_t* elem_matvar = nullptr;

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

} // Calls the right assembly method based on the input data type

bool MatioBackend::write_container(const char* name, const MatData& data)
{   
    int err = 0;

    if ( _mat_file == NULL ) { // check if mat file object exists

        fprintf(stderr, "MatioBackend::read_container: Failed to find mat object. Did you remember to call either the init() or load() methods first? \n");
        
        err++;

        return 0 == err;

    }

    matvar_t* mat_var = make_matvar(name, data);

    auto ret = Mat_VarWrite(_mat_file, mat_var, _compression);

    Mat_VarFree(mat_var);

    return ret == 0;

} // Based on the input matdata, builds recursively the associated MatIO variable and then writes it to file.

//////////////// Methods for container reading (parsing of a matvar_t into a MatData object) ///////////////////

bool make_matdata(const matvar_t* mat_var, MatData& matdata); // forward declaration

bool make_scalar_matdata(const matvar_t* scalar_mat_var, MatData& matdata)
{
    int err = 0;

    if (scalar_mat_var->class_type == MAT_C_DOUBLE) // double (matrix or single value)
    {
        
        typedef Eigen::Map<Eigen::MatrixXd> EigenMap;

        int rows = scalar_mat_var->dims[0];
        int cols = scalar_mat_var->dims[1];
        int rank = scalar_mat_var->rank;
        int slices = rank != 3 ? 1 : scalar_mat_var->dims[2];

        matdata = EigenMap((double*) scalar_mat_var->data, rows, cols * slices); // mapping variable data to an Eigen Matrix

    }
    else if(scalar_mat_var->class_type == MAT_C_CHAR) // character array
    {
        if (scalar_mat_var->dims[0] != 1) 
        {

            fprintf(stderr, "MatioBackend::make_scalar_matdata: array of chars not supported yet.\n");

            err++;

            return 0 == err; 
            
        }
        
        std::string strng;

        strng.assign( (char*) scalar_mat_var->data, scalar_mat_var->dims[1]);

        matdata = strng;
        
    }
    else
    {
        fprintf(stderr, "MatioBackend::make_scalar_matdata: The provided data has (yet) unsupported MatIO type %d. \n", scalar_mat_var->class_type);

        err++;

    }

    return 0 == err;

}// The recursive call to make_cell_matdata and make_struct_matdata methods 
// will eventually decay into a make_scalar_matdata() call and break the recursion.
// make_scalar_matdata assigns data to the provided MatData object, based on the type of scalar_mat_var.

bool make_cell_matdata(const matvar_t* matio_cell_array, MatData& matdata)
{

    int err = 0;

    int cell_dim = matio_cell_array->dims[0] != 1  ? matio_cell_array->dims[0]: matio_cell_array->dims[1]; // extracts the relevant cell array dimension

    matdata = XBot::matlogger2::MatData::make_cell(cell_dim);
    
    for (int cell_index = 0; cell_index < cell_dim; cell_index++)
    {
        matvar_t* cell = Mat_VarGetCell(const_cast<matvar_t*>(matio_cell_array), cell_index);

        bool make_data_ok = make_matdata(cell, matdata[cell_index]);

        make_data_ok ? : err++; 
        
    }

    return 0 == err; 

}// Creates the outer cell and than calls recursively make_matdata for each element until the bottom data layer is reached.
// At that point, make_scalar_matdata is called and provided matdata is fully populated.

bool make_struct_matdata(const matvar_t* matio_structure, MatData& matdata)
{
    
    int err = 0;

    std::vector<const char *> field_names;

    int n_fields = Mat_VarGetNumberOfFields(const_cast<matvar_t*>(matio_structure)); 

    char *const * f_names =  Mat_VarGetStructFieldnames(matio_structure); 

    for (int struct_index = 0; struct_index < n_fields; struct_index++)
    {
        field_names.push_back(f_names[struct_index]);
    }

    matdata = XBot::matlogger2::MatData::make_struct();
    
    for(int i = 0; i < n_fields; i++)
    {
        matvar_t* struct_field = Mat_VarGetStructFieldByName(const_cast<matvar_t*>(matio_structure), field_names[i], 0);

        bool make_data_ok = make_matdata(struct_field, matdata[field_names[i]]);

        make_data_ok ? : err++; 
    }

    return 0 == err; 

}// Creates the outer structure and than calls recursively make_matdata for each element until the bottom data layer is reached.
// At that point, make_scalar_matdata is called and provided matdata is fully populated.

bool make_matdata(const matvar_t* mat_var, MatData& matdata)
{
    int err = 0;

    if(mat_var->class_type == MAT_C_CELL) // cell array
    {
        size_t* cell_dims = mat_var->dims;
        
        if (!(cell_dims[0] == 1 || cell_dims[1] == 1)) // cell dimension check
        {
            fprintf(stderr, "MatioBackend::make_matdata: Only up to one dimensional cell arrays are currently supported. \n");
        
            err++;

        }

        bool make_cell_ok = make_cell_matdata(mat_var, matdata);

        make_cell_ok ? : err++; 

    }
    else if(mat_var->class_type == MAT_C_STRUCT) // structure
    {
        
        bool make_struct_ok = make_struct_matdata(mat_var, matdata);

        make_struct_ok ? : err++;

    }
    else if(mat_var->class_type == MAT_C_DOUBLE || mat_var->class_type == MAT_C_CHAR) // equivalent of the MatData "scalar" types
    {
        
        bool make_scalar_ok = make_scalar_matdata(mat_var, matdata);

        make_scalar_ok ? : err++;

    }
    else
    {
        fprintf(stderr, "MatioBackend::make_matdata: The provided data has (yet) unsupported MatIO type %d. \n", mat_var->class_type);

        err++;

    }

    return 0 == err;

} // Calls the right MatData assembly method based on the input data type

bool MatioBackend::read_container(const char* var_name, MatData& matdata)
{
    int err = 0;  
    
    if ( _mat_file == NULL ) { // check if mat file object exists

        fprintf(stderr, "MatioBackend::read_container: Failed to find mat object. Did you remember to call either the init() or load() methods first? \n");
        
        err++;

        return 0 == err;

    }

    matvar_t* mat_var = Mat_VarRead(_mat_file, var_name);

    if ( mat_var == NULL ) { // variable empty (reading failed)

        fprintf(stderr, "MatioBackend::read_container: Failed to read the required variable. Check that you have provided a valid variable name. \n");
        
        err++;

        Mat_VarFree(mat_var);

        return 0 == err;
    }

    if ( mat_var->data == NULL ) { // empty data

        fprintf(stderr, "MatioBackend::read_container: Variable read, but empty data field. \n");
        
        err++;

        Mat_VarFree(mat_var);

        return 0 == err;
    }

    bool make_matdata_ok = make_matdata(mat_var, matdata); // parse mat_var into matdata (navigating recursively into the variable, from the outer shell towards the base data layer)

    make_matdata_ok ? : err++;

    Mat_VarFree(mat_var); // frees all the memory used by the MatIO variable

    return err == 0;

} // Based on the input var_name, read the variable from the loaded at file and
//  Builds recursively the associated MatData object.