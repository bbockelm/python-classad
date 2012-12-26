
#include <string>

#include <classad/classad.h>
#include <classad/sink.h>
#include <classad/source.h>
#include <boost/python.hpp>
#include <boost/iterator/transform_iterator.hpp>

std::string ClassadLibraryVersion()
{
    std::string val;
    classad::ClassAdLibraryVersion(val);
    return val;
}

struct AttrPairToFirst :
  public std::unary_function<std::pair<std::string, classad::ExprTree*> const&, std::string>
{
  AttrPairToFirst::result_type operator()(AttrPairToFirst::argument_type p) const
  {
    return p.first;
  }
};
typedef boost::transform_iterator<AttrPairToFirst, classad::AttrList::iterator> AttrKeyIter;

struct ExprTreeHolder
{
    ExprTreeHolder(const std::string &str)
        : m_expr(NULL), m_owns(true)
    {
        classad::ClassAdParser parser;
        classad::ExprTree *expr = NULL;
        if (!parser.ParseExpression(str, expr))
        {
            PyErr_SetString(PyExc_SyntaxError, "Unable to parse string into a ClassAd.");
            boost::python::throw_error_already_set();
        }
        m_expr = expr;
    }

    ExprTreeHolder(classad::ExprTree *expr) : m_expr(expr), m_owns(false)
    {}

    ~ExprTreeHolder()
    {
        if (m_owns && m_expr) delete m_expr;
    }

    boost::python::object Evaluate() const
    {
        if (!m_expr)
        {
            PyErr_SetString(PyExc_RuntimeError, "Cannot operate on an invalid ExprTree");
            boost::python::throw_error_already_set();
        }
        classad::Value value;
        if (!m_expr->Evaluate(value)) {
            PyErr_SetString(PyExc_SyntaxError, "Unable to evaluate expression");
            boost::python::throw_error_already_set();
        }
        boost::python::object result;
        std::string strvalue;
        long long intvalue;
        bool boolvalue;
        double realvalue;
        PyObject* obj;
        switch (value.GetType())
        {
        case classad::Value::BOOLEAN_VALUE:
            value.IsBooleanValue(boolvalue);
            obj = boolvalue ? Py_True : Py_False;
            result = boost::python::object(boost::python::handle<>(boost::python::borrowed(obj)));
            break;
        case classad::Value::STRING_VALUE:
            value.IsStringValue(strvalue);
            result = boost::python::str(strvalue);
            break;
        case classad::Value::ABSOLUTE_TIME_VALUE:
        case classad::Value::INTEGER_VALUE:
            value.IsIntegerValue(intvalue);
            result = boost::python::long_(intvalue);
            break;
        case classad::Value::RELATIVE_TIME_VALUE:
        case classad::Value::REAL_VALUE:
            value.IsRealValue(realvalue);
            result = boost::python::object(realvalue);
            break;
        case classad::Value::ERROR_VALUE:
            result = boost::python::object(classad::Value::ERROR_VALUE);
            break;
        case classad::Value::UNDEFINED_VALUE:
            result = boost::python::object(classad::Value::UNDEFINED_VALUE);
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "Unknown ClassAd value type.");
            boost::python::throw_error_already_set();
        }
        return result;
    }

    std::string toRepr()
    {
        if (!m_expr)
        {
            PyErr_SetString(PyExc_RuntimeError, "Cannot operate on an invalid ExprTree");
            boost::python::throw_error_already_set();
        }
        classad::ClassAdUnParser up;
        std::string ad_str;
        up.Unparse(ad_str, m_expr);
        return ad_str;
    }

    std::string toString()
    {
        if (!m_expr)
        {
            PyErr_SetString(PyExc_RuntimeError, "Cannot operate on an invalid ExprTree");
            boost::python::throw_error_already_set();
        }
        classad::PrettyPrint pp;
        std::string ad_str;
        pp.Unparse(ad_str, m_expr);
        return ad_str;
    }

    classad::ExprTree *get()
    {
        if (!m_expr)
        {
            PyErr_SetString(PyExc_RuntimeError, "Cannot operate on an invalid ExprTree");
            boost::python::throw_error_already_set();
        }
        return m_expr->Copy();
    }

    classad::ExprTree *m_expr;
    bool m_owns;
};

struct AttrPair :
  public std::unary_function<std::pair<std::string, classad::ExprTree*> const&, boost::python::object>
{
  AttrPair::result_type operator()(AttrPair::argument_type p) const
  {
    return boost::python::make_tuple<std::string, boost::python::object>(p.first, boost::python::object(ExprTreeHolder(p.second)));
  }
};
typedef boost::transform_iterator<AttrPair, classad::AttrList::iterator> AttrItemIter;

// Forward decls
struct ClassAdWrapper;
ClassAdWrapper *parseString(const std::string &str);

void *convert_to_FILEptr(PyObject* obj) {
    return PyFile_Check(obj) ? PyFile_AsFile(obj) : 0;
}

struct ClassAdWrapper : classad::ClassAd, boost::python::wrapper<classad::ClassAd>
{
    boost::python::object LookupWrap( const std::string &attr) const
    {
        classad::ExprTree * expr = Lookup(attr);
        if (!expr)
        {
            PyErr_SetString(PyExc_KeyError, attr.c_str());
            boost::python::throw_error_already_set();
        }
        if (expr->GetKind() == classad::ExprTree::LITERAL_NODE) return EvaluateAttrObject(attr);
        ExprTreeHolder holder(expr);
        boost::python::object result(holder);
        return result;
    }

    boost::python::object EvaluateAttrObject(const std::string &attr) const
    {
        classad::ExprTree *expr;
        if (!(expr = Lookup(attr))) {
            PyErr_SetString(PyExc_KeyError, attr.c_str());
            boost::python::throw_error_already_set();
        }
        ExprTreeHolder holder(expr);
        return holder.Evaluate();
    }

    void InsertAttrObject( const std::string &attr, boost::python::object value)
    {
        boost::python::extract<ExprTreeHolder&> expr_obj(value);
        if (expr_obj.check())
        {
            classad::ExprTree *expr = expr_obj().get();
            Insert(attr, expr);
            return;
        }
        boost::python::extract<classad::Value::ValueType> value_enum_obj(value);
        if (value_enum_obj.check())
        {
            classad::Value::ValueType value_enum = value_enum_obj();
            classad::Value classad_value;
            if (value_enum == classad::Value::ERROR_VALUE)
            {
                classad_value.SetErrorValue();
                classad::ExprTree *lit = classad::Literal::MakeLiteral(classad_value);
                Insert(attr, lit);
            }
            else if (value_enum == classad::Value::UNDEFINED_VALUE)
            {
                classad_value.SetUndefinedValue();
                classad::ExprTree *lit = classad::Literal::MakeLiteral(classad_value);
                if (!Insert(attr, lit))
                {
                    PyErr_SetString(PyExc_AttributeError, attr.c_str());
                    boost::python::throw_error_already_set();
                }
            }
            return;
        }
        if (PyString_Check(value.ptr()))
        {
            std::string cppvalue = boost::python::extract<std::string>(value);
            if (!InsertAttr(attr, cppvalue))
            {
                PyErr_SetString(PyExc_AttributeError, attr.c_str());
                boost::python::throw_error_already_set();
            }
            return;
        }
        if (PyLong_Check(value.ptr()))
        {
            long long cppvalue = boost::python::extract<long long>(value);
            if (!InsertAttr(attr, cppvalue))
            {
                PyErr_SetString(PyExc_AttributeError, attr.c_str());
                boost::python::throw_error_already_set();
            }
            return;
        }
        if (PyInt_Check(value.ptr()))
        {
            long int cppvalue = boost::python::extract<long int>(value);
            if (!InsertAttr(attr, cppvalue))
            {
                PyErr_SetString(PyExc_AttributeError, attr.c_str());
                boost::python::throw_error_already_set();
            }
            return;
        }
        if (PyFloat_Check(value.ptr()))
        {
            double cppvalue = boost::python::extract<double>(value);
            if (!InsertAttr(attr, cppvalue))
            {
                PyErr_SetString(PyExc_AttributeError, attr.c_str());
                boost::python::throw_error_already_set();
            }
            return;
        }
        PyErr_SetString(PyExc_TypeError, "Unknown ClassAd value type.");
        boost::python::throw_error_already_set();
    }

    std::string toRepr()
    {
        classad::ClassAdUnParser up;
        std::string ad_str;
        up.Unparse(ad_str, this);
        return ad_str;
    }

    std::string toString()
    {
        classad::PrettyPrint pp;
        std::string ad_str;
        pp.Unparse(ad_str, this);
        return ad_str;
    }

    AttrKeyIter beginKeys()
    {
        return AttrKeyIter(begin());
    }

    AttrKeyIter endKeys()
    {
        return AttrKeyIter(end());
    }

    AttrItemIter beginItems()
    {
        return AttrItemIter(begin());
    }

    AttrItemIter endItems()
    {
        return AttrItemIter(end());
    }

    ClassAdWrapper() : classad::ClassAd() {}

    ClassAdWrapper(const std::string &str)
    {
        classad::ClassAdParser parser;
        classad::ClassAd *result = parser.ParseClassAd(str);
        if (!result)
        {
            PyErr_SetString(PyExc_SyntaxError, "Unable to parse string into a ClassAd.");
            boost::python::throw_error_already_set();
        }
        CopyFrom(*result);
        delete result;
    }
};

ClassAdWrapper *parseString(const std::string &str)
{
    classad::ClassAdParser parser;
    classad::ClassAd *result = parser.ParseClassAd(str);
    if (!result)
    {
        PyErr_SetString(PyExc_SyntaxError, "Unable to parse string into a ClassAd.");
        boost::python::throw_error_already_set();
    }
    ClassAdWrapper * wrapper_result = new ClassAdWrapper();
    wrapper_result->CopyFrom(*result);
    delete result;
    return wrapper_result;
}

ClassAdWrapper *parseFile(FILE *stream)
{
    classad::ClassAdParser parser;
    classad::ClassAd *result = parser.ParseClassAd(stream);
    if (!result)
    {
        PyErr_SetString(PyExc_SyntaxError, "Unable to parse input stream into a ClassAd.");
    }
    ClassAdWrapper * wrapper_result = new ClassAdWrapper();
    wrapper_result->CopyFrom(*result);
    delete result;
    return wrapper_result;
}

BOOST_PYTHON_MODULE(classad)
{
    using namespace boost::python;

    def("version", ClassadLibraryVersion);

    def("parse", parseString, return_value_policy<manage_new_object>());
    def("parse", parseFile, return_value_policy<manage_new_object>());

    class_<ClassAdWrapper, boost::noncopyable>("ClassAd")
        .def(init<std::string>())
        .def("__delitem__", &ClassAdWrapper::Delete)
        .def("__getitem__", &ClassAdWrapper::LookupWrap)
        .def("eval", &ClassAdWrapper::EvaluateAttrObject)
        .def("__setitem__", &ClassAdWrapper::InsertAttrObject)
        .def("__str__", &ClassAdWrapper::toString)
        .def("__repr__", &ClassAdWrapper::toRepr)
        // I see no way to use the SetParentScope interface safely.
        // Delay exposing it to python until we absolutely have to!
        //.def("setParentScope", &ClassAdWrapper::SetParentScope)
        .def("__iter__", boost::python::range(&ClassAdWrapper::beginKeys, &ClassAdWrapper::endKeys))
        .def("keys", boost::python::range(&ClassAdWrapper::beginKeys, &ClassAdWrapper::endKeys))
        .def("items", boost::python::range(&ClassAdWrapper::beginItems, &ClassAdWrapper::endItems))
        ;

    class_<ExprTreeHolder>("ExprTree", init<std::string>())
        .def("__str__", &ExprTreeHolder::toString)
        .def("__repr__", &ExprTreeHolder::toRepr)
        .def("eval", &ExprTreeHolder::Evaluate)
        ;

    boost::python::enum_<classad::Value::ValueType>("Value")
        .value("Error", classad::Value::ERROR_VALUE)
        .value("Undefined", classad::Value::UNDEFINED_VALUE)
        ;

    boost::python::converter::registry::insert(convert_to_FILEptr,
        boost::python::type_id<FILE>(),
        &boost::python::converter::wrap_pytype<&PyFile_Type>::get_pytype);
}

