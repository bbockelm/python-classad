
#include <boost/python.hpp>
#include <classad/source.h>

#include "classad_wrapper.h"
#include "exprtree_wrapper.h"

using namespace boost::python;


Py_ssize_t py_len(boost::python::object const& obj)
{
    Py_ssize_t result = PyObject_Length(obj.ptr());
    if (PyErr_Occurred()) boost::python::throw_error_already_set();
    return result;
}


std::string ClassadLibraryVersion()
{
    std::string val;
    classad::ClassAdLibraryVersion(val);
    return val;
}


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
        boost::python::throw_error_already_set();
    }
    ClassAdWrapper * wrapper_result = new ClassAdWrapper();
    wrapper_result->CopyFrom(*result);
    delete result;
    return wrapper_result;
}

ClassAdWrapper *parseOld(object input)
{
    ClassAdWrapper * wrapper = new ClassAdWrapper();
    object input_list;
    extract<std::string> input_extract(input);
    if (input_extract.check())
    {
        input_list = input.attr("splitlines")();
    }
    else
    {
        input_list = input.attr("readlines")();
    }
    unsigned input_len = py_len(input_list);
    for (unsigned idx=0; idx<input_len; idx++)
    {
        object line = input_list[idx].attr("strip")();
        if (line.attr("startswith")("#"))
        {
            continue;
        }
        std::string line_str = extract<std::string>(line);
        if (!wrapper->Insert(line_str))
        {
            PyErr_SetString(PyExc_SyntaxError, line_str.c_str());
            throw_error_already_set();
        }
    }
    return wrapper;
}

void *convert_to_FILEptr(PyObject* obj) {
    return PyFile_Check(obj) ? PyFile_AsFile(obj) : 0;
}

BOOST_PYTHON_MODULE(classad)
{
    using namespace boost::python;

    def("version", ClassadLibraryVersion, "Return the version of the linked ClassAd library.");

    def("parse", parseString, return_value_policy<manage_new_object>());
    def("parse", parseFile, return_value_policy<manage_new_object>(),
        "Parse input into a ClassAd.\n"
        ":param input: A string or a file pointer.\n"
        ":return: A ClassAd object.");
    def("parseOld", parseOld, return_value_policy<manage_new_object>(),
        "Parse old ClassAd format input into a ClassAd.\n"
        ":param input: A string or a file pointer.\n"
        ":return: A ClassAd object.");

    class_<ClassAdWrapper, boost::noncopyable>("ClassAd", "A classified advertisement.")
        .def(init<std::string>())
        .def("__delitem__", &ClassAdWrapper::Delete)
        .def("__getitem__", &ClassAdWrapper::LookupWrap)
        .def("eval", &ClassAdWrapper::EvaluateAttrObject, "Evaluate the ClassAd attribute to a python object.")
        .def("__setitem__", &ClassAdWrapper::InsertAttrObject)
        .def("__str__", &ClassAdWrapper::toString)
        .def("__repr__", &ClassAdWrapper::toRepr)
        // I see no way to use the SetParentScope interface safely.
        // Delay exposing it to python until we absolutely have to!
        //.def("setParentScope", &ClassAdWrapper::SetParentScope)
        .def("__iter__", boost::python::range(&ClassAdWrapper::beginKeys, &ClassAdWrapper::endKeys))
        .def("keys", boost::python::range(&ClassAdWrapper::beginKeys, &ClassAdWrapper::endKeys))
        .def("values", boost::python::range(&ClassAdWrapper::beginValues, &ClassAdWrapper::endValues))
        .def("items", boost::python::range(&ClassAdWrapper::beginItems, &ClassAdWrapper::endItems))
        .def("__len__", &ClassAdWrapper::size)
        .def("lookup", &ClassAdWrapper::LookupExpr, "Lookup an attribute and return a ClassAd expression.  This method will not attempt to evaluate it to a python object.")
        .def("printOld", &ClassAdWrapper::toOldString, "Represent this ClassAd as a string in the \"old ClassAd\" format.")
        ;

    class_<ExprTreeHolder>("ExprTree", "An expression in the ClassAd language", init<std::string>())
        .def("__str__", &ExprTreeHolder::toString)
        .def("__repr__", &ExprTreeHolder::toRepr)
        .def("eval", &ExprTreeHolder::Evaluate)
        ;

    register_ptr_to_python< boost::shared_ptr<ClassAdWrapper> >();

    boost::python::enum_<classad::Value::ValueType>("Value")
        .value("Error", classad::Value::ERROR_VALUE)
        .value("Undefined", classad::Value::UNDEFINED_VALUE)
        ;

    boost::python::converter::registry::insert(convert_to_FILEptr,
        boost::python::type_id<FILE>());
}

