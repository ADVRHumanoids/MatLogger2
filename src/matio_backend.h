#ifndef __XBOT_MATLOGGER2_MATIO_BACKEND_H__
#define __XBOT_MATLOGGER2_MATIO_BACKEND_H__

#include <cstdio>

#include "matio.h"
#include "matlogger2_backend.h"

namespace XBot { namespace matlogger2 {
   
    class MatioBackend : public Backend
    {
        
    public:
        
        virtual bool init(std::string logger_name, 
                          bool enable_compression) override;
        
        virtual bool load(std::string matfile_path, 
                          bool enable_write_access) override;

        virtual bool get_var_names(std::vector<std::string>& var_names) override;

        virtual bool write(const char * var_name, const double* data, int rows, int cols, int slices) override;
        
        virtual bool write_container(const char * name, const MatData& data) override;

        virtual bool readvar(const char* var_name, Eigen::MatrixXd& mat_data, int& slices) override;

        virtual bool read_container(const char* var_name, MatData& data) override;

        virtual bool delvar(const char* var_name) override;

        virtual bool get_matpath(const char** matname) override;

        virtual bool close() override;
        
    private:
        
        mat_t * _mat_file;
        matio_compression _compression;
        int _mat_access_mode = -1; // defaults to -1, if no call to load() is used
        const int _max_header_bytes = 128; // maximum .mat header file dimension (bytes)
    };

    
} }



#endif
