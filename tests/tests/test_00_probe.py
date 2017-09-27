
import unittest
from lib import probe

class Test(unittest.TestCase):
    '''Test basic USB to LIN connectivity'''

    def setUp(self):
        self._probe = probe.probe()
        self._master = self._probe.scan(1)

    def tearDown(self):
        pass

    # Locate the master node
    def test_01(self):
        self.assertTrue(self._master is not None)

    # Try an all-nodes scan, we should find at least one node (the master)
    def test_02(self):
        nodes = self._probe.scan()
        self.assertTrue(len(nodes) > 0)

    # Read the BoardFunction parameter from the master and verify that it's as expected
    def test_03(self):
        master_function = self._master.get_param(1)
        self.assertTrue(master_function == 1)
