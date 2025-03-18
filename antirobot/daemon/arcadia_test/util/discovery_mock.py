import yatest
import requests
import base64

from antirobot.daemon.arcadia_test.util.mock import NetworkSubprocess


class Discovery(NetworkSubprocess):
    def __init__(self, port, admin_port, **kwargs):
        self.admin_port = admin_port

        path = yatest.common.build_path(
            "antirobot/tools/discovery_mock/discovery_mock",
        )

        super().__init__(
            path, port,
            [
                "--admin-port", str(admin_port),
                "--discovery-port", str(port),
            ],
            **kwargs
        )

    def all_ports(self):
        return [self.port, self.admin_port]

    def set(self, req, rsp):
        response = requests.post(f"http://localhost:{self.admin_port}/set", json={
            "req": self.__serialize(req),
            "rsp": self.__serialize(rsp),
        })

        response.raise_for_status()

    def remove(self, req):
        response = requests.post(f"http://localhost:{self.admin_port}/remove", json={
            "req": self.__serialize(req),
        })

        response.raise_for_status()

    def clear(self):
        response = requests.post(f"http://localhost:{self.admin_port}/clear", json={})
        response.raise_for_status()

    @staticmethod
    def __serialize(x):
        return base64.b64encode(x.SerializeToString()).decode()
