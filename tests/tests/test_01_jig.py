
import unittest
from lib import jig, probe

class Test(unittest.TestCase):
    '''Test that we can interact with the jig'''

    _jig = None
    _probe = None

    def setUp(self):
        print "setUp"
        self._jig = jig.jig()
        self._probe = probe.probe()

    def tearDown(self):
        print "tearDown"

    def test_one(self):
        print "test_one"

