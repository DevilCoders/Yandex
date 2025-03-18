from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util.asserts import (
    AssertNotRobotResponse,
    AssertXmlCaptcha,
)
from antirobot.daemon.arcadia_test.util import (
    GenRandomIP,
    Fullreq,
)


class TestXmlSearch(AntirobotTestSuite):
    def test_don_show_captcha_on_ordinary_search(self):
        ip = GenRandomIP()

        self.antirobot.ban(ip)
        request = Fullreq(
            "https://yandex.ru/search/xml"
            "?user=seobitproject"
            "&key=03.85393049:21180c0d9b1c84c55c8aa58b5789d911"
            "&l10n=ru&sortby=tm.order%3Dascending"
            "&filter=strict&groupby=attr%3D%22%22.mode%3Dflat.groups-on-page%3D10.docs-in-group%3D1"
            "&lr=2&query=&lr=2",
            headers={
                "X-Forwarded-For-Y": ip,
            })

        for _ in range(1000):
            AssertNotRobotResponse(self.antirobot.send_request(request))

    def test_show_captcha_on_partner_requests(self):
        ip = GenRandomIP()

        self.antirobot.ban(ip)
        partner_request = Fullreq("http://yandex.com.tr/search/xml?showmecaptcha=yes&text=123",
                                  headers={
                                      "X-Forwarded-For-Y": ip,
                                      "X-Real-Ip": ip,
                                  })

        AssertXmlCaptcha(self.antirobot.send_request(partner_request))

    def test_show_captcha_on_images_xml_requests(self):
        ip = GenRandomIP()

        self.antirobot.ban(ip)
        partner_request = Fullreq("http://yandex.ru/images-xml?showmecaptcha=yes&text=123",
                                  headers={
                                      "X-Forwarded-For-Y": ip,
                                      "X-Real-Ip": ip,
                                  })

        AssertXmlCaptcha(self.antirobot.send_request(partner_request))
