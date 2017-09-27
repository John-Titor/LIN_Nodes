
import unittest
from lib import jig
import time

class Test(unittest.TestCase):
    '''Test that we can interact with the jig'''

    def setUp(self):
        jig.reset()

    def tearDown(self):
        jig.reset()

    def _set(self, control, state):
        control.set(state)
        self.assertTrue(control.state == state)

    def test_SG_inputs(self):
        self._set(jig.SG0, jig.PULL_TO_GROUND)
        time.sleep(0.5)
        self._set(jig.SG0, jig.PULL_TO_NONE)
        time.sleep(0.5)
        jig.SG0.reset()

    def test_SP_inputs(self):
        self._set(jig.PullToBattery, 0)

        self._set(jig.SP1, jig.PULL_TO_NONE)
        time.sleep(0.5)
        self._set(jig.SP1, jig.PULL_TO_GROUND)
        time.sleep(0.5)
        self._set(jig.PullToBattery, 1)
        self._set(jig.SP1, jig.PULL_TO_BATTERY)
        time.sleep(0.5)
        jig.SP1.reset()
