
#include <boost/python.hpp>
#include <classad/source.h>

#include "classad_wrapper.h"
#include "exprtree_wrapper.h"


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

    class_<ClassAdWrapper, boost::noncopyable>("ClassAd", "A classified advertisement.")
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
        .def("values", boost::python::range(&ClassAdWrapper::beginValues, &ClassAdWrapper::endValues))
        .def("items", boost::python::range(&ClassAdWrapper::beginItems, &ClassAdWrapper::endItems))
        .def("__len__", &ClassAdWrapper::size)
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

