from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite

from antirobot.daemon.arcadia_test import util
from antirobot.daemon.arcadia_test.util import asserts


class TestCommandLineArgs(AntirobotTestSuite):

    def test_custom_port(self):
        port = self.get_port()

        with self.start_antirobots({}, args=["-p", port], wait=False):
            asserts.AssertEventuallyTrue(lambda: util.service_available(port))
