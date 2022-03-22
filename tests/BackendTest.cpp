#include "matio_backend.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <iostream>
#include <signal.h>
#include <chrono>
#include <list>
#include <map>
#include <boost/variant.hpp>

// For some basic documentation on Googletest see https://google.github.io/googletest/primer.html

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

class BackendTest: public ::testing::Test {
    

protected:

     BackendTest(){
         
     }

     virtual ~BackendTest() {
     }

     virtual void SetUp() {
         
     }

     virtual void TearDown() {
     }
     
     
};

TEST_F(BackendTest, write_mat_test)
{

    std::unique_ptr<XBot::matlogger2::Backend> _backend;
    std::vector<std::string> var_names;
    std::vector<Eigen::MatrixXd> Mat;

    std::string mat_path = "/tmp/write_test.mat";

    _backend = XBot::matlogger2::Backend::MakeInstance("matio");

    bool is_backend_ok = _backend->init(mat_path, false);

    int count = 0;

    // Creating a 2D matrix in the initialized mat file
    std::string new_var_name1 = "new_matrix";
    int n_rows1 = 2, n_cols1 = 2, n_slices1 = 1;
    
    Eigen::MatrixXd new_var1(n_rows1, n_cols1 * n_slices1);
    
    for (int i = 0; i < n_slices1; i++) // initialize matrix with incrementing values, starting from 1
    {
        for (int j = 0; j < n_rows1; j++)
        {
            for (int k = 0; k < n_cols1; k++)
            {
                count++;
                new_var1(j, k + i * n_cols1 ) = count;
            }
        }
    }

    // Creating a 3D matrix in the initialized mat file
    std::string new_var_name2 = "block_matrix";
    int n_rows2 = 3, n_cols2 = 7, n_slices2 = 5;

    Eigen::MatrixXd new_var2(n_rows2, n_cols2 * n_slices2);
    count = 0;
    for (int i = 0; i < n_slices2; i++) // initialize matrix with incrementing values, starting from 1
    {
        for (int j = 0; j < n_rows2; j++)
        {
            for (int k = 0; k < n_cols2; k++)
            {
                count++;
                new_var2(j, k + i * n_cols2) = count;
            }
        }
    }

    // Creating a 2D vector in the initialized mat file
    std::string new_var_name3 = "vector_row";
    int n_rows3 = 1, n_cols3 = 5, n_slices3 = 1;

    Eigen::MatrixXd new_var3(n_rows3, n_cols3 * n_slices3);
    count = 0;
    for (int i = 0; i < n_slices3; i++) // initialize matrix with incrementing values, starting from 1
    {
        for (int j = 0; j < n_rows3; j++)
        {
            for (int k = 0; k < n_cols3; k++)
            {
                count++;
                new_var3(j, k + i * n_cols3) = count;
            }
        }
    }

    // Creating another 2D vector in the initialized mat file
    std::string new_var_name4 = "vector_col";
    int n_rows4 = 3, n_cols4 = 1, n_slices4 = 1;

    Eigen::MatrixXd new_var4(n_rows4, n_cols4 * n_slices4);
    count = 0;
    for (int i = 0; i < n_slices4; i++) // initialize matrix with incrementing values, starting from 1
    {
        for (int j = 0; j < n_rows4; j++)
        {
            for (int k = 0; k < n_cols4; k++)
            {
                count++;
                new_var4(j, k + i * n_cols4) = count;
            }
        }
    }

    // Creating a structure
    auto struct_data = XBot::matlogger2::MatData::make_struct();
    struct_data["field_1"] = XBot::matlogger2::MatData::make_cell(3);
    struct_data["field_2"] = new_var2;
    struct_data["field_3"] = XBot::matlogger2::MatData::make_struct();

    struct_data["field_1"].asCell() = {1.0,
            "ci\nao",
            Eigen::MatrixXd::Identity(2,5)};
    
    struct_data["field_3"]["subfield_1"] = 1;
    struct_data["field_3"]["subfield_2"] = 2.0;
    struct_data["field_3"]["subfield_3"] = 3.0;
    struct_data["field_3"]["subfield_4"] = 4.0;

    // Creating a cell

    int cell_size = 3;
    auto cell_data = XBot::matlogger2::MatData::make_cell(cell_size);

    cell_data[0] = Eigen::Vector2d::Random();
    cell_data[1] = Eigen::Vector3d::Random();
    cell_data[2] = "prova";


    double time;

    // Writing 2D matrix to file
    uint64_t bytes1 = 0;
    bytes1 = new_var1.size() * sizeof(new_var1[0]);
    
    time = measure_sec
    (
        [&]()
        {
            _backend->write(new_var_name1.c_str(),
                            new_var1.data(),
                            n_rows1, n_cols1, n_slices1);
        }
    );

    std::cout << "Written variable " << new_var_name1 << " (" << bytes1 * 1e-6 << " MB)" << " in " << time << " s" << std::endl;

    // Writing 3D matrix to file
    uint64_t bytes2 = 0;
    bytes2 = new_var2.size() * sizeof(new_var2[0]);

    time = measure_sec
    (
        [&]()
        {
            _backend->write(new_var_name2.c_str(),
                            new_var2.data(),
                            n_rows2, n_cols2, n_slices2);
        }
    );

    std::cout << "Written variable " << new_var_name2 << " (" << bytes2 * 1e-6 << " MB)" << " in " << time << " s" << std::endl;

    // Writing row vector to file
    uint64_t bytes3 = 0;
    bytes3 = new_var3.size() * sizeof(new_var3[0]);

    time = measure_sec
    (
        [&]()
        {
            _backend->write(new_var_name3.c_str(),
                            new_var3.data(),
                            n_rows3, n_cols3, n_slices3);
        }
    );

    std::cout << "Written variable " << new_var_name3 << " (" << bytes3 * 1e-6 << " MB)" << " in " << time << " s" << std::endl;

    // Writing column vector to file
    uint64_t bytes4 = 0;
    bytes4 = new_var4.size() * sizeof(new_var4[0]);

    time = measure_sec
    (
        [&]()
        {
            _backend->write(new_var_name4.c_str(),
                            new_var4.data(),
                            n_rows4, n_cols4, n_slices4);
        }
    );

    std::cout << "Written variable " << new_var_name4 << " (" << bytes4 * 1e-6 << " MB)" << " in " << time << " s" << std::endl;

    std::vector<std::string> matdata_varnames(2);
    matdata_varnames[0] = "structure";
    matdata_varnames[1] = "cell";

    // Writing structure to file
    _backend->write_container("structure", struct_data);

    // Writing cell to file 
    _backend->write_container("cell", cell_data);

    _backend->close();

}

TEST_F(BackendTest, read_variables)
{
    std::unique_ptr<XBot::matlogger2::Backend> _backend;

    typedef Eigen::Map<Eigen::MatrixXd, 0> EigenMap;

    std::vector<std::string> var_names;
    std::vector<Eigen::MatrixXd> Mat;
    XBot::matlogger2::MatData read_var1;
    XBot::matlogger2::MatData read_var2;

    std::string mat_path = "/tmp/write_test.mat";

    _backend = XBot::matlogger2::Backend::MakeInstance("matio");

    bool is_backend_ok = _backend->load(mat_path, true);

    // Printing data in the file
    bool are_varnames_ok = _backend->get_var_names(var_names);

    int n_vars = var_names.size();
    
    std::vector<int> rows(n_vars);
    std::vector<int> cols(n_vars);
    std::vector<int> slices(n_vars);
    std::vector< Eigen::MatrixXd, Eigen::aligned_allocator<Eigen::MatrixXd> > mat_data(n_vars);
    
    std::cout << "--- Standard variable reading test ---" << std::endl;

    int j = 0;
    for (int i = 0; i < n_vars; ++i) // printing info on all variables
    {   
        j = i;
        if ((var_names[j] != "structure") && (var_names[j] != "cell")){ // use the standard readvar method

            bool is_varread_ok = _backend->readvar(var_names[j].c_str(), mat_data[j], slices[j]);
            rows[j] = mat_data[j].rows();
            cols[j] = mat_data[j].cols()/slices[j]; // actual number of columns per slice

            std::cout << "Var. read ok (1 -> ok): " << is_varread_ok << std::endl;
            std::cout << "Variable: " << var_names[j] << "\n" << "dim: " << "("<< rows[j] << ", " << cols[j] << ", " << slices[j] << ")"<< std::endl; 
            
            std::cout << "data: " << mat_data[j] << std::endl;

        }
        
            
    }
    
    std::cout << "--- Struct/Cell variable reading test ---" << std::endl;

    bool is_varread1_ok = _backend->read_container("structure", read_var1);

    std::cout << "struct read ok:" << is_varread1_ok << std::endl;

    // _backend->write_container("struct_copy", read_var1); // works, but memory leak (Conditional jump or move depends on uninitialised value)

    bool is_varread2_ok = _backend->read_container("cell", read_var2);

    std::cout << "cell read ok:" << is_varread2_ok << std::endl;
    
    // _backend->write_container("cell_copy", read_var2); // works, but memory leak (Conditional jump or move depends on uninitialised value)

    _backend->close();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}