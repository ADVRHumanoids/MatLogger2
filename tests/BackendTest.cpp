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

    int n_rows1 = 10, n_cols1 = 10, n_slices1 = 1;
    int n_rows2 = 3, n_cols2 = 7, n_slices2 = 5;
    int n_rows3 = 1, n_cols3 = 5, n_slices3 = 1;
    int n_rows4 = 10, n_cols4 = 1, n_slices4 = 1;

    std::string new_var_name1 = "new_matrix";
    std::string new_var_name2 = "block_matrix";
    std::string new_var_name3 = "vector_row";
    std::string new_var_name4 = "vector_col";
    std::string new_var_name5 = "append_test_mat";
    std::string new_var_name6= "lon_vect_test_mat";

    std::string stndrd_wrt_test_path = "/tmp/write_test1.mat";
    std::string cellstruct_test_path = "/tmp/write_test2.mat";
    std::string append_test_path = "/tmp/write_test3.mat";
    std::string append_long_vect_path = "/tmp/write_test4.mat";

    Eigen::MatrixXd new_var1, new_var2, new_var3, new_var4;

    XBot::matlogger2::MatData struct_data;

    int cell_size = 3;
    XBot::matlogger2::MatData cell_data;

    BackendTest(){

      // Initializing all variables to be written to file

      int count = 0;
      // Creating a 2D matrix in the initialized mat file
      this->new_var1 = Eigen::MatrixXd(this->n_rows1, this->n_cols1 * this->n_slices1);

      for (int i = 0; i < this->n_slices1; i++) // initialize matrix with incrementing values, starting from 1
      {
          for (int j = 0; j < this->n_rows1; j++)
          {
              for (int k = 0; k < this->n_cols1; k++)
              {
                  count++;
                  this->new_var1(j, k + i * this->n_cols1 ) = count;
              }
          }
      }

      // Creating a 3D matrix in the initialized mat file

      this->new_var2 = Eigen::MatrixXd(this->n_rows2, this->n_cols2 * this->n_slices2);
      count = 0;
      for (int i = 0; i < this->n_slices2; i++) // initialize matrix with incrementing values, starting from 1
      {
          for (int j = 0; j < this->n_rows2; j++)
          {
              for (int k = 0; k < this->n_cols2; k++)
              {
                  count++;
                  this->new_var2(j, k + i * this->n_cols2) = count;
              }
          }
      }

      // Creating a 2D vector in the initialized mat file

      this->new_var3 = Eigen::MatrixXd(this->n_rows3, this->n_cols3 * this->n_slices3);
      count = 0;
      for (int i = 0; i < this->n_slices3; i++) // initialize matrix with incrementing values, starting from 1
      {
          for (int j = 0; j < this->n_rows3; j++)
          {
              for (int k = 0; k < this->n_cols3; k++)
              {
                  count++;
                  this->new_var3(j, k + i * this->n_cols3) = count;
              }
          }
      }

      // Creating another 2D vector in the initialized mat file

      this->new_var4 = Eigen::MatrixXd(this->n_rows4, this->n_cols4 * this->n_slices4);
      count = 0;
      for (int i = 0; i < this->n_slices4; i++) // initialize matrix with incrementing values, starting from 1
      {
          for (int j = 0; j < this->n_rows4; j++)
          {
              for (int k = 0; k < this->n_cols4; k++)
              {
                  count++;
                  this->new_var4(j, k + i * this->n_cols4) = count;
              }
          }
      }

      // Creating a structure
      this->struct_data = XBot::matlogger2::MatData::make_struct();
      this->struct_data["field_1"] = XBot::matlogger2::MatData::make_cell(3);
      this->struct_data["field_2"] = Eigen::Vector4d::Random();
      this->struct_data["field_3"] = XBot::matlogger2::MatData::make_struct();

      this->struct_data["field_1"].asCell() = {1.0,
              "ci\nao",
              Eigen::MatrixXd::Identity(2,5)};

      this->struct_data["field_3"]["subfield_1"] = 1;
      this->struct_data["field_3"]["subfield_2"] = 2.0;
      this->struct_data["field_3"]["subfield_3"] = 3.0;
      this->struct_data["field_3"]["subfield_4"] = 4.0;

      // Creating a cell

      this->cell_data = XBot::matlogger2::MatData::make_cell(this->cell_size);

      this->cell_data[0] = Eigen::Vector2d::Random();
      this->cell_data[1] = Eigen::Vector3d::Random();
      this->cell_data[2] = "test";

    }

    virtual ~BackendTest() {
    }

    virtual void SetUp() {

    }

    virtual void TearDown() {
    }
     
     
};

TEST_F(BackendTest, write_standard_mat_test)
{

    std::unique_ptr<XBot::matlogger2::Backend> _backend;

    _backend = XBot::matlogger2::Backend::MakeInstance("matio");

    ASSERT_TRUE(_backend->init(this->stndrd_wrt_test_path, false));

    double time;

    // Writing 2D matrix to file
    uint64_t bytes1 = 0;
    bytes1 = this->new_var1.size() * sizeof(this->new_var1[0]);
    
    time = measure_sec
    (
        [&]()
        {
            ASSERT_TRUE(_backend->write(this->new_var_name1.c_str(),
                            this->new_var1.data(),
                            this->n_rows1, this->n_cols1, this->n_slices1));
        }
    );

    std::cout << "Written variable " << this->new_var_name1 << " (" << bytes1 * 1e-6 << " MB)" << " in " << time << " s" << std::endl;

    // Writing 3D matrix to file
    uint64_t bytes2 = 0;
    bytes2 = this->new_var2.size() * sizeof(this->new_var2[0]);

    time = measure_sec
    (
        [&]()
        {
            ASSERT_TRUE(_backend->write(this->new_var_name2.c_str(),
                            this->new_var2.data(),
                            this->n_rows2, this->n_cols2, this->n_slices2));
        }
    );

    std::cout << "Written variable " << this->new_var_name2 << " (" << bytes2 * 1e-6 << " MB)" << " in " << time << " s" << std::endl;

    // Writing row vector to file
    uint64_t bytes3 = 0;
    bytes3 = this->new_var3.size() * sizeof(this->new_var3[0]);

    time = measure_sec
    (
        [&]()
        {
            ASSERT_TRUE(_backend->write(this->new_var_name3.c_str(),
                            this->new_var3.data(),
                            this->n_rows3, this->n_cols3, this->n_slices3));
        }
    );

    std::cout << "Written variable " << this->new_var_name3 << " (" << bytes3 * 1e-6 << " MB)" << " in " << time << " s" << std::endl;

    // Writing column vector to file
    uint64_t bytes4 = 0;
    bytes4 = this->new_var4.size() * sizeof(this->new_var4[0]);

    time = measure_sec
    (
        [&]()
        {
            ASSERT_TRUE(_backend->write(this->new_var_name4.c_str(),
                            this->new_var4.data(),
                            this->n_rows4, this->n_cols4, this->n_slices4));
        }
    );

    std::cout << "Written variable " << this->new_var_name4 << " (" << bytes4 * 1e-6 << " MB)" << " in " << time << " s" << std::endl;

    _backend->close();

}

TEST_F(BackendTest, write_cell_struct_mat_test)
{

    std::unique_ptr<XBot::matlogger2::Backend> _backend;

    _backend = XBot::matlogger2::Backend::MakeInstance("matio");

    ASSERT_TRUE(_backend->init(this->cellstruct_test_path, false));

    // Writing structure to file
    ASSERT_TRUE(_backend->write_container("structure", this->struct_data));

    // Writing cell to file 
    ASSERT_TRUE(_backend->write_container("cell", this->cell_data));

    _backend->close();

}

TEST_F(BackendTest, read_standard_variables)
{
    std::unique_ptr<XBot::matlogger2::Backend> _backend;

    typedef Eigen::Map<Eigen::MatrixXd, 0> EigenMap;

    std::vector<std::string> var_names;

    _backend = XBot::matlogger2::Backend::MakeInstance("matio");

    ASSERT_TRUE(_backend->load(this->stndrd_wrt_test_path, true));

    // Printing data in the file
    ASSERT_TRUE(_backend->get_var_names(var_names));

    int n_vars = var_names.size();
    
    std::vector<int> rows(n_vars);
    std::vector<int> cols(n_vars);
    std::vector<int> slices(n_vars);
    std::vector< Eigen::MatrixXd, Eigen::aligned_allocator<Eigen::MatrixXd> > mat_data(n_vars);
    
    std::cout << "--- Standard variable reading test ---" << std::endl;

    int j = 0;
    for (int j = 0; j < n_vars; ++j) // printing info on all variables
    {   

        ASSERT_TRUE(_backend->readvar(var_names[j].c_str(), mat_data[j], slices[j]));
        rows[j] = mat_data[j].rows();
        cols[j] = mat_data[j].cols()/slices[j]; // actual number of columns per slice

        std::cout << "Variable: " << var_names[j] << "\n" << "dim: " << "("<< rows[j] << ", " << cols[j] << ", " << slices[j] << ")"<< std::endl; 
        
        std::cout << "data: " << mat_data[j] << std::endl;
            
    }
    
    _backend->close();
}

TEST_F(BackendTest, read_cell_struct_variables)
{

    std::string mat_path = "/tmp/write_test2.mat";

    std::unique_ptr<XBot::matlogger2::Backend> _backend;

    _backend = XBot::matlogger2::Backend::MakeInstance("matio");

    bool is_backend_ok = _backend->load(mat_path, true);


    std::vector<std::string> var_names;
    bool are_varnames_ok = _backend->get_var_names(var_names);
    int n_vars = var_names.size();

    XBot::matlogger2::MatData read_var1;
    XBot::matlogger2::MatData read_var2;

    std::cout << "--- Struct/Cell variable reading test ---" << std::endl;

    bool is_varread1_ok = _backend->read_container("structure", read_var1);

    std::cout << "\n struct read ok:" << is_varread1_ok << std::endl;

    read_var1.print();

    // _backend->write_container("struct_copy", read_var1); // works, but memory leak (Conditional jump or move depends on uninitialised value)

    bool is_varread2_ok = _backend->read_container("cell", read_var2);

    std::cout << "\n cell read ok:" << is_varread2_ok << std::endl;
    
    // _backend->write_container("cell_copy", read_var2); // works, but memory leak (Conditional jump or move depends on uninitialised value)

    read_var2.print();

    _backend->close();
}

TEST_F(BackendTest, append_to_existing_data)
{

    std::unique_ptr<XBot::matlogger2::Backend> _backend;

    _backend = XBot::matlogger2::Backend::MakeInstance("matio");

    ASSERT_TRUE(_backend->init(this->append_test_path, false));

    // Writing 2D matrix to file

    // writing matrix to file
    ASSERT_TRUE(_backend->write(this->new_var_name1.c_str(),
                    this->new_var1.data(),
                    this->n_rows1, this->n_cols1, this->n_slices1));
    // adding a column vector
    ASSERT_TRUE(_backend->write(this->new_var_name1.c_str(),
                    this->new_var4.data(),
                    this->n_rows4, this->n_cols4, this->n_slices1));
    // adding a copy of the original matrix
    ASSERT_TRUE(_backend->write(this->new_var_name1.c_str(),
                    this->new_var1.data(),
                    this->n_rows1, this->n_cols1, this->n_slices1));

    _backend->close();

}

TEST_F(BackendTest, checkHugeVarDump)
{
  std::unique_ptr<XBot::matlogger2::Backend> _backend;

  _backend = XBot::matlogger2::Backend::MakeInstance("matio");

  ASSERT_TRUE(_backend->init(this->append_long_vect_path, false));

  size_t vect_length = 1e5;
  const size_t MB_SIZE = 1e6;

  std::cout << "Dumping long vector of approximately " << ((double)vect_length) * ((double)sizeof(double)) / ((double)MB_SIZE) << " MB" << std::endl;

  for(int i = 0; i < (vect_length); i++)
  {
      double v = i;
      ASSERT_TRUE(_backend->write(this->new_var_name6.c_str(),
                      &v,
                      1, 1, 1));

  }

  std::cout << "closing backend \n";

  _backend->close();

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
