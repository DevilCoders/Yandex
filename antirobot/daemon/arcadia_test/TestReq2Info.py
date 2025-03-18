from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite


class TestReq2Info(AntirobotTestSuite):
    def assert_correct_result(self, result_data):
        assert 'HostType: web\n' in result_data

    def test_url_request(self):
        req_str = "http://yandex.ru/search/?text=test"
        self.assert_correct_result(self.req2info(req_str))

    def test_raw_request(self):
        req_str = "GET /search/?text=kex&clid=1537597 HTTP/1.1\r\n" \
            "Host: yandex.ru\r\n" \
            "X-Forwarded-For-Y: 1.2.3.4\r\n" \
            "\r\n"

        self.assert_correct_result(self.req2info(req_str, rawreq=True))
