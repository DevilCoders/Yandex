import yatest

from antirobot.daemon.arcadia_test.util.mock import NetworkSubprocess


class Wizard(NetworkSubprocess):
    def __init__(self, port, **kwargs):
        path = yatest.common.build_path(
            "antirobot/tools/wizard_mock/wizard_mock",
        )

        super().__init__(path, port, ["--port", str(port)], **kwargs)
