#include <matlogger2/matlogger2.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>

namespace py = pybind11;
using namespace XBot;

auto construct_matlogger = [](std::string arg, bool compress, int buf_size)
{
    MatLogger2::Options opt;
    opt.default_buffer_size = buf_size;
    opt.enable_compression = compress;
    return MatLogger2::MakeLogger(arg, opt);
};

void add_mat(MatLogger2& self, const std::string& name, const Eigen::MatrixXd& var)
{
    if(!self.add(name, var))
    {
        throw std::invalid_argument("Illegal name or value");
    }
}

void add_scalar(MatLogger2& self, const std::string& name, double var)
{
    if(!self.add(name, var))
    {
        throw std::invalid_argument("Illegal name or value");
    }
}

PYBIND11_MODULE(matlogger, m) {
    
    py::enum_<VariableBuffer::Mode>(m, "BufferMode", py::arithmetic())
        .value("CircularBuffer", VariableBuffer::Mode::circular_buffer)
        .value("ProducerConsumer", VariableBuffer::Mode::producer_consumer);
        
    py::class_<MatLogger2>(m, "MatLogger2")
        .def(py::init(construct_matlogger), 
             py::arg("file"), 
             py::arg("enable_compression") = false, 
             py::arg("default_buffer_size") = MatLogger2::Options().default_buffer_size)
        .def("getFilename", &MatLogger2::get_filename)
        .def("create", &MatLogger2::create, 
             py::arg("name"), 
             py::arg("rows"), 
             py::arg("cols") = 1, 
             py::arg("buffer_size") = 1000)
        .def("add", add_mat)
        .def("add", add_scalar)
        .def("setBufferMode", &MatLogger2::set_buffer_mode)
        ;
    
}



