import random
import time

import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    asserts,
    Fullreq,
    GenRandomIP,
    ICOOKIE_HEADER,
    IP_HEADER,
    IsCaptchaRedirect,
    random_segment,
    VALID_I_COOKIE,
)

REGULAR_SEARCH = "http://yandex.ru/search/?text=cats"

SESSION_ID_COOKIE = (
    "PHPSESSID=9kjbrj36snavjf30j2ipoatkh2; yandex_gid=11071; "
    "my_perpages=%5B%5D; Session_id=3:1560790262.5.0.150790262792:yh37FQ:82.1|888164031.-1.3|209836.840232.pe5iYh7FgIYXB4F6zVwMSA3Jvjc"
)
SESSION_ID_MASKED_COOKIE = (
    "PHPSESSID=9kjbrj36snavjf30j2ipoatkh2; yandex_gid=11071; "
    "my_perpages=%5B%5D; Session_id=3:1560790262.5.0.150790262792:yh37FQ:82.1|888164031.-1.3|209836.8402******************************"
)
SESSIONID2_COOKIE = (
    "yp=1542747101.csc.1#1562068164.oyu.6815984621558243395#1555836689.szm.1%3A1920x1080%3A1800x964#1879534633.udn."
    "cDp2aXJ0dWFsLnZlcmdl#1542660685.ygu.1#1831732584.yrts.1516372584#1874836173.yrtsi.1559476173#1541278287.ysl.1#1559562564.yu.6815984621558243395; "
    "L=AU3cfWxQU3tcXHFad3UFVXBSegtXQUNaGBw/PTEjWFoFIwNeXA==.1564174633.13938.317263.68893845ed97c3a0f2f016f31b176c41; "
    "device_id=\"a8912d0225f8d6862d60b65af255494f94d4b0c4a\"; ys=ymrefl.D9A32DC84B3FC515#udn.cDp2aXJ0dWFsLnZlcmdl; "
    "sessionid2=3:1565200767.5.0.1564174633483:yh37FNGUmADrUhSBcBMAKg:4c.1|489625370.0.302.0:3|203289.371932.A9UQS_Yu0ibPRA1fbZEZHg6VGiI; "
    "_ym_isad=2; lastVisitedPage=%7B%22489625370%22%3A%22%2Fartist%2F2662089%2Ftracks%22%7D; cycada=mhD2sSFyprraC/+Sn/H8+vhEtTZfh0rsaE8ss1pvRHI=; "
    "active-browser-timestamp=1565213296088"
)
SESSIONID2_MASKED_COOKIE = (
    "yp=1542747101.csc.1#1562068164.oyu.6815984621558243395#1555836689.szm.1%3A1920x1080%3A1800x964#1879534633.udn."
    "cDp2aXJ0dWFsLnZlcmdl#1542660685.ygu.1#1831732584.yrts.1516372584#1874836173.yrtsi.1559476173#1541278287.ysl."
    "1#1559562564.yu.6815984621558243395; L=AU3cfWxQU3tcXHFad3UFVXBSegtXQUNaGBw/PTEjWFoFIwNeXA==.1564174633.13938.317263.68893845ed97c3a0f2f016f31b176c41; "
    "device_id=\"a8912d0225f8d6862d60b65af255494f94d4b0c4a\"; ys=ymrefl.D9A32DC84B3FC515#udn.cDp2aXJ0dWFsLnZlcmdl; "
    "sessionid2=3:1565200767.5.0.1564174633483:yh37FNGUmADrUhSBcBMAKg:4c.1|489625370.0.302.0:3|203289.3719******************************; "
    "_ym_isad=2; lastVisitedPage=%7B%22489625370%22%3A%22%2Fartist%2F2662089%2Ftracks%22%7D; cycada=mhD2sSFyprraC/+Sn/H8+vhEtTZfh0rsaE8ss1pvRHI=; "
    "active-browser-timestamp=1565213296088"
)
YA_SESS_ID_COOKIE = (
    "user-geo-region-id=28; ym_visorc_22663942=b; yandexuid=1413100441565039906; _ym_visorc_40504750=b; sso_status=sso.passport.yandex.ru:synchronized; "
    "ya_sess_id=noauth:1565039906; _ym_visorc_10630330=b; "
    "spravka=u96eNTY1MDM5OTIwO2k9MTc2LjEyMC4xOTUuMTA7dT0xNTY1MDM5OTIwMjEzMjI4MTg2O2g9OGQzYzUzMTZmNjlmYTRmMzU2N2NhNGNlODMwMzIzMGM"
)
YA_SESS_ID_MASKED_COOKIE = (
    "user-geo-region-id=28; ym_visorc_22663942=b; yandexuid=1413100441565039906; _ym_visorc_40504750=b; sso_status=sso.passport.yandex.ru:synchronized; "
    "ya_sess_id=******************************; _ym_visorc_10630330=b; "
    "spravka=u96eNTY1MDM5OTIwO2k9MTc2LjEyMC4xOTUuMTA7dT0xNTY1MDM5OTIwMjEzMjI4MTg2O2g9OGQzYzUzMTZmNjlmYTRmMzU2N2NhNGNlODMwMzIzMGM"
)
SESSGUARD_COOKIE = (
    "PHPSESSID=9kjbrj36snavjf30j2ipoatkh2; yandex_gid=11071; my_perpages=%5B%5D; "
    "sessguard=3:1560790262.5.0.150790262792:yh37FQ:82.1|888164031.-1.3|209836.840232.pe5iYh7FgIYXB4F6zVwMSA3Jvjc"
)
SESSGUARD_MASKED_COOKIE = (
    "PHPSESSID=9kjbrj36snavjf30j2ipoatkh2; yandex_gid=11071; my_perpages=%5B%5D; "
    "sessguard=3:1560790262.5.0.150790262792:yh37FQ:82.1|888164031.-1.3|209836.8402******************************"
)
REGULAR_COOKIE_VALUE = (
    "yandexuid=565457051551953889; yp=1867313944.udn.cDp3am9xZm5sc2Jz; "
    "ys=udn.cDp3am9xZm5sc2Jz#wprid.1565017029335198-336990133381799491900035-man1-3586"
)

CBB_SYNC_PERIOD = 1
CBB_BLOCK_1 = 77
CBB_BLOCK_2 = 42
CBB_BAN_1 = 777
CBB_BAN_2 = 888
CBB_MARK = 999
CBB_MARK_LOG_ONLY = 1111
CBB_FLAG_DEGRADATION = 661
CBB_FLAG_NOT_BANNED = 499


class TestLogs(AntirobotTestSuite):
    options = {
        "cgi_secrets": ["oauth_token"],
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "cbb_re_flag": [CBB_BLOCK_1, CBB_BLOCK_2],
        "cbb_captcha_re_flag": [CBB_BAN_1, CBB_BAN_2],
        "cbb_re_mark_flag": CBB_MARK,
        "cbb_re_mark_log_only_flag": CBB_MARK_LOG_ONLY,
        "random_events_fraction": 1,
    }

    global_config = {
        "rules": [{"id": 0, "cbb": [], "yql": ["has_cookie_my == 1"]}],
        "mark_rules": [],
    }

    num_old_antirobots = 0

    def setup_method(self, method):
        super().setup_method(method)
        self.unified_agent.pop_event_logs()

    def create_request(self):
        return Fullreq(REGULAR_SEARCH, headers={IP_HEADER: GenRandomIP()})

    def test_daemon_log_emits_entries_equal_to_requests(self):
        requests_count = 123
        for _ in range(requests_count):
            self.antirobot.send_request(self.create_request())

        collected_entries = 0
        while collected_entries != requests_count:
            events = self.unified_agent.pop_daemon_logs()
            collected_entries += len(events)
            time.sleep(0.1)

    def get_last_cookie_in_logs(self):
        return self.unified_agent.wait_event_logs(["TRequestData"])[0].Event.Data.split("\r\n")[4]

    def get_last_ban_reasons_in_logs(self):
        return self.unified_agent.wait_event_logs(["TCaptchaRedirect"])[0].Event.BanReasons.Yql

    @pytest.mark.parametrize("sent_cookie, expected_cookie", [
        (SESSION_ID_COOKIE, SESSION_ID_MASKED_COOKIE),
        (SESSIONID2_COOKIE, SESSIONID2_MASKED_COOKIE),
        (YA_SESS_ID_COOKIE, YA_SESS_ID_MASKED_COOKIE),
        (SESSGUARD_COOKIE, SESSGUARD_MASKED_COOKIE),
    ])
    def test_check_session_cookies_hidden_in_event_log(self, sent_cookie, expected_cookie):
        request = Fullreq(
            REGULAR_SEARCH,
            headers={IP_HEADER: GenRandomIP(), "Cookie": sent_cookie},
        )
        self.send_request(request)

        assert self.get_last_cookie_in_logs() == "Cookie: " + expected_cookie

    def test_check_regular_cookie_unspoiled_in_event_log(self):
        request = Fullreq(
            REGULAR_SEARCH,
            headers={IP_HEADER: GenRandomIP(), "Cookie": REGULAR_COOKIE_VALUE},
        )
        self.send_request(request)

        assert (
            self.get_last_cookie_in_logs() == "Cookie: " + REGULAR_COOKIE_VALUE
        )

    def test_check_cookie_header_case_insensitive(self):
        request_capitalized = Fullreq(
            REGULAR_SEARCH,
            headers={IP_HEADER: GenRandomIP(), "CoOkIe": REGULAR_COOKIE_VALUE},
        )
        self.send_request(request_capitalized)
        assert (
            self.get_last_cookie_in_logs() == "Cookie: " + REGULAR_COOKIE_VALUE
        )

        request = Fullreq(
            REGULAR_SEARCH,
            headers={IP_HEADER: GenRandomIP(), "Cookie": REGULAR_COOKIE_VALUE},
        )
        self.send_request(request)
        assert (
            self.get_last_cookie_in_logs() == "Cookie: " + REGULAR_COOKIE_VALUE
        )

    def test_ban_reasons(self):
        ip = GenRandomIP()

        def ping():
            self.send_fullreq("http://yandex.ru/search?text=xyz", headers={
                IP_HEADER: ip,
                "Cookie": "my=1234",
            })

        ping()
        self.unified_agent.get_last_event_in_daemon_logs()

        ping()
        assert self.get_last_ban_reasons_in_logs()

    @pytest.mark.parametrize("url", (
        "http://yandex.ru/search?text=cats&oauth_token=SECRET",
        "http://yandex.ru/search?oauth_token=SECRET&text=dogs&woof&bark",
        "http://yandex.ru/",
        "http://yandex.ru/?&",
    ))
    def test_url_mask(self, url):
        self.send_fullreq(url, headers={
            "Referer": url,
            IP_HEADER: GenRandomIP(),
        })

        expected_url = url[len("http://yandex.ru"):].replace("SECRET", "")

        expected_get_line = f"GET {expected_url} HTTP/1.1"
        assert self.get_last_get_line_in_logs() == expected_get_line

        event = self.get_last_event_in_daemon_logs()
        assert event["req_url"] == expected_url

    def get_last_get_line_in_logs(self):
        return self.unified_agent.wait_event_logs(["TRequestData"])[0].Event.Data.split("\r\n")[0]

    def test_block_reason(self):
        (
            rule_id_mrrobot,
            rule_id_parser,
            rule_id_bumblebee,
            rule_id_bee,
        ) = random_segment(4, 2**32)

        self.cbb.add_text_block(CBB_BLOCK_1, "cgi=/.*mrrobot.*/", rule_id=rule_id_mrrobot)
        self.cbb.add_text_block(CBB_BLOCK_1, "cgi=/.*parser.*/", rule_id=rule_id_parser)
        self.cbb.add_text_block(CBB_BLOCK_2, "cgi=/.*bumblebee.*/", rule_id=rule_id_bumblebee)
        self.cbb.add_text_block(CBB_BLOCK_2, "cgi=/.*bee.*/", rule_id=rule_id_bee)
        time.sleep(CBB_SYNC_PERIOD + 1)

        def check(query, reasons):
            self.send_fullreq(
                f"http://yandex.ru/search?text={query}",
                headers={IP_HEADER: GenRandomIP()},
            )

            assert self.get_last_block_reasons() == reasons

        check("mrrobot", {f"{CBB_BLOCK_1}#{rule_id_mrrobot}"})
        check("parser", {f"{CBB_BLOCK_1}#{rule_id_parser}"})
        check("bumblebee", {f"{CBB_BLOCK_2}#{rule_id_bee}", f"{CBB_BLOCK_2}#{rule_id_bumblebee}"})

    def get_last_block_reasons(self):
        s = self.unified_agent.get_last_event_in_daemon_logs()["block_reason"]
        return set(s[len("CBB_"):].split("_"))

    def test_cbb_ban_reasons(self):
        rule_id_mrrobot, rule_id_bee = random_segment(2, 2**32)

        self.cbb.add_text_block(CBB_BAN_1, "cgi=/.*mrrobot.*/", rule_id=rule_id_mrrobot)
        self.cbb.add_text_block(CBB_BAN_2, "cgi=/.*bee.*/", rule_id=rule_id_bee)
        time.sleep(CBB_SYNC_PERIOD + 1)

        ip = GenRandomIP()

        self.send_fullreq(
            "http://yandex.ru/search?text=mrrobot",
            headers={
                IP_HEADER: ip,
                ICOOKIE_HEADER: VALID_I_COOKIE,
            },
        )

        assert (
            self.unified_agent.get_last_event_in_daemon_logs()["cbb_ban_rules"] ==
            [f"{CBB_BAN_1}#{rule_id_mrrobot}"]
        )

        self.send_fullreq(
            "http://yandex.ru/search?text=bee",
            headers={
                IP_HEADER: ip,
                ICOOKIE_HEADER: VALID_I_COOKIE,
            },
        )

        event = self.unified_agent.get_last_event_in_daemon_logs()
        assert event["prev_cbb_ban_rules"] == [f"{CBB_BAN_1}#{rule_id_mrrobot}"]
        assert event["cbb_ban_rules"] == [f"{CBB_BAN_2}#{rule_id_bee}"]

        log = self.unified_agent.get_last_cacher_log()
        assert log.cbb_ban_rules == [f"{CBB_BAN_2}#{rule_id_bee}"]

    def test_cbb_mark_reasons(self):
        rule_id = random.randrange(2**32)
        self.cbb.add_text_block(CBB_MARK, "cgi=/.*markme.*/", rule_id=rule_id)
        time.sleep(CBB_SYNC_PERIOD + 1)

        self.antirobot.ping_search(query="markme")

        assert self.unified_agent.get_last_event_in_daemon_logs()["cbb_mark_rules"] == [f"{CBB_MARK}#{rule_id}"]
        assert self.unified_agent.get_last_cacher_log().cbb_mark_rules == [f"{CBB_MARK}#{rule_id}"]

    def test_cbb_mark_log_only_reasons(self):
        rule_id_mark, rule_id_log = random_segment(2, 2**32)

        self.cbb.add_text_block(CBB_MARK, "cgi=/.*markme.*/", rule_id=rule_id_mark)
        self.cbb.add_text_block(CBB_MARK_LOG_ONLY, "cgi=/.*markme.*/", rule_id=rule_id_log)
        time.sleep(CBB_SYNC_PERIOD + 1)

        self.antirobot.ping_search(query="markme")

        assert set(self.unified_agent.get_last_event_in_daemon_logs()["cbb_mark_rules"]) == set([f"{CBB_MARK_LOG_ONLY}#{rule_id_log}", f"{CBB_MARK}#{rule_id_mark}"])


class TestLogsWithoutBansByFactors(AntirobotTestSuite):
    options = {
        "DisableBansByFactors": 1,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "cbb_captcha_re_flag": [CBB_BAN_1],
        "random_events_fraction": 1,
    }

    def test_exp_bin_log(self):
        rule_id = random.randrange(2**32)
        self.cbb.add_text_block(CBB_BAN_1, "cgi=/.*exp_web.*/;exp_bin=3", rule_id=rule_id)
        time.sleep(CBB_SYNC_PERIOD + 1)

        def wait():
            response = self.send_fullreq(
                "http://yandex.ru/search?text=exp_web",
                headers={IP_HEADER: GenRandomIP()},
            )
            return IsCaptchaRedirect(response)

        asserts.AssertEventuallyTrue(wait)

        assert (
            self.unified_agent.get_last_event_in_daemon_logs()["cbb_ban_rules"] ==
            [f"{CBB_BAN_1}#{rule_id}#bin=3"]
        )


class TestEventLogs(AntirobotTestSuite):
    options = {
        'random_events_fraction@collections': 1,
        'random_events_fraction@web': 0,
        'random_factors_fraction': 0,
        'additional_factors_fraction@web': 1,
    }

    def test_event_logs_expand(self):
        headers = {
            "X-Forwarded-For": "2a00:1fa0:869d:63a:1d68:7486:4c71:2cde",
            "X-Forwarded-For-Y": "2a00:1fa0:869d:63a:1d68:7486:4c71:2cde",
            "X-Forwarded-Proto": "https",
            "X-HTTPS-Request": "yes",
            "X-Req-Id": "1628859850754103-6091842193416643686-vla1-2700-vla-l7-balancer-prod-8080-BAL",
            "X-Source-Port-Y": "51400",
            "X-Start-Time": "1628859850754103",
            "X-Yandex-HTTP-Version": "http2",
            "X-Yandex-HTTPS": "yes",
            "X-Yandex-HTTPS-Info": "handshake-time=0.043322s, no-tls-tickets, handshake-ts=1628859558, cipher-id=4867, protocol-id=772",
            "X-Yandex-ICookie": "4249130471626350067",
            "X-Yandex-ICookie-Info": "source=unknown",
            "X-Yandex-IP": "2a02:6b8:a::a",
            "X-Yandex-Ja3": "771,4867-4865-4866-52393-52392-49195-49199-49196-49200-49171-49172-156-157-47-53,0-23-65281-10-11-35-16-5-13-18-51-45-43-21,29-23-24,0",
            "X-Yandex-Ja4": "1027-2052-1025-1283-2053-1281-2054-1537,,772-771-770-769,h2-http/1.1,29,1",
            "X-Yandex-LoginHash": "704",
            "X-Yandex-P0f": "6:51+13:0:0:8192,0::bad:0",
            "X-Yandex-RandomUID": "3166436861628859850",
            "X-Yandex-TCP-Info": "v=2; rtt=0.054391s; rttvar=0.028476s; snd_cwnd=122; total_retrans=0",
            "X-Antirobot-Experiments": "no",
            "Y-User-Agent": "Mozilla/5.0 ( ; ) AppleWebKit/537.36 (KHTML, like Gecko)  Not;A Brand/99 Chrome/91 YaBrowser/91  Safari/537.36",
            "Y-User-Agent-Proto": "GgZXZWJLaXQiCENocm9taXVtMghDaHJvbWl1bWIEAAAAAGoEAAAAAHIEAAAAAHoEAAAAAIIBBAAAAACKAQQAAAAAkgEEAAAAAJoBBAAAAACiAQQAAAAAqgEEAAAAAA==",
            "accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9",
            "accept-encoding": "gzip, deflate, br",
            "accept-language": "ru,en;q=0.9",
            "cookie": "sae=0:0E4A0C7D-B75F-4829-ADD9-49C02E2A5C2F:p:21.6.3.756:w:d:RU:20150823; is_gdpr=0; is_gdpr_b=CIL8NhC/PygC; ymex=1944219611.yrts.1628859611" +
                      "; gdpr=0; _ym_uid=1626350422159560183; _ym_isad=2; Session_id=3:1628859644.5.0.1628859644001:3ixxTIZ0aB06Bp2GoB8AKg:4.1|97052655.0.2|3:2391" +
                      "45.45587.DBbB-H6fgygnjMM0IVQNcpFNM9I; sessionid2=3:1628859644.5.0.1628859644001:3ixxTIZ0aB06Bp2GoB8AKg:4.1|97052655.0.2|3:239145.45587.DBbB" +
                      "-H6fgygnjMM0IVQNcpFNM9I; L=fFcCe1FzYQcBQnFYf1VVbABweFBfZEIKIyMUNF1fNww0QT8A.1628859644.14696.352476.714de7c0dab26f352b25a260e903b6e8; yande" +
                      "x_login=oleg.hrebtov; amcuid=7886562451628859774; ys=newsca.native_cache#ybzcc.ru#def_bro.0#udn.cDpvbGVnLmhyZWJ0b3Y%3D#mclid.1955454; mda=0" +
                      "; yandex_gid=2; _ym_d=1628859798; yandexuid=4249130471626350067; font_loaded=YSv1; my=YwA=; i=XNCi+VSpra6D+22mdXHFVqtSY9bfE6LOxh/OBdYHgcFMB" +
                      "mTkQHhOyCMiXk+k+/FrV3bsCCyfDbDD5JO1MdWcwf8lcL4=; yabs-frequency=/5/0000000000000000/z0npS9G00018GY62OK5jXW0004X28m00/; _yasc=ggjZMfQ3JxL707" +
                      "YoReGwS5tisnh193ST1g0pMkyCxWpvuvTFSdvt2EZmm08=; yp=1660395800.cld.1955450#1660395800.brd.6400000000#1944219644.udn.cDpvbGVnLmhyZWJ0b3Y%3D#1" +
                      "631451797.ygu.1#1629119005.clh.1955454#1628946205.yu.1828270261628859579#1644627807.szm.1:1366x768:1366x652#1629032612.gpauto.60_046375:30_" +
                      "393600:140:0:1628859802#1631538215.csc.1",
            "device-memory": "4",
            "downlink": "3.3",
            "dpr": "1",
            "ect": "4g",
            "host": "yandex.ru",
            "referer": "https://yandex.ru/",
            "rtt": "200",
            "sec-ch-ua": "Not;A Brand\";v=\"99\", \"Yandex\";v=\"91\", \"Chromium\";v=\"91",
            "sec-ch-ua-mobile": "?0",
            "sec-fetch-dest": "document",
            "sec-fetch-mode": "navigate",
            "sec-fetch-site": "same-origin",
            "sec-fetch-user": "?1",
            "serp-rendering-experiment": "snippet-with-header",
            "upgrade-insecure-requests": "1",
            "user-agent": "Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.135 YaBrowser/21.6.3.756 Yowser/2.5 Safari/537.36",
            "viewport-width": "1366",
            "probability": "papamamasestrabrat",
        }

        self.unified_agent.pop_event_logs()

        self.antirobot.send_request(Fullreq("http://yandex.ru/collections", headers=headers))
        headers["probability"] = "papamamabratsestra"
        assert not IsCaptchaRedirect(self.antirobot.send_request(Fullreq("http://yandex.ru/search", headers=headers)))
        time.sleep(5)

        events = self.unified_agent.get_event_logs("TRequestData")
        factors = self.unified_agent.get_event_logs("TAntirobotFactors")
        assert len([event for event in events if "papamamasestrabrat" in event.Event.Data]) == 1
        assert len([event for event in events if "papamamabratsestra" in event.Event.Data]) == 0
        assert len(factors) == 1


class TestDaemonLogs3(AntirobotTestSuite):
    options = {
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "cbb_re_flag": [CBB_BLOCK_1, CBB_BLOCK_2],
        "cbb_captcha_re_flag": [CBB_BAN_1, CBB_BAN_2],
        "CbbFlagDegradation": CBB_FLAG_DEGRADATION,
        "CbbFlagNotBanned": CBB_FLAG_NOT_BANNED,
    }

    captcha_args = ["--generate", "correct"]

    def test_logs_has_same_timestamp(self):
        self.send_fullreq(
            "http://yandex.ru/search",
            headers={IP_HEADER: GenRandomIP()}
        )

        last_cacher_log = self.unified_agent.get_last_cacher_log()
        last_processor_log = self.unified_agent.get_last_processor_log_with_unikey()

        assert last_cacher_log.unique_key == last_processor_log.unique_key
        assert last_cacher_log.timestamp == last_processor_log.timestamp

    def test_cacher_log_verdict(self):
        self.send_fullreq(
            "http://yandex.ru/search",
            headers={IP_HEADER: GenRandomIP()}
        )
        last_cacher_log = self.unified_agent.get_last_cacher_log()
        assert last_cacher_log.verdict == 'NEUTRAL'

        self.cbb.add_text_block(CBB_BLOCK_1, "cgi=/.*must_block.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)
        resp = self.send_fullreq(
            "http://yandex.ru/search?text=must_block",
            headers={IP_HEADER: GenRandomIP()}
        )
        last_cacher_log = self.unified_agent.get_last_cacher_log()
        asserts.AssertBlocked(resp)
        assert last_cacher_log.verdict == 'BLOCKED'

        self.cbb.add_text_block(CBB_BAN_1, "cgi=/.*must_ban.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)
        resp = self.send_fullreq(
            "http://yandex.ru/search?text=must_ban",
            headers={IP_HEADER: GenRandomIP()}
        )
        last_cacher_log = self.unified_agent.get_last_cacher_log()
        asserts.AssertCaptchaRedirect(resp)
        assert last_cacher_log.verdict == 'ENEMY'

    def test_logging_blocked_req(self):
        rule_id = random.randrange(2**32)
        self.cbb.add_text_block(CBB_BLOCK_1, "cgi=/.*must_block.*/", rule_id=rule_id)
        time.sleep(CBB_SYNC_PERIOD + 1)

        resp = self.send_fullreq(
            "http://yandex.ru/search?text=must_block",
            headers={IP_HEADER: GenRandomIP()}
        )

        last_cacher_log = self.unified_agent.get_last_cacher_log().GetDict()
        asserts.AssertBlocked(resp)
        assert last_cacher_log['verdict'] == 'BLOCKED'
        assert 'block_reason' in last_cacher_log and last_cacher_log['block_reason'] == f'CBB_{CBB_BLOCK_1}#{rule_id}'
        assert 'cacher_blocked' in last_cacher_log and last_cacher_log['cacher_blocked'] is True
        assert 'cacher_block_reason' in last_cacher_log and last_cacher_log['cacher_block_reason'] == f'CBB_{CBB_BLOCK_1}#{rule_id}'

    def test_logging_banned_req(self):
        rule_id = random.randrange(2**32)
        self.cbb.add_text_block(CBB_BAN_1, "cgi=/.*must_ban.*/", rule_id=rule_id)
        time.sleep(CBB_SYNC_PERIOD + 1)

        resp = self.send_fullreq(
            "http://yandex.ru/search?text=must_ban",
            headers={IP_HEADER: GenRandomIP()}
        )

        last_cacher_log = self.unified_agent.get_last_cacher_log().GetDict()
        asserts.AssertCaptchaRedirect(resp)
        assert last_cacher_log['verdict'] == 'ENEMY'
        assert 'block_reason' in last_cacher_log and len(last_cacher_log['block_reason']) == 0
        assert 'cacher_blocked' in last_cacher_log and last_cacher_log['cacher_blocked'] is False
        assert 'cacher_block_reason' in last_cacher_log and last_cacher_log['cacher_block_reason'] == f'CBB_{CBB_BAN_1}#{rule_id}'

    def test_logging_degradation(self):
        self.send_fullreq(
            "http://yandex.ru/search?query=degradead_web",
            headers={IP_HEADER: GenRandomIP()},
        )
        assert self.unified_agent.get_last_cacher_log().degradation is False

        self.cbb.add_text_block(CBB_FLAG_DEGRADATION, "cgi=/.*degradead_web.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        self.send_fullreq(
            "http://yandex.ru/search?query=degradead_web",
            headers={IP_HEADER: GenRandomIP()},
        )
        assert self.unified_agent.get_last_cacher_log().degradation is True

    def test_logging_cbb_whitelist(self):
        self.cbb.add_text_block(CBB_BAN_1, "cgi=/.*ban_or_not_to_ban.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        resp = self.send_fullreq(
            "http://yandex.ru/search?query=ban_or_not_to_ban",
            headers={IP_HEADER: GenRandomIP()},
        )
        last_cacher_log = self.unified_agent.get_last_cacher_log()
        assert last_cacher_log.cbb_whitelist is False
        asserts.AssertCaptchaRedirect(resp)
        assert last_cacher_log.verdict == 'ENEMY'

        self.cbb.add_text_block(CBB_FLAG_NOT_BANNED, "cgi=/.*ban_or_not_to_ban.*/")
        time.sleep(CBB_SYNC_PERIOD + 1)

        resp = self.send_fullreq(
            "http://yandex.ru/search?query=ban_or_not_to_ban",
            headers={IP_HEADER: GenRandomIP()},
        )
        last_cacher_log = self.unified_agent.get_last_cacher_log()
        assert last_cacher_log.cbb_whitelist is True
        assert last_cacher_log.verdict == 'NEUTRAL'
