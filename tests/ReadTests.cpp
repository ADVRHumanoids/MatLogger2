#include <gtest/gtest.h>

#include <cstdio>
#include <iostream>
#include <signal.h>
#include <chrono>
#include <list>
#include <map>
#include <boost/variant.hpp>

#include "matlogger2/matlogger2.h"
#include "matlogger2/mat_data.h"

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

class ReadTests: public ::testing::Test {

protected:

    int n_rows_mat1 = 23, n_cols_mat1 = 14, n_slices_mat1 = 1, n_slices_block_mat = 5; // just some variables useful to check the correctness of reading operations
    int n_rows_mat2 = 11, n_cols_mat2 = 34, n_slices_mat2 = 1; 

    std::string mat_path = "/tmp/readExample.mat";
    std::string matrix_name = std::string("matrix"), matrix2_name = std::string("matrix2"), block_mat_name = "block_mat",
                matrix_tb_deleted_name = std::string("matrix_tb_deleted"), block_matrix_name = std::string("block_matrix"), 
                cell_data_name = std::string("cell_data_name"), struct_data_name = std::string("struct_var"); 
    
    Eigen::MatrixXd matrix, matrix2, matrix_tb_deleted, block_matrix_check;

    XBot::matlogger2::MatData struct_data;
    XBot::matlogger2::MatData cell_data;

    ReadTests(){
        
        // first matrix
        matrix = Eigen::MatrixXd(this->n_rows_mat1, this->n_cols_mat1 * this->n_slices_mat1);
        int count = 0;
        for (int i = 0; i < this->n_slices_mat1; i++) // initialize matrix with incrementing values, starting from 1
        {
            for (int j = 0; j < this->n_rows_mat1; j++)
            {
                for (int k = 0; k < this->n_cols_mat1; k++)
                {
                    count++;
                    matrix(j, k + i * this->n_cols_mat1) = count;
                }
            }
        }

        // second matrix (to test adding variables after loading mat file)
        matrix2 = Eigen::MatrixXd(this->n_rows_mat2, this->n_cols_mat2 * this->n_slices_mat2);
        int count2 = 0;
        for (int i = 0; i < this->n_slices_mat2; i++) // initialize matrix with incrementing values, starting from 1
        {
            for (int j = 0; j < this->n_rows_mat2; j++)
            {
                for (int k = 0; k < this->n_cols_mat2; k++)
                {
                    count2--;
                    matrix2(j, k + i * this->n_cols_mat2 ) = count2;
                }
            }
        }

        // auxiliary block matrix used to check the correct writing and reading of block matrix

        block_matrix_check = Eigen::MatrixXd(this->n_rows_mat1, this->n_cols_mat1 * this->n_slices_block_mat);
        for (int i = 0; i < this->n_slices_block_mat; i++) // initialize matrix with incrementing values, starting from 1
        {
            for (int j = 0; j < this->n_rows_mat1; j++)
            {
                for (int k = 0; k < this->n_cols_mat1; k++)
                {
                    block_matrix_check(j, k + i * this->n_cols_mat1 ) = matrix(j, k);
                }
            }
        }

        // structure
        struct_data = XBot::matlogger2::MatData::make_struct();
        struct_data["field_1"] = XBot::matlogger2::MatData::make_cell(3);
        struct_data["field_2"] = Eigen::MatrixXd::Identity(5, 8);
        struct_data["field_3"] = XBot::matlogger2::MatData::make_struct();
        struct_data["field_1"].asCell() = {1.0,
                "ci\nao",
                Eigen::MatrixXd::Identity(2,5)};
        struct_data["field_3"]["subfield_1"] = 1;
        struct_data["field_3"]["subfield_2"] = 2.4;
        struct_data["field_3"]["subfield_3"] = 3.1;
        struct_data["field_3"]["subfield_4"] = 4.9;

        // cell
        int cell_size = 3;
        cell_data = XBot::matlogger2::MatData::make_cell(cell_size);
        Eigen::Vector2d first_el; 
        first_el << 1.0, 4.0;
        Eigen::Vector3d second_el;
        second_el << 9.0, 1.67, 4.147;
        Eigen::Vector4d third_el;
        third_el << 6.7, 1.1245, 8.7665, 343.7;

        cell_data[0] = first_el;
        cell_data[1] = second_el;
        cell_data[2] = third_el;
        // cell_data[0] = Eigen::Vector2d::Random();
        // cell_data[1] = Eigen::Vector3d::Random();
        // cell_data[2] = Eigen::Vector4d::Random();

    }

    virtual ~ReadTests() {
    }

    virtual void SetUp() {
        
    }

    virtual void TearDown() {
    }
     
     
};

TEST_F(ReadTests, dump_sample_file)
{
    XBot::MatLogger2::Options opt;
    opt.default_buffer_size = 1e6;

    remove(this->mat_path.c_str()); // remove test file if already existing

    using namespace XBot::matlogger2;

    auto logger = XBot::MatLogger2::MakeLogger(this->mat_path, opt);
    ASSERT_TRUE(logger->save(this->struct_data_name, this->struct_data));
    ASSERT_TRUE(logger->save(this->cell_data_name, this->cell_data));

    ASSERT_TRUE(logger->add(matrix_name, this->matrix));

    for (int i = 0; i < this->n_slices_block_mat; i++)
    {
        ASSERT_TRUE(logger->add(this->block_mat_name, matrix));
    }
    logger.reset();

}

//TEST_F(ReadTests, load_and_modify)
//{
//    XBot::MatLogger2::Options opts;
//    opts.load_file_from_path = true;
//    opts.default_buffer_size = 1e6;

//    auto logger = XBot::MatLogger2::MakeLogger(this->mat_path, opts);

//    // even if reading, we need to call the create methot to set
//    // the buffer size to the desired one

//    // appending to existing mat
//    ASSERT_TRUE(logger->create(this->matrix2_name,
//                            this->n_rows_mat2, this->n_cols_mat2 * this->n_slices_mat2,
//                            opts.default_buffer_size));

//    ASSERT_TRUE(logger->add(this->matrix2_name, this->matrix2));

//    // creating matrix which will be deleted
//    ASSERT_TRUE(logger->create(this->matrix_tb_deleted_name,
//                            this->n_rows_mat2, this->n_cols_mat2 * this->n_slices_mat2,
//                            opts.default_buffer_size));
//    ASSERT_TRUE(logger->add(this->matrix_tb_deleted_name, this->matrix2));

//    logger.reset();

//}

//TEST_F(ReadTests, read_var_names)
//{
//    XBot::MatLogger2::Options opts;
//    opts.load_file_from_path = true;

//    std::string mat_path = "/tmp/readExample.mat";

//    auto logger = XBot::MatLogger2::MakeLogger(mat_path, opts);

//    std::vector<std::string> var_names;

//    ASSERT_TRUE(logger->get_mat_var_names(var_names));

//    int n_vars = var_names.size();

//    logger.reset();

//    int j = 0;
//    for (int j = 0; j < n_vars; ++j) // printing info on all variables
//    {

//        std::cout << "Variable: " << var_names[j] << "\n";
                
//    }
//}

//TEST_F(ReadTests, delete_var)
//{
//    XBot::MatLogger2::Options opts;
//    opts.load_file_from_path = true;

//    std::string mat_path = "/tmp/readExample.mat";

//    auto logger = XBot::MatLogger2::MakeLogger(mat_path, opts);

//    ASSERT_TRUE(logger->delvar(this->matrix_tb_deleted_name));

//    logger.reset();

//}

//TEST_F(ReadTests, check_var_deleted_ok)
//{
//    XBot::MatLogger2::Options opts;
//    opts.load_file_from_path = true;

//    std::string mat_path = "/tmp/readExample.mat";

//    auto logger = XBot::MatLogger2::MakeLogger(mat_path, opts);

//    std::vector<std::string> var_names;

//    ASSERT_TRUE(logger->get_mat_var_names(var_names));

//    int n_vars = var_names.size();

//    logger.reset();

//    int j = 0;
//    bool was_var_del_ok = true;
//    for (int j = 0; j < n_vars; ++j) // printing info on all variables
//    {

//        std::cout << "Variable: " << var_names[j] << "\n";
        
//        if (var_names[j] == this->matrix_tb_deleted_name)
//        {
//            was_var_del_ok = false;
//        }
                
//    }

//    ASSERT_TRUE(was_var_del_ok);
//}

//TEST_F(ReadTests, read_and_print)
//{
//    XBot::MatLogger2::Options opts;
//    opts.load_file_from_path = true;

//    std::string mat_path = "/tmp/readExample.mat";

//    auto logger = XBot::MatLogger2::MakeLogger(mat_path, opts);

//    Eigen::MatrixXd matrix, matrix2, block_matrix;
//    int slices;

//    ASSERT_TRUE(logger->readvar(matrix_name, matrix, slices));
//    std::cout << "\n" << "read " << this->matrix_name.c_str() << ": "<< "\n" << matrix << std::endl;
//    std::cout << "n. slices: " << slices << std::endl;
//    ASSERT_TRUE(matrix.rows() == this->n_rows_mat1);
//    ASSERT_TRUE(matrix.cols() == this->n_cols_mat1 * this->n_slices_mat1);
//    ASSERT_TRUE(matrix.isApprox(this->matrix));

//    ASSERT_TRUE(logger->readvar(this->matrix2_name, matrix2, slices));
//    std::cout << "\n" << "read " << this->matrix2_name.c_str() << ": " << "\n" << matrix2 << std::endl;
//    std::cout << "n. slices: " << slices << std::endl;
//    ASSERT_TRUE(matrix2.rows() == this->n_rows_mat2);
//    ASSERT_TRUE(matrix2.cols() == this->n_cols_mat2 * this->n_slices_mat2);
//    ASSERT_TRUE(matrix2.isApprox(this->matrix2));
    
//    ASSERT_TRUE(logger->readvar("block_matrix", block_matrix, slices));
//    std::cout << "\n" << "read " << this->block_matrix_name.c_str() << ": " << "\n"  << block_matrix << std::endl;
//    std::cout << "n. slices: " << slices << std::endl;
//    ASSERT_TRUE(block_matrix.rows() == this->n_rows_mat1);
//    ASSERT_TRUE(block_matrix.cols() == this->n_cols_mat1 * this->n_slices_block_mat);
//    ASSERT_TRUE(block_matrix.isApprox(this->block_matrix_check));

//    XBot::matlogger2::MatData struct_data, cell_data;

//    ASSERT_TRUE(logger->read_container(this->struct_data_name, struct_data));
//    std::cout << "\n" << "read " << this->struct_data_name.c_str() << ": " << std::endl;
//    struct_data.print();
//    // std::cout << struct_data["field_1"] << std::endl;
//    ASSERT_TRUE(struct_data["field_1"][0].value().as<Eigen::MatrixXd>()(0, 0) == this->struct_data["field_1"][0].value().as<double>()); // upon reading, doubles are converted to MatrixXd (MatIO does not make any difference)
//    ASSERT_TRUE(struct_data["field_1"][1].value().as<std::string>() == this->struct_data["field_1"][1].value().as<std::string>());
//    ASSERT_TRUE(struct_data["field_1"][2].value().as<Eigen::MatrixXd>().isApprox(this->struct_data["field_1"][2].value().as<Eigen::MatrixXd>()));
//    ASSERT_TRUE(struct_data["field_2"].value().as<Eigen::MatrixXd>().isApprox(this->struct_data["field_2"].value().as<Eigen::MatrixXd>()));
//    ASSERT_TRUE(struct_data["field_3"]["subfield_1"].value().as<Eigen::MatrixXd>()(0, 0) == this->struct_data["field_3"]["subfield_1"].value().as<double>());
//    ASSERT_TRUE(struct_data["field_3"]["subfield_2"].value().as<Eigen::MatrixXd>()(0, 0) == this->struct_data["field_3"]["subfield_2"].value().as<double>());
//    ASSERT_TRUE(struct_data["field_3"]["subfield_3"].value().as<Eigen::MatrixXd>()(0, 0) == this->struct_data["field_3"]["subfield_3"].value().as<double>());
//    ASSERT_TRUE(struct_data["field_3"]["subfield_4"].value().as<Eigen::MatrixXd>()(0, 0) == this->struct_data["field_3"]["subfield_4"].value().as<double>());

//    ASSERT_TRUE(logger->read_container(this->cell_data_name, cell_data));
//    std::cout << "\n" << "read " << this->cell_data_name.c_str() << ": " << std::endl;
//    cell_data.print();
//    ASSERT_TRUE(this->cell_data[0].value().as<Eigen::MatrixXd>().isApprox(cell_data[0].value().as<Eigen::MatrixXd>()));
//    ASSERT_TRUE(this->cell_data[1].value().as<Eigen::MatrixXd>().isApprox(cell_data[1].value().as<Eigen::MatrixXd>()));
//    ASSERT_TRUE(this->cell_data[2].value().as<Eigen::MatrixXd>().isApprox(cell_data[2].value().as<Eigen::MatrixXd>()));

//    logger.reset();

//}

//TEST_F(ReadTests, check_throws)
//{
//    // Checking exceptions with structure data
//    EXPECT_THROW(struct_data["field_1"].value().as<std::string>(), std::exception);
//    EXPECT_THROW(struct_data["field_2"].value().as<double>(), std::exception);
//    EXPECT_THROW(struct_data["field_1"].value().as<double>(), std::exception);
//    EXPECT_THROW(struct_data["field_1"][0].value().as<Eigen::MatrixXd>(), std::exception);

//    // Checking exceptions with cell data
    
//    EXPECT_THROW(cell_data[0].value().as<std::string>(), std::exception);
//    EXPECT_THROW(cell_data[0].value().as<double>(), std::exception);

//}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
