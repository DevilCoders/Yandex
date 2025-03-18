import time
import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util.asserts import (
    AssertEventuallyTrue,
    AssertRedirectNotToCaptcha,
    AssertNotBlocked,
    AssertNotRobotResponse,
)
from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    GenRandomIP,
    captcha_page,
    spravka,
)


DAEMON_COUNT = 2
INTERVAL = 2
BAN_TIME = 3
MAX_INPUTS = 3
LAG_TIME = 1
CBB_SYNC_PERIOD = 1
LONG_TEST_TIMEOUT = 120


class TestBlockedByCaptchaInputs(AntirobotTestSuite):
    options = {
        'DisableBansByFactors': 1,  # avoid getting captcha
        "CaptchaFreqMaxInputs": MAX_INPUTS,
        "CaptchaFreqMaxInterval": "%ds" % INTERVAL,
        "CaptchaFreqBanTime": "%ds" % BAN_TIME,
        "AllowBlock": 1,
        "RemoveExpiredPeriod": LAG_TIME,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbEnabled": 1,
        "InitialChinaRedirectEnabled": False,
    }

    captcha_args = ['--generate', 'correct', '--check', 'success']
    num_antirobots = DAEMON_COUNT

    def do_block(self, daemon, ip):
        captcha = captcha_page.HtmlCaptchaPage(self, "yandex.ru")

        for _ in range(MAX_INPUTS + 1):
            AssertRedirectNotToCaptcha(spravka.DoCheckCaptcha(daemon, ip, captcha))

        AssertEventuallyTrue(lambda: not daemon.is_blocked(ip), secondsBetweenCalls=0.5)

    def add_cbb_block(self, flag, ip, expire_time):
        resp = self.cbb.add_block(flag, ip, ip, expire_time)
        assert resp.read().strip() == b'Ok'

    def get_full_req(self, req, ip, spravka=None):
        headers = {'X-Forwarded-For-Y': ip}
        if spravka:
            headers['Cookie'] = 'spravka=%s' % spravka

        return Fullreq(req, headers=headers)

    def get_search_req(self, ip, spravka=None):
        return self.get_full_req("http://yandex.ru/yandsearch?text=cats", ip, spravka)

    def get_non_search_req(self, ip, spravka=None):
        return self.get_full_req("http://yandex.ru/news", ip, spravka)

    def get_cbb_flag_ips(self):
        resp = self.cbb.fetch_flag_data(int(self.antirobot.dump_cfg()["CbbFlag"]), with_format=['range_src'])
        assert resp.getcode() == 200
        return [x.strip().decode('utf-8') for x in resp.readlines()]

    @pytest.mark.parametrize("daemon_id", range(DAEMON_COUNT))
    def test_block_on_captcha_inputs(self, daemon_id):
        ip = GenRandomIP()
        self.do_block(self.antirobots[daemon_id], ip)

        for d in self.antirobots:
            AssertEventuallyTrue(lambda: not d.is_blocked(ip), secondsBetweenCalls=CBB_SYNC_PERIOD/2.0)

        time.sleep(BAN_TIME)
        for d in self.antirobots:
            AssertEventuallyTrue(lambda: not d.is_blocked(ip), secondsBetweenCalls=LAG_TIME/2.0)

    def test_must_not_block(self):
        ip = GenRandomIP()
        captcha = captcha_page.HtmlCaptchaPage(self, "yandex.ru")
        daemon = self.antirobots[0]

        # block will occur on (MAX_INPUT+1)-th request
        for i in range(MAX_INPUTS):
            AssertNotBlocked(spravka.DoCheckCaptcha(daemon, ip, captcha))

        assert not daemon.is_blocked(ip)

        time.sleep(INTERVAL)
        AssertNotBlocked(spravka.DoCheckCaptcha(daemon, ip, captcha))
        AssertNotBlocked(spravka.DoCheckCaptcha(daemon, ip, captcha))
        time.sleep(1)
        assert not daemon.is_blocked(ip)

    def test_single_cbb_record(self):
        ip1 = GenRandomIP()
        ip2 = GenRandomIP()
        daemon = self.antirobots[0]

        self.do_block(daemon, ip1)
        self.do_block(daemon, ip2)

        self.do_block(daemon, ip1)
        self.do_block(daemon, ip2)

        self.do_block(daemon, ip1)
        self.do_block(daemon, ip2)

        AssertEventuallyTrue(lambda: self.get_cbb_flag_ips().count(ip1) == 1)
        assert self.get_cbb_flag_ips().count(ip2) == 1

    def test_block_by_external_cbb_add(self):
        ip = GenRandomIP()
        spr = spravka.GetSpravkaForAddr(self.antirobots[0], ip)

        flag = int(self.antirobot.dump_cfg()["CbbFlag"])
        self.add_cbb_block(flag, ip, int(time.time() + 5 * 60))

        # wait for Antirobot refresh its externally blocked addrs
        time.sleep(CBB_SYNC_PERIOD + 1)

        for d in self.antirobots:
            AssertNotBlocked(d.send_request(self.get_search_req(ip, spr)))
            AssertNotBlocked(d.send_request(self.get_search_req(ip)))
            AssertNotBlocked(d.send_request(self.get_search_req(GenRandomIP(), spr)))

            AssertNotBlocked(d.send_request(self.get_non_search_req(ip, spr)))
            AssertNotBlocked(d.send_request(self.get_non_search_req(ip)))
            AssertNotBlocked(d.send_request(self.get_non_search_req(GenRandomIP(), spr)))

    def test_not_robot_after_external_block(self):
        ip = GenRandomIP()

        flag = int(self.antirobot.dump_cfg()["CbbFlag"])
        self.add_cbb_block(flag, ip, int(time.time() + BAN_TIME))

        time.sleep(CBB_SYNC_PERIOD)

        for d in self.antirobots:
            AssertEventuallyTrue(lambda: not d.is_blocked(ip), secondsBetweenCalls=1)

        time.sleep(BAN_TIME + CBB_SYNC_PERIOD)

        for d in self.antirobots:
            resp = d.send_request(self.get_search_req(ip))
            AssertNotBlocked(resp)
            AssertNotRobotResponse(resp)
