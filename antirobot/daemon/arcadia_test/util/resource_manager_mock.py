import yatest

from antirobot.daemon.arcadia_test.util.mock import NetworkSubprocess


class ResourceManager(NetworkSubprocess):
    def __init__(self, port, args, **kwargs):
        path = yatest.common.build_path(
            "antirobot/tools/resource_manager_mock/resource_manager_mock",
        )

        super().__init__(path, port, ["--addr", f":{port}"] + args, **kwargs)
