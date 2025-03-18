import urllib
import yatest

from antirobot.daemon.arcadia_test.util.mock import NetworkSubprocess


class Cbb(NetworkSubprocess):
    def __init__(self, port, **kwargs):
        path = yatest.common.build_path(
            "antirobot/tools/cbb_api_mock/cbb_api_mock",
        )

        super().__init__(path, port, [str(port)], **kwargs)

    def do_cbb_request(self, req, params={}):
        url = f"http://{self.host}/cgi-bin/{req}"

        if len(params) > 0:
            url += "?" + urllib.parse.urlencode(params)

        resp = urllib.request.urlopen(url)
        assert resp.getcode() == 200

        return resp

    def set_range(self, op, params={}):
        return self.do_cbb_request("set_range.pl", {"operation": op, **params})

    def add_block(self, flag, range_src, range_dst, expire):
        return self.set_range("add", {
            "flag": flag,
            "range_src": range_src,
            "range_dst": range_dst,
            "expire": expire,
        })

    def add_text_block(self, flag, txt_block, rule_id=None):
        params = {"flag": flag, "range_txt": txt_block}

        if rule_id is not None:
            params["rule_id"] = rule_id

        return self.set_range("add", params)

    def add_re_block(self, flag, re_block):
        return self.set_range("add", {"flag": flag, "range_re": re_block})

    def remove_block(self, flag, range_src):
        return self.set_range("del", {"flag": flag, "range_src": range_src})

    def remove_text_block(self, flag, txt_block):
        return self.set_range("del", {"flag": flag, "range_txt": txt_block})

    def fetch_flag_data(self, flag, with_format=[]):
        return self.do_cbb_request("get_range.pl", {
            "flag": flag,
            "with_format": ",".join(with_format),
        })

    def clear(self, flags=[]):
        self.do_cbb_request("debug/clear.pl", {"flags": ",".join(map(str, flags))})
