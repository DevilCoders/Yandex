from yatest.common import network
from library.python.testing.pyremock.lib.pyremock import MockHttpServer


class BaseTest(object):
    DEFAULT_PORT = 11111

    @classmethod
    def setup_class(cls):
        cls.pm = network.PortManager()
        cls.mock_port = cls.pm.get_port(cls.DEFAULT_PORT)
        cls.mock_server = MockHttpServer(cls.mock_port)
        cls.mock_server.start()

    @classmethod
    def teardown_class(cls):
        cls.mock_server.stop()
        cls.pm.release()

    def setup(self):
        self.mock_server.reset()

    def _make_url(self, port, query):
        return 'http://localhost:{port}{query}'.format(port=port, query=query)

    def _start_mock(self):
        mock_port = self.pm.get_port()
        mock_server = MockHttpServer(mock_port)
        mock_server.start()
        return mock_server, mock_port
