#include "matlogger2/mat_data.h"

namespace XBot {
namespace matlogger2 {

using detail::MatDataBase;

class StructData : public MatDataBase
{

public:

    virtual std::string type() const override { return "struct"; }

    virtual bool is_struct() const override { return true; }

    virtual std::map<std::string, MatData>& getStruct() override { return _data; }

    virtual MatDataBase::UniquePtr clone() const override { return UniquePtr(new StructData(*this)); }

    void print(std::ostream & os) const override;

private:

    std::map<std::string, MatData> _data;

};


class CellData : public MatDataBase
{

public:

    virtual std::string type() const override { return "cell"; }

    virtual bool is_cell() const override { return true; }

    virtual std::vector<MatData>& getCell() override { return _data; }

    virtual MatDataBase::UniquePtr clone() const override { return UniquePtr(new CellData(*this)); }

    void print(std::ostream & os) const override;

private:

    std::vector<MatData> _data;

};

class ScalarData : public MatDataBase
{

public:

    virtual std::string type() const override { return "scalar"; }

    virtual bool is_scalar() const override { return true; }

    virtual MatScalarType& getScalar() override { return _data; }

    virtual MatDataBase::UniquePtr clone() const override { return UniquePtr(new ScalarData(*this)); }

    void print(std::ostream & os) const override;

private:

    MatScalarType _data;

};

/* IMPL */

bool MatData::is_struct() const
{
    return _data_ptr->is_struct();
}

bool MatData::is_cell() const
{
    return _data_ptr->is_cell();
}

bool MatData::is_scalar() const
{
    return _data_ptr->is_scalar();
}

MatScalarType & MatData::value()
{
    return _data_ptr->getScalar();
}

std::map<std::string, MatData> & MatData::asStruct()
{
    return _data_ptr->getStruct();
}

std::vector<MatData> & MatData::asCell()
{
    return _data_ptr->getCell();
}

const MatScalarType & MatData::value() const
{
    return _data_ptr->getScalar();
}

const std::map<std::string, MatData> & MatData::asStruct() const
{
    return _data_ptr->getStruct();
}

const std::vector<MatData> & MatData::asCell() const
{
    return _data_ptr->getCell();
}

MatData & MatData::operator[] (int i)
{
    return asCell()[i];
}

const MatData & MatData::operator[] (int i) const
{ 
    return asCell()[i];
}

MatData & MatData::operator[] (const std::string& key)
{
    return asStruct()[key];
}

const MatData & MatData::operator[] (const std::string& key) const
{
    return asStruct().at(key);
}

MatData::MatData()
{
    _data_ptr.reset(new ScalarData);
    value() = 0.0;
}

MatData::MatData(const MatData & other)
{
    _data_ptr = other._data_ptr->clone();
}

MatData& MatData::operator= (const MatData& rhs)
{
    if(this == &rhs)
    {
        return *this;
    }

    _data_ptr.reset();
    _data_ptr = rhs._data_ptr->clone();

    return *this;
}

MatData MatData::make_struct()
{
    MatData data;
    data._data_ptr.reset(new StructData);

    return data;
}


MatData MatData::make_cell(int size)
{
    MatData data;
    data._data_ptr.reset(new CellData);

    data.asCell().resize(size);

    return data;
}

MatDataBase * MatData::make_scalar()
{
    return new ScalarData;
}

void MatData::print(std::ostream & os) const
{
    _data_ptr->print(os);
}

MatData::~MatData()
{

}

void ScalarData::print(std::ostream & os) const
{
    os << _data;
}

void CellData::print(std::ostream & os) const
{
    for(const auto& elem: _data)
    {
        os << "\n - ";
        elem.print(os);
    }
}

void StructData::print(std::ostream & os) const
{
    for(const auto& elem: _data)
    {
        os << "\n";
        os << elem.first << ": ";
        elem.second.print(os);
    }
}

bool MatDataBase::is_struct() const { return false; }

bool MatDataBase::is_cell() const { return false; }

bool MatDataBase::is_scalar() const { return false; }

MatScalarType& MatDataBase::getScalar() { throw bad_type("scalar", type()); }

std::map<std::string, MatData>& MatDataBase::getStruct() { throw bad_type("struct", type()); }

std::vector<MatData>& MatDataBase::getCell() { throw bad_type("cell", type()); }

bad_type::bad_type(std::string req, std::string actual)
{
    this->req = req;
    this->actual = actual;
}

const char* bad_type::what() const noexcept
{
    msg = "Requested type '" + req + "' does not match the actual type '" + actual + "'";
    return msg.c_str();
}

}
}
