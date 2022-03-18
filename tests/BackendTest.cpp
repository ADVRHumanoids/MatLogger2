#include <matlogger2/matlogger2.h>

#include <cstdio>

#include "matio_backend.h"

#include <iostream>

#include <Eigen/StdVector>

using namespace XBot::matlogger2;

int main()
{
    std::unique_ptr<Backend> _backend;
    std::vector<std::string> var_names;
    std::vector<Eigen::MatrixXd> Mat;

    std::string mat_path = "/tmp/new_log.mat";

    _backend = Backend::MakeInstance("matio");
    // bool is_backend_ok = _backend->init(mat_path, false);
    bool is_backend_ok = _backend->load(mat_path, true);

    // Adding a new variable to the loaded mat file
    std::string new_var_name = "new_var";
    int n_rows = 2, n_cols = 2, n_slices = 1;
    int append_dim = 3;
    Eigen::MatrixXd new_var(n_rows, n_cols * n_slices);

    int count = 0;
    for (int i = 0; i < n_slices; i++) // initialize matrix with incrementing values
    {
        for (int j = 0; j < n_rows; j++)
        {
            for (int k = 0; k < n_cols; k++)
            {
                count++;
                new_var(j, k + i * n_cols ) = count;
            }
        }
    }
    
    bool is_newvar_written_ok = _backend->write(new_var_name.c_str(),
                    new_var.data(),
                    n_rows, n_cols, n_slices, append_dim);
                    

    // Printing info on the data
    bool are_varnames_ok = _backend->get_var_names(var_names);

    int n_vars = var_names.size();
    
    std::vector<double*> data(n_vars);
    std::vector<int> rows(n_vars);
    std::vector<int> cols(n_vars);
    std::vector<int> slices(n_vars); 


    for (int i = 0; i < n_vars; ++i) // printing info on all variables
    {
        bool is_varread_ok = _backend->readvar(var_names[i].c_str(), &(data[i]), rows[i], cols[i], slices[i]);

        std::cout << "Var. read ok: " << is_varread_ok << std::endl;
        std::cout << "Variable: " << var_names[i] << "\n" << "dim: " << "("<< rows[i] << ", " << cols[i] << ", " << slices[i] << ")"<< std::endl; 

        Mat.push_back(Eigen::Map<Eigen::MatrixXd>(data[i], rows[i], cols[i] * (slices[i])));
        
        std::cout << "data: " << Mat[i] << std::endl;

    }


    // const char* cell[2] = {"field1", "field2"};
    // Removing the variable from the loaded mat file
    // bool is_newvar_removed_ok = _backend->delvar(new_var_name.c_str());

    // Checking method for getting mat file name
    // const char* matname;
    // _backend->get_matpath(&matname);
    // std::cout << "mat file name: " << matname << std::endl;

    _backend->close();
    
}