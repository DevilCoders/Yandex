import time

from pathlib import Path

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite


class TestPing(AntirobotTestSuite):
    Controls = Path.cwd() / "controls"
    PingControlFile = Controls / "weight"

    options = {
        "HandlePingControlFilePath": PingControlFile,
        "HandleWatcherPollInterval": "1s",
    }

    @classmethod
    def setup_class(cls):
        cls.Controls.mkdir(exist_ok=True)
        super().setup_class()

    def setup_method(self, method):
        super().setup_method(method)
        self.set_content("")

    def set_content(self, content):
        with open(self.PingControlFile, "w") as file:
            file.write(content)

    def ordinary_test(self, content, expected):
        self.set_content(content)
        time.sleep(1)
        assert self.send_request("/ping").info()["RS-Weight"] == expected

    def test_default_value(self):
        queries = ["-10", "aboba", "17a", " 10x\n"]
        for content in queries:
            self.ordinary_test(content, "10")
        self.ordinary_test("101", "100")

    def test_common_values(self):
        queries = ["1", "2", "10", "50", "100"]
        for content in queries:
            self.ordinary_test(content, content)
