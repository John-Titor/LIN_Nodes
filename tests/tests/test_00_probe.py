
import unittest
from lib import jig, probe

class TestOne(unittest.TestCase):

    def setUp(self):
        print "setUp"
        self._jig = jig.jig()
        self._probe = probe.probe()

    def tearDown(self):
        print "tearDown"

    def test_one(self):
        self._master = self._probe.scan(1)
        self.assertTrue(self._master is not None)

