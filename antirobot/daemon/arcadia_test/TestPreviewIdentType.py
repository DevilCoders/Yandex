import pytest
import time
from pathlib import Path

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    GenRandomIP,
)
from antirobot.daemon.arcadia_test.util.asserts import (
    AssertResponseForUser,
    AssertNotRobotResponse,
)


HANDLE_WATCHER_POLL_INTERVAL = 0.1
AMNESTY_IP_INTERVAL = 3
REMOVE_EXPIRED_PERIOD = 0.5

WHATSAPP_UA1 = "WhatsApp/2.20.52 i"
WHATSAPP_UA2 = "WhatsApp/3.0"
VIBER_UA = "Viber/5.5.0.3531 CFNetwork/758.5.3 Darwin/15.6.0"
SKYPE_UA = "Mozilla/5.0 (Windows NT 6.1; WOW64) SkypeUriPreview Preview/0.5"
FIREFOX_UA = "Mozilla/5.0 (Windows; U; Windows NT 5.1; ru; rv:1.8.0.9) Gecko/20061206 Firefox/1.5.0.9"
SAFARI_UA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36"
YOWSER_UA = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.122 YaBrowser/20.3.0.2220 Yowser/2.5 Safari/537.36"

UA_SPACE = {
    WHATSAPP_UA1: 1,
    WHATSAPP_UA2: 1,
    VIBER_UA: 1,
    SKYPE_UA: 1,
}


class TestBlockedPage(AntirobotTestSuite):
    Controls = Path.cwd() / "controls"
    options = {
        "HandlePreviewIdentTypeEnabledFilePath": Controls / "preview_ident_type_enabled",
        "HandleWatcherPollInterval": "%.1fs" % HANDLE_WATCHER_POLL_INTERVAL,
        "preview_ident_type_enabled@collections": True,
        "preview_ident_type_enabled@web": False,
        "DisableBansByFactors": 1,
        "AmnestyIpInterval": AMNESTY_IP_INTERVAL,
        "RemoveExpiredPeriod": REMOVE_EXPIRED_PERIOD,
    }

    @classmethod
    def setup_class(cls):
        cls.Controls.mkdir(exist_ok=True)
        super().setup_class()

    @pytest.mark.parametrize("service, user_agent, ip_ver", [
        ("collections", FIREFOX_UA,   6),
        ("collections", YOWSER_UA,    4),
        ("collections", WHATSAPP_UA1, 6),
        ("collections", WHATSAPP_UA2, 4),
        ("collections", VIBER_UA,     4),
        ("collections", SKYPE_UA,     6),
    ])
    def test_ban(self, service, user_agent, ip_ver):
        ip = GenRandomIP(ip_ver)
        self.antirobot.ban(ip, service=service, headers={"User-Agent": user_agent})

        # Если забанили в мессенджере, не должны забанить этот IP вообще
        # Если забанили не в мессенджере, то в любом мессенджере запросы будут проходить

        for ua in (WHATSAPP_UA1, WHATSAPP_UA2, VIBER_UA, SKYPE_UA, FIREFOX_UA, SAFARI_UA, YOWSER_UA):
            if UA_SPACE.get(user_agent) == UA_SPACE.get(ua):
                assert self.antirobot.is_banned(ip, path=service, headers={"User-Agent": ua}), f"Must be banned on '{ua}'"
            else:
                assert not self.antirobot.is_banned(ip, path=service, headers={"User-Agent": ua}), f"Must not be banned on '{ua}'"

    @pytest.mark.parametrize("service, service_type, config_enabled, panic_enabled", [
        ("collections", "collections", True,  False),
        ("yandsearch",  "web",         False, False),
        ("collections", "collections", True,  True),
        ("yandsearch",  "web",         False, True),
    ])
    def test_disabled(self, service, service_type, config_enabled, panic_enabled):
        ip = GenRandomIP()
        self.set_panic_flags(service_type if panic_enabled else "")

        request_url = f'http://yandex.ru/{service}?text=skachat+votsap+besplatno+bez+registracii+i+sms'
        headers = {"User-Agent": WHATSAPP_UA1, "X-Forwarded-For-Y": ip}
        response = self.send_request(Fullreq(request_url, headers=headers))

        if config_enabled and not panic_enabled:
            assert response.getcode() == 200
            AssertNotRobotResponse(response)
        else:
            AssertResponseForUser(response)
            assert response.getcode() == 204
            assert not response.read()

    def test_amnesty(self):
        ip = GenRandomIP()
        service = "collections"
        user_agent = WHATSAPP_UA1

        self.antirobot.ban(ip, service=service, headers={"User-Agent": user_agent})
        assert self.antirobot.is_banned(ip, path=service, headers={"User-Agent": user_agent})
        time.sleep((AMNESTY_IP_INTERVAL + REMOVE_EXPIRED_PERIOD) * 1.1)
        assert not self.antirobot.is_banned(ip, path=service, headers={"User-Agent": user_agent})

    def set_panic_flags(self, value):
        with open(self.options["HandlePreviewIdentTypeEnabledFilePath"], "w") as file:
            file.write(value)
        time.sleep(HANDLE_WATCHER_POLL_INTERVAL + 0.1)

    def teardown_method(self):
        self.set_panic_flags("")
        super().teardown_method()
