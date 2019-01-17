#ifndef __XBOT_MATLOGGER2_MATIO_BACKEND_H__
#define __XBOT_MATLOGGER2_MATIO_BACKEND_H__

#include <cstdio>

#include <matio-cmake/matio/src/matio.h>
#include "matlogger2_backend.h"

namespace XBot { namespace matlogger2 {
   
    class MatioBackend : public Backend
    {
        
    public:
        
        virtual bool init(std::string logger_name, 
                          bool enable_compression) override;
        
        virtual bool write(const char * name,
                   const double * data, 
                   int rows, 
                   int cols, 
                   int slices) override;
        
        virtual bool close() override;
        
    private:
        
        mat_t * _mat_file;
        matio_compression _compression;
    };
    
} }



#endif
