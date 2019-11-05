#ifndef __XBOT_MATLOGGER2_MAT_DATA_H__
#define __XBOT_MATLOGGER2_MAT_DATA_H__

#include <map>
#include <vector>
#include <iostream>
#include <boost/variant.hpp>
#include <Eigen/Dense>

namespace XBot
{
namespace matlogger2
{

namespace detail {

    typedef  boost::variant<std::string,
                            double,
                            Eigen::MatrixXd> __MatScalarBaseType;

    class MatDataBase;

}

class MatScalarType : public detail::__MatScalarBaseType
{

public:

    using detail::__MatScalarBaseType::__MatScalarBaseType;

    template <typename T>
    T& as()
    {
        return boost::get<T>(*this);
    }

};


class MatData
{

public:

    /* Default constructor (double type, value = 0) */
    MatData();

    /* Construct scalar element from type convertible to either
            - double
            - std::strung
            - Eigen::MatrixXd
    */
    template <typename T = double>
    MatData(const T& value);

    /* Copy constructor (deep copy) */
    MatData(const MatData& other);
    MatData(MatData&& other) = default;

    /* Copy assignment (deep copy) */
    MatData& operator=(const MatData& rhs);
    MatData& operator=(MatData&& rhs) = default;

    /* Factories for struct and cell types (for scalar, use constructor) */
    static MatData make_struct();
    static MatData make_cell(int size = 0);

    /* Type checkers */
    bool is_struct() const;
    bool is_cell() const;
    bool is_scalar() const;

    /* Getters to underlying types (throw on wrong type) */
    MatScalarType& value();
    std::map<std::string, MatData>& asStruct();
    std::vector<MatData>& asCell();

    const MatScalarType& value() const;
    const std::map<std::string, MatData>& asStruct() const;
    const std::vector<MatData>& asCell() const;

    /* Direct access to cell elements */
    MatData& operator[](int i);
    const MatData& operator[](int i) const;

    /* Direct access to struct elements */
    MatData& operator[](const std::string& key);
    const MatData& operator[](const std::string& key) const;


    /* Print to ostream object */
    void print(std::ostream& os = std::cout) const;

    /* Destructor */
    ~MatData();

private:

    static detail::MatDataBase * make_scalar();

    std::unique_ptr<detail::MatDataBase> _data_ptr;

};

class detail::MatDataBase
{

public:

    typedef std::unique_ptr<MatDataBase> UniquePtr;

    virtual bool is_struct() const;
    virtual bool is_cell() const;
    virtual bool is_scalar() const;

    virtual std::string type() const = 0;
    virtual MatScalarType& getScalar();
    virtual std::map<std::string, MatData>& getStruct();
    virtual std::vector<MatData>& getCell();

    virtual MatDataBase::UniquePtr clone() const = 0;

    virtual void print(std::ostream& os = std::cout) const = 0;

    MatDataBase() = default;
    MatDataBase(const MatDataBase&) = default;
    virtual ~MatDataBase() = default;

private:


};

template<typename T>
MatData::MatData(const T& value):
    _data_ptr(make_scalar())
{

    this->value() = value;
}

}

}

#endif // MAT_DATA_H
