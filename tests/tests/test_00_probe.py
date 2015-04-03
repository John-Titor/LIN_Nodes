
import unittest

class TestOne(unittest.TestCase):

    def setUp(self):
        print "setUp"

    def tearDown(self):
        print "tearDown"

    def test_one(self):
        print "test_one"

