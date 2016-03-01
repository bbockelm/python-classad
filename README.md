
# Introduction

The python-classad package aims to provide a python interface to the ClassAd
library used by HTCondor.  It tries to expose as many "safe" uses of ClassAds
as possible while maintaining an interface which is natural to python
programmers.

# Prerequisites

Building requires the following:

* CMake 2.6 or higher
* GCC 4.8 or better (HTCondor classads library needs C++11)
* boost.python and development libraries
* Python development libraries
* HTCondor classads development library 

## RHEL 6+

    yum install cmake boost-devel boost-python condor-classads-devel python-devel

# Building

    cmake .
    make

This produces "classad.so" in the working directory.

# Usage

See below for an example session:

    [bbockelm@hcc-briantest python-classads]$ python
    Python 2.6.6 (r266:84292, Jun 18 2012, 09:57:52) 
    [GCC 4.4.6 20110731 (Red Hat 4.4.6-3)] on linux2
    Type "help", "copyright", "credits" or "license" for more information.
    >>> import classad
    >>> ad = classad.ClassAd()
    >>> expr = classad.ExprTree("2+2")
    >>> ad["foo"] = expr
    >>> print ad["foo"].eval()
    4
    >>> ad["bar"] = 2.1
    >>> ad["baz"] = classad.ExprTree("time() + 4")
    >>> print list(ad)
    ['bar', 'foo', 'baz']
    >>> print dict(ad.items())
    {'baz': time() + 4, 'foo': 2 + 2, 'bar': 2.100000000000000E+00}
    >>> print ad
    
        [
            bar = 2.100000000000000E+00; 
            foo = 2 + 2; 
            baz = time() + 4
        ]
    >>> ad2=classad.parse(open("test_ad", "r"));
    >>> ad2["error"] = classad.Value.Error
    >>> ad2["undefined"] = classad.Value.Undefined
    >>> print ad2
    
        [
            error = error; 
            bar = 2.100000000000000E+00; 
            foo = 2 + 2; 
            undefined = undefined; 
            baz = time() + 4
        ]
    >>> ad2["undefined"]
    classad.Value.Undefined
    >>> 

