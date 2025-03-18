import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import Fullreq


GUARANTEED_CHINA_IP = "59.151.106.224"
IP_HEADER = "X-Forwarded-For-Y"
UNAUTHORIZED_URL_FROM_CHINA = ["http://yandex.ru/portal/ntp/notifications", "http://yandex.ru/portal/ntp/notifications/test"]
REGULAR_SEARCH = "http://yandex.ru/search?text=cats"
REDIRECT_CHINA_HERE = "https://passport.yandex.ru/auth/?origin=china&retpath=http%3A%2F%2Fyandex.ru%2Fsearch%3Ftext%3Dcats"
VALID_L_COOKIE = "L=SG0TWlVvB1J9DXZXW0AHWw9neXB1bncCEjgrJkYPUmdMHgIvKx1YAicOERR7XhghDQIPElM6dQc9JwtaXTsDHw==.1311989535.9065.287854.22faf2292da9c7dbabc30e8512cc098d"


class TestChinaRedirect(AntirobotTestSuite):
    options = {
        "DisableBansByFactors": 1,
    }

    def GenerateChinaRequest(self, search=REGULAR_SEARCH):
        return Fullreq(search, headers={IP_HEADER: GUARANTEED_CHINA_IP, "accept-language": "zh"})

    def GenerateChinaRequestWithLCookie(self, search=REGULAR_SEARCH):
        return Fullreq(search, headers={IP_HEADER: GUARANTEED_CHINA_IP, "accept-language": "zh", "Cookie": VALID_L_COOKIE})

    def GenerateChinaRequestAcceptingRussian(self, search=REGULAR_SEARCH):
        return Fullreq(search, headers={IP_HEADER: GUARANTEED_CHINA_IP, "accept-language": "ru"})

    def test_check_china_redirect_stats(self):
        request = self.GenerateChinaRequest()
        self.send_request(request)
        assert(self.antirobot.get_metric("is_china_redirect_enabled_ahhh") == 1)
        self.send_request("/admin?action=stop_china_redirect")
        assert(self.antirobot.get_metric("is_china_redirect_enabled_ahhh") == 0)
        self.send_request(request)
        assert(self.antirobot.get_metric("requests_from_china_not_loginned_deee") == 2)
        assert(self.antirobot.get_metric("redirected_to_login_requests_from_china_deee") == 1)
        self.send_request("/admin?action=start_china_redirect")

    def test_check_china_request_redirected(self):
        request = self.GenerateChinaRequest()
        response = self.send_request(request)
        httpFound = 302
        assert(response.getcode() == httpFound)
        assert(response.headers['Location'] == REDIRECT_CHINA_HERE)

    def test_check_china_redirect_disabled(self):
        request = self.GenerateChinaRequest()
        self.send_request("/admin?action=stop_china_redirect")
        response = self.send_request(request)
        httpOk = 200
        assert(response.getcode() == httpOk)
        self.send_request("/admin?action=start_china_redirect")

    def test_loginned_china_request_not_banned(self):
        request = self.GenerateChinaRequestWithLCookie()
        response = self.send_request(request)
        httpOk = 200
        assert(response.getcode() == httpOk)

    def test_chinese_request_accepting_russian_not_banned(self):
        request = self.GenerateChinaRequestAcceptingRussian()
        response = self.send_request(request)
        httpOk = 200
        assert(response.getcode() == httpOk)

    @pytest.mark.parametrize('url', UNAUTHORIZED_URL_FROM_CHINA)
    def test_check_china_unauthorized(self, url):
        request = self.GenerateChinaRequest(url)
        response = self.send_request(request)
        httpUnauthorized = 401
        assert(response.getcode() == httpUnauthorized)

    @pytest.mark.parametrize('url', UNAUTHORIZED_URL_FROM_CHINA)
    def test_check_china_unauthorized_disabled(self, url):
        request = self.GenerateChinaRequest(url)
        self.send_request("/admin?action=stop_china_unauthorized")
        response = self.send_request(request)
        httpOk = 302
        assert(response.getcode() == httpOk)
        self.send_request("/admin?action=start_china_unauthorized")

    @pytest.mark.parametrize('url', UNAUTHORIZED_URL_FROM_CHINA)
    def test_check_china_unathorized_stats(self, url):
        request = self.GenerateChinaRequest(url)
        prevNumRequests = self.antirobot.get_metric("unauthorized_request_from_china_deee")
        self.send_request(request)
        assert(self.antirobot.get_metric("is_china_unauthorized_enabled_ahhh") == 1)
        self.send_request("/admin?action=stop_china_unauthorized")
        assert(self.antirobot.get_metric("is_china_unauthorized_enabled_ahhh") == 0)
        self.send_request(request)
        assert(self.antirobot.get_metric("unauthorized_request_from_china_deee") == prevNumRequests + 1)
        self.send_request("/admin?action=start_china_unauthorized")
