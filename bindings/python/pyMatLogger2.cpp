#include <matlogger2/matlogger2.h>
#include <matlogger2/utils/mat_appender.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace XBot;

auto construct_matlogger = [](std::string arg, MatLogger2::Options opts)
{

    return MatLogger2::MakeLogger(arg, opts);
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

auto construct_matappender = []()
{
    return MatAppender::MakeInstance();
};

Eigen::MatrixXd readvar(MatLogger2& self, std::string varname)
{
    Eigen::MatrixXd data;
    int slices; // only useful iff reading 3D matrices

    self.readvar(varname, data, slices);

    return data;
}

std::vector<std::string> get_varnames(MatLogger2& self)
{
    std::vector<std::string> varnames;

    self.get_mat_var_names(varnames);

    return varnames;
}

PYBIND11_MODULE(matlogger, m) {
    
    py::enum_<VariableBuffer::Mode>(m, "BufferMode", py::arithmetic())
            .value("CircularBuffer", VariableBuffer::Mode::circular_buffer)
            .value("ProducerConsumer", VariableBuffer::Mode::producer_consumer);

    py::class_<MatLogger2::Options>(m, "Options")
            .def(py::init<>())
            .def_readwrite("enable_compression", &MatLogger2::Options::enable_compression)
            .def_readwrite("load_file_from_path", &MatLogger2::Options::load_file_from_path)
            .def_readwrite("default_buffer_size", &MatLogger2::Options::default_buffer_size)
            .def_readwrite("default_buffer_size_max_bytes", &MatLogger2::Options::default_buffer_size_max_bytes);

    py::class_<MatLogger2, std::shared_ptr<MatLogger2>>(m, "MatLogger2")
            .def(py::init(construct_matlogger),
                 py::arg("file"),
                 py::arg("opts") = MatLogger2::Options())
            .def("getFilename", &MatLogger2::get_filename)
            .def("create", &MatLogger2::create,
                 py::arg("name"),
                 py::arg("rows"),
                 py::arg("cols") = 1,
                 py::arg("buffer_size") = 1000)
            .def("add", add_mat)
            .def("add", add_scalar)
            .def("setBufferMode", &MatLogger2::set_buffer_mode)

            .def("readvar", readvar)
            .def("get_varnames", get_varnames)
            ;

    py::class_<MatAppender, std::shared_ptr<MatAppender>>(m, "MatAppender")
            .def(py::init(construct_matappender))
            .def("add_logger", &MatAppender::add_logger)
            .def("flush_available_data", &MatAppender::flush_available_data)
            .def("start_flush_thread", &MatAppender::start_flush_thread)
            ;


    
}



