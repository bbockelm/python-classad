#!/usr/bin/python

import re
import classad
import unittest

class TestClassad(unittest.TestCase):

    def test_load_classad_from_file(self):
        ad = classad.parse(open("tests/test.ad"))
        self.assertEqual(ad["foo"], "bar")
        self.assertEqual(ad["baz"], classad.Value.Undefined)
        self.assertRaises(KeyError, ad.__getitem__, "bar")

    def test_old_classad(self):
        ad = classad.parseOld(open("tests/test.old.ad"))
        contents = open("tests/test.old.ad").read()
        self.assertEqual(ad.printOld(), contents)

    def test_exprtree(self):
        ad = classad.ClassAd()
        ad["foo"] = classad.ExprTree("2+2")
        expr = ad["foo"]
        self.assertEqual(expr.__repr__(), "2 + 2")
        self.assertEqual(expr.eval(), 4)

    def test_exprtree_func(self):
        ad = classad.ClassAd()
        ad["foo"] = classad.ExprTree('regexps("foo (bar)", "foo bar", "\\\\1")')
        self.assertEqual(ad.eval("foo"), "bar")

    def test_ad_assignment(self):
        ad = classad.ClassAd()
        ad["foo"] = 2.1
        self.assertEqual(ad["foo"], 2.1)
        ad["foo"] = 2
        self.assertEqual(ad["foo"], 2)
        ad["foo"] = "bar"
        self.assertEqual(ad["foo"], "bar")
        self.assertRaises(TypeError, ad.__setitem__, {})

    def test_ad_refs(self):
        ad = classad.ClassAd()
        ad["foo"] = classad.ExprTree("bar + baz")
        ad["bar"] = 2.1
        ad["baz"] = 4
        self.assertEqual(ad["foo"].__repr__(), "bar + baz")
        self.assertEqual(ad.eval("foo"), 6.1)

    def test_ad_special_values(self):
        ad = classad.ClassAd()
        ad["foo"] = classad.ExprTree('regexp(12, 34)')
        ad["bar"] = classad.Value.Undefined
        self.assertEqual(ad["foo"].eval(), classad.Value.Error)
        self.assertNotEqual(ad["foo"].eval(), ad["bar"])
        self.assertEqual(classad.Value.Undefined, ad["bar"])

    def test_ad_iterator(self):
        ad = classad.ClassAd()
        ad["foo"] = 1
        ad["bar"] = 2
        self.assertEqual(len(ad), 2)
        self.assertEqual(len(list(ad)), 2)
        self.assertEqual(list(ad)[1], "foo")
        self.assertEqual(list(ad)[0], "bar")
        self.assertEqual(list(ad.items())[1][1], 1)
        self.assertEqual(list(ad.items())[0][1], 2)
        self.assertEqual(list(ad.values())[1], 1)
        self.assertEqual(list(ad.values())[0], 2)

    def test_ad_lookup(self):
        ad = classad.ClassAd()
        ad["foo"] = classad.Value.Error
        self.assertTrue(isinstance(ad.lookup("foo"), classad.ExprTree))
        self.assertEquals(ad.lookup("foo").eval(), classad.Value.Error)

if __name__ == '__main__':
    unittest.main()

