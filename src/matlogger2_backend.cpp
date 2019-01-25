/* Part of this code is taken from ADVRHumanoids/XBotInterface, SoLib.h
 * 
 * Copyright (C) 2017 IIT-ADVR
 * Author: Giuseppe Rigano
 * email:  giuseppe.rigano@iit.it
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
*/



#include <map>
#include <string>
#include <iostream>
#include <cstdio>
#include <dlfcn.h>

#include <boost/algorithm/string.hpp>

#include "matlogger2_backend.h"


namespace
{
    
template <class T>
static std::unique_ptr<T> GetFactory(const std::string& lib_name) 
{

    char * error;
    void * lib_handle;
    
    lib_handle = dlopen(lib_name.c_str(), RTLD_NOW);
    
    if(!lib_handle)
    {
        fprintf(stderr, "%s\n", dlerror());
        return nullptr;
    } 
    else 
    {

        T*(*create)();
        create = (T*(*)()) dlsym(lib_handle, "create_instance");
        if((error = dlerror()) != NULL)
        {
            fprintf(stderr, "Error in loading library '%s':\n%s", lib_name.c_str(), error);
            return nullptr;
        }

        T* instance = ( T* ) create();
        if(instance != nullptr) 
        {
            return std::unique_ptr<T>(instance);
        }
        
        fprintf(stderr, "Error in loading library '%s': obtained pointer is null\n", lib_name.c_str());

    }

    return nullptr;

    }

}

class DummyBackend : public XBot::matlogger2::Backend
{
public:
  
virtual bool close(){return true;}
virtual bool init(std::string logger_name, bool compression){return true;}
virtual bool write(const char* name, const double* data, int rows, int cols, int slices){return true;}
    
};

XBot::matlogger2::Backend::UniquePtr XBot::matlogger2::Backend::MakeInstance(std::string type)
{
    if(type == "dummy")
    {
        return std::make_unique<DummyBackend>();
    }
    
    return GetFactory<Backend>("libmatlogger2-backend-" + type + MATLOGGER2_LIB_EXT);
}


