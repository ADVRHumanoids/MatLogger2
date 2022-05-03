#include <gtest/gtest.h>

#include <cstdio>
#include <iostream>
#include <signal.h>
#include <chrono>
#include <list>
#include <map>
#include <boost/variant.hpp>

#include "matlogger2/matlogger2.h"
#include "matlogger2/utils/mat_reader.h"
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

     ReadTests(){
         
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

    using namespace XBot::matlogger2;

    int n_rows1 = 23, n_cols1 = 14, n_slices1 = 1;
    
    Eigen::MatrixXd matrix(n_rows1, n_cols1 * n_slices1);
    int count = 0;
    for (int i = 0; i < n_slices1; i++) // initialize matrix with incrementing values, starting from 1
    {
        for (int j = 0; j < n_rows1; j++)
        {
            for (int k = 0; k < n_cols1; k++)
            {
                count++;
                matrix(j, k + i * n_cols1 ) = count;
            }
        }
    }

    auto struct_data = MatData::make_struct();
    struct_data["field_1"] = MatData::make_cell(3);
    struct_data["field_2"] = Eigen::MatrixXd::Identity(5, 8);
    struct_data["field_3"] = MatData::make_struct();

    struct_data["field_1"].asCell() = {1.0,
            "ci\nao",
            Eigen::MatrixXd::Identity(2,5)};

    struct_data["field_3"]["subfield_1"] = 1;
    struct_data["field_3"]["subfield_2"] = 2.0;
    struct_data["field_3"]["subfield_3"] = 3.0;
    struct_data["field_3"]["subfield_4"] = 4.0;

    struct_data.print();

    auto struct_data_cpy = struct_data;

    struct_data_cpy["field_2"].value().as<Eigen::MatrixXd>() = Eigen::Matrix3d::Random();

    int cell_size = 3;
    auto cell_data = MatData::make_cell(cell_size);
    cell_data[0] = Eigen::Vector2d::Random();
    cell_data[1] = Eigen::Vector3d::Random();
    cell_data[2] = Eigen::Vector4d::Random();

    std::string mat_path = "/tmp/readExample.mat";

    auto logger = XBot::MatLogger2::MakeLogger(mat_path);
    logger->save("mvar", struct_data);
    logger->save("cellvar", cell_data);
    logger->add("matrix", matrix);

    logger.reset();

}

TEST_F(ReadTests, read_and_modify)
{
    XBot::MatLogger2::Options opts;
    opts.load_file_from_path = true;

    std::string mat_path = "/tmp/readExample.mat";

    auto logger = XBot::MatLogger2::MakeLogger(mat_path, opts);

    int n_rows1 = 11, n_cols1 = 34, n_slices1 = 1;
    
    Eigen::MatrixXd matrix(n_rows1, n_cols1 * n_slices1);
    int count = 0;
    for (int i = 0; i < n_slices1; i++) // initialize matrix with incrementing values, starting from 1
    {
        for (int j = 0; j < n_rows1; j++)
        {
            for (int k = 0; k < n_cols1; k++)
            {
                count--;
                matrix(j, k + i * n_cols1 ) = count;
            }
        }
    }

    logger->add("new_matrix", matrix);

}

TEST_F(ReadTests, read_and_print)
{
    XBot::MatLogger2::Options opts;
    opts.load_file_from_path = true;

    std::string mat_path = "/tmp/readExample.mat";

    auto logger = XBot::MatLogger2::MakeLogger(mat_path, opts);

}
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}