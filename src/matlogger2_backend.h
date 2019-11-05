#ifndef __XBOT_MATLOGGER2_BACKEND_H__
#define __XBOT_MATLOGGER2_BACKEND_H__

#include <string>
#include <memory>

#include <matlogger2/mat_data.h>

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
        
        virtual bool write(const char * name,
                           const double * data, 
                           int rows, 
                           int cols, 
                           int slices) = 0;

        virtual bool write_container(const char * name,
                           const MatData& data);
        
        virtual bool close() = 0;
        
        virtual ~Backend() = default;
        
        
    };
    
} }

#endif
