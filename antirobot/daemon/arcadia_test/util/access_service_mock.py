import yatest

from antirobot.daemon.arcadia_test.util.mock import NetworkSubprocess


class AccessService(NetworkSubprocess):
    def __init__(self, port, args, **kwargs):
        path = yatest.common.build_path(
            "antirobot/tools/access_service_mock/access_service_mock",
        )

        super().__init__(path, port, ["--addr", f":{port}"] + args, **kwargs)
