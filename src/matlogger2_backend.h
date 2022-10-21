#ifndef __XBOT_MATLOGGER2_BACKEND_H__
#define __XBOT_MATLOGGER2_BACKEND_H__

#include <string>
#include <memory>

#include <vector>

#include "matlogger2/mat_data.h"

#include "Eigen/Dense"

namespace XBot { namespace matlogger2 {
   
    class MATL2_API Backend
    {
        
    public:
        
        typedef std::unique_ptr<Backend> UniquePtr;
        typedef std::shared_ptr<Backend> Ptr;
        
        static UniquePtr MakeInstance(std::string type);
        
        virtual bool init(std::string logger_name, 
                          bool enable_compression
                          ) = 0;
        
        virtual bool load(std::string matfile_path, 
                          bool enable_write_access = false) = 0;

        virtual bool get_var_names(std::vector<std::string>& var_names) = 0;

        virtual bool write(const char* var_name, 
                           const double* data, 
                           int rows, int cols, 
                           int slices) = 0;

        virtual bool write_container(const char* name,
                           const MatData& data);

        virtual bool readvar(const char* var_name, 
                            Eigen::MatrixXd& mat_data,
                            int& slices) = 0;

        virtual bool read_container(const char* var_name, 
                                    MatData& data);

        virtual bool delvar(const char* var_name) = 0;

        virtual bool get_matpath(const char** matname) =  0;
        
        virtual bool close() = 0;
        
        virtual ~Backend() = default;
        
        
    };
    
} }

#endif
