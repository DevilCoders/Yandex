import json
import urllib
import yatest

from antirobot.daemon.arcadia_test.util.mock import NetworkSubprocess


class Captcha(NetworkSubprocess):
    def __init__(self, port, args, **kwargs):
        path = yatest.common.build_path(
            "antirobot/tools/api_captcha_mock/api_captcha_mock",
        )

        super().__init__(path, port, ["-p", str(port)] + args, **kwargs)

    def set_strategy(self, key, value):
        resp = urllib.request.urlopen(f"http://{self.host}/set_strategy?key={key}&value={value}")
        assert resp.getcode() == 200
        resp_content = resp.read().decode()
        assert json.loads(resp_content)["result"] == "ok", resp_content
