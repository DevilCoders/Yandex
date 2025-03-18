import time
from itertools import cycle

import pytest

from antirobot.daemon.arcadia_test import util
from antirobot.daemon.arcadia_test.util import asserts
from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite

REQ_SEARCH = "http://yandex.ru/yandsearch?text=blah"
REQ_NON_SEARCH = "http://yandex.ru/abc"

RPS_SMOOTH_FACTOR = 0.6
RPS_THRESHOLD = 10.0

RPS_ULTRA_HIGH = RPS_THRESHOLD * 3
RPS_HIGH = RPS_THRESHOLD + 1
RPS_SUB_HIGH = RPS_THRESHOLD - 1
RPS_OVER_NORMAL = 4.0
RPS_NORMAL = 2.0
RPS_VERY_SMALL = 0.2
RPS_ZERO = 0

# all durations are in seconds
TEST_DURATION = 10
BLOCK_PERIOD = 5
AMNESTY_LAG = 1

DURATION_NORMAL = 5
DURATION_SHORT = 2
DURATION_LONG = 20

LONG_TEST_TIMEOUT = 120
CBB_SYNC_PERIOD = 1


class ConstRpsGen:
    def __init__(self, rps, durationInSec, startTime=time.time()):
        self.rps = rps
        self.now = 0
        self.step = (1.0 / rps) if rps > 0 else durationInSec
        self.startTime = startTime
        self.endTime = self.startTime + durationInSec

    def __iter__(self):
        self.now = self.startTime
        return self

    def __next__(self):
        self.now += self.step
        if self.now > self.endTime:
            raise StopIteration

        return self.now


class RpsPlan:
    def __init__(self, rpsWithDurationList):
        self.rpsWithDurationList = rpsWithDurationList

    def __iter__(self):
        tim = time.time()
        for (rps, duration) in self.rpsWithDurationList:
            rpsGen = ConstRpsGen(rps, duration, tim)
            tim += duration
            for step in rpsGen:
                yield step


REQ_CATEGORIES = [
    lambda self: self.create_search_req_with_spravka_maker(),
    lambda self: self.create_non_search_req_maker(),
    lambda self: self.create_non_search_req_with_spravka_maker(),
]


class TestAntiDDos(AntirobotTestSuite):
    options = {
        "DisableBansByFactors": 1,
        "DDosSmoothFactor": RPS_SMOOTH_FACTOR,
        "DDosRpsThreshold": RPS_THRESHOLD,
        "DDosFlag1BlockEnabled": 1,
        "DDosFlag2BlockEnabled": 1,
        "DDosFlag1BlockPeriod": "%ds" % BLOCK_PERIOD,
        "DDosFlag2BlockPeriod": "%ds" % BLOCK_PERIOD,
        "RemoveExpiredPeriod": AMNESTY_LAG,
        "CbbSyncPeriod": CBB_SYNC_PERIOD,
        "CbbApiTimeout": 2,
        "InitialChinaRedirectEnabled": False,
    }
    num_old_antirobots = 0

    def gen_spravkas(self, count):
        "Returns a list of generated spravka"

        resp = self.send_request("/admin?action=getspravka&count=%d&domain=yandex.ru" % count)
        return resp.read().decode().strip().split("\n")

    def gen_spravka(self):
        return self.gen_spravkas(1)[0]

    def exec_rps_plan(self, rpsPlan, requestMakeFunc):
        for timestamp in rpsPlan:
            currentTime = time.time()
            if currentTime < timestamp:
                time.sleep(timestamp - currentTime)
            resp = self.send_request(requestMakeFunc(timestamp))

        return resp

    def create_some_req_maker(
        self,
        reqStr,
        multiIP=False,
        useSpravka=False,
        multiSpravka=False,
        ipAddr=None,
    ):
        "Returns a callable that creates a request with a given timestamp"

        if multiIP:
            ip = util.GenRandomIP
        else:
            ipAddr = ipAddr or util.GenRandomIP()

            def ip():
                return ipAddr

        spravka = None
        if useSpravka:
            if multiSpravka:
                spravkas = cycle(self.gen_spravkas(500))

                def spravka():
                    return next(spravkas)

            else:
                dummySpravka = self.gen_spravka()

                def spravka():
                    return dummySpravka

        def req_maker(timestamp):
            "Actual request maker"

            headers = {
                "X-Start-Time": str(int(timestamp * 1e6)),
                "X-Forwarded-For-Y": ip(),
            }
            if spravka:
                headers["Cookie"] = "spravka=%s" % spravka()

            return util.Fullreq(reqStr, headers=headers)

        return req_maker

    def create_search_req_with_spravka_maker(self, ipAddr=None):
        return self.create_some_req_maker(
            REQ_SEARCH,
            multiIP=False,
            useSpravka=True,
            multiSpravka=True,
            ipAddr=ipAddr,
        )

    def create_non_search_req_maker(self):
        return self.create_some_req_maker(
            REQ_NON_SEARCH, multiIP=False, useSpravka=False
        )

    def create_non_search_req_with_spravka_maker(self):
        return self.create_some_req_maker(
            REQ_NON_SEARCH, multiIP=True, useSpravka=True, multiSpravka=False
        )

    @pytest.mark.parametrize("reqMakerFabric", REQ_CATEGORIES)
    def test_continuous_high_rps(self, reqMakerFabric):
        rpsPlan = RpsPlan([(RPS_HIGH, DURATION_NORMAL)])

        reqMaker = reqMakerFabric(self)
        asserts.AssertBlocked(self.exec_rps_plan(rpsPlan, reqMaker))

    @pytest.mark.parametrize("reqMakerFabric", REQ_CATEGORIES)
    def test_high_rps_with_pauses(self, reqMakerFabric):
        plan = [
            (RPS_ULTRA_HIGH, DURATION_SHORT),
            (RPS_ZERO, DURATION_NORMAL),
            (RPS_ULTRA_HIGH, DURATION_SHORT),
            (RPS_ZERO, DURATION_NORMAL),
            (RPS_ULTRA_HIGH, DURATION_SHORT),
            (RPS_ZERO, DURATION_NORMAL),
            (RPS_ULTRA_HIGH, DURATION_SHORT),
        ]

        reqMaker = reqMakerFabric(self)
        asserts.AssertBlocked(self.exec_rps_plan(RpsPlan(plan), reqMaker))

    @pytest.mark.parametrize("reqMakerFabric", REQ_CATEGORIES)
    def test_high_rps_with_low_rps(self, reqMakerFabric):
        plan = [
            (RPS_ULTRA_HIGH, DURATION_SHORT),
            (RPS_NORMAL, DURATION_SHORT),
            (RPS_ULTRA_HIGH, DURATION_SHORT),
            (RPS_NORMAL, DURATION_SHORT),
            (RPS_ULTRA_HIGH, DURATION_SHORT),
            (RPS_NORMAL, DURATION_SHORT),
            (RPS_ULTRA_HIGH, DURATION_SHORT),
        ]

        reqMaker = reqMakerFabric(self)
        asserts.AssertBlocked(self.exec_rps_plan(RpsPlan(plan), reqMaker))

    @pytest.mark.parametrize("reqMakerFabric", REQ_CATEGORIES)
    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_continue_polling_after_block(self, reqMakerFabric):
        plan = [(RPS_HIGH, DURATION_NORMAL)]

        reqMaker = reqMakerFabric(self)
        asserts.AssertBlocked(self.exec_rps_plan(RpsPlan(plan), reqMaker))

        time.sleep(BLOCK_PERIOD / 2)

        asserts.AssertBlocked(self.send_request(reqMaker(time.time())))

    @pytest.mark.parametrize("reqMakerFabric", REQ_CATEGORIES)
    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_wait_for_unblock_then_polling_again(self, reqMakerFabric):
        plan = [(RPS_HIGH, DURATION_NORMAL)]

        searchreq_maker = reqMakerFabric(self)

        def fire():
            return self.exec_rps_plan(RpsPlan(plan), searchreq_maker)

        asserts.AssertBlocked(fire())

        # waiting for unblock
        time.sleep(BLOCK_PERIOD + 2 * AMNESTY_LAG)
        asserts.AssertNotBlocked(self.send_request(searchreq_maker(time.time())))

        # then fire again
        asserts.AssertBlocked(fire())

    @pytest.mark.parametrize("reqMakerFabric", REQ_CATEGORIES)
    def test_continuous_low_rps(self, reqMakerFabric):
        plan = [(RPS_NORMAL, DURATION_LONG)]

        asserts.AssertNotBlocked(self.exec_rps_plan(RpsPlan(plan), reqMakerFabric(self)))

    @pytest.mark.parametrize("reqMakerFabric", REQ_CATEGORIES)
    def test_continuous_low_rps_with_high_peaks(self, reqMakerFabric):
        plan = [
            (RPS_NORMAL, DURATION_LONG),
            (RPS_HIGH, DURATION_SHORT),
            (RPS_NORMAL, DURATION_LONG),
            (RPS_HIGH, DURATION_SHORT),
            (RPS_NORMAL, DURATION_NORMAL),
        ]

        asserts.AssertNotBlocked(self.exec_rps_plan(RpsPlan(plan), reqMakerFabric(self)))

    @pytest.mark.parametrize("reqMakerFabric", REQ_CATEGORIES)
    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_continuous_low_rps_after_block(self, reqMakerFabric):
        planUntilBlock = [(RPS_HIGH, DURATION_NORMAL)]
        planAfterBlock = [(RPS_NORMAL, DURATION_LONG)]

        reqMaker = reqMakerFabric(self)
        asserts.AssertBlocked(self.exec_rps_plan(RpsPlan(planUntilBlock), reqMaker))

        time.sleep(BLOCK_PERIOD + 2 * AMNESTY_LAG)
        asserts.AssertNotBlocked(self.send_request(reqMaker(time.time())))

        asserts.AssertNotBlocked(self.exec_rps_plan(RpsPlan(planAfterBlock), reqMaker))

    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_not_robot_after_unblock_on_search_req(self):
        reqMaker = self.create_search_req_with_spravka_maker()
        planUntilBlock = [(RPS_HIGH, DURATION_NORMAL)]

        asserts.AssertBlocked(self.exec_rps_plan(RpsPlan(planUntilBlock), reqMaker))
        time.sleep(BLOCK_PERIOD + 2 * AMNESTY_LAG)

        resp = self.send_request(reqMaker(time.time()))
        asserts.AssertNotBlocked(resp)
        asserts.AssertNotRobotResponse(resp)

    @pytest.mark.timeout(LONG_TEST_TIMEOUT)
    def test_unblock_after_add_addr_to_nonblocking_list(self):
        ipToTest = util.GenRandomIP()
        reqMaker = self.create_search_req_with_spravka_maker(ipAddr=ipToTest)
        planUntilBlock = [(RPS_HIGH, DURATION_NORMAL)]

        asserts.AssertBlocked(self.exec_rps_plan(RpsPlan(planUntilBlock), reqMaker))

        self.cbb.add_block(
            int(self.antirobot.dump_cfg()["CbbFlagNonblocking"]), ipToTest, ipToTest, None
        )
        time.sleep(CBB_SYNC_PERIOD + 1)

        asserts.AssertNotBlocked(self.exec_rps_plan(RpsPlan(planUntilBlock), reqMaker))
