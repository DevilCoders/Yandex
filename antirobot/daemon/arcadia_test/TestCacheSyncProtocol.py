import time

import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    ProcessReq,
    GenRandomIP
)
from antirobot.idl import cache_sync_pb2 as cache_sync


NAMESPACE_IPV4 = 1

HOST_WEB = 1


def get_test_request(ip=None):
    if not ip:
        ip = GenRandomIP()
    return "\r\n".join(["GET /yandsearch?text=123 HTTP/1.1",
                        "Host: yandex.ru",
                        "X-Forwarded-For-Y: %s" % ip,
                        "X-Req-Id: 1375954266972097-8626638277124794355",
                        ""])


def get_message_with_corrupted_request():
    message = cache_sync.TMessage()
    message.Request.Request = "some_corrupted_data"
    return message


def get_message_with_corrupted_bans(request):
    message = cache_sync.TMessage()
    message.Request.Request = request
    message.Blocks.add().Data = b"some_corrupted_data"
    message.Blocks.add().Data = b"more_corrupted_data"
    return message


def get_message_with_corrupted_blocks(request):
    message = cache_sync.TMessage()
    message.Request.Request = request
    message.Bans.add().HostType = 9999
    message.Bans.add().HostType = 9999
    return message


def assert_greater(a, b):
    assert a > b


def ip_to_str(ip):
    parts = ip.to_bytes(4, byteorder="big")
    return ".".join(map(str, parts))


def str_to_ip(s):
    parts = map(int, s.split("."))
    return int.from_bytes(parts, byteorder="big")


class TestCacheSyncProtocol(AntirobotTestSuite):
    def make_process_req(self, data):
        process_location = self.antirobot.dump_cfg()["RequestForwardLocation"]
        return ProcessReq(process_location, data)

    def test_process_request_only(self):
        message = cache_sync.TMessage()
        message.Request.Request = get_test_request()
        message.Request.RequesterAddr = ""

        resp = self.antirobot.send_request(self.make_process_req(message.SerializeToString()))
        assert resp.getcode() == 200

    @pytest.mark.parametrize("corrupted_message", [
        # это сосиска в тесте
        "Сосиска".encode(),
        get_message_with_corrupted_request().SerializeToString(),
        get_message_with_corrupted_blocks(get_test_request(ip="192.168.1.1")).SerializeToString(),
        get_message_with_corrupted_bans(get_test_request(ip="192.168.1.2")).SerializeToString(),
    ])
    def test_process_corrupted_message(self, corrupted_message):
        resp = self.antirobot.send_request(self.make_process_req(corrupted_message))
        assert resp.getcode() == 400
        assert len(resp.read()) > 0

    @pytest.mark.parametrize("block_or_ban", [
        "ban",
        "block",
    ])
    def test_process_returns_actions(self, block_or_ban):
        ip = GenRandomIP()

        if block_or_ban == "block":
            self.antirobot.block(ip)
            message_key = "Blocks"
        else:
            self.antirobot.ban(ip)
            message_key = "Bans"

        message = cache_sync.TMessage()
        message.Request.Request = get_test_request(ip)

        resp = self.antirobot.send_request(self.make_process_req(message.SerializeToString()))
        assert resp.getcode() == 200

        message = cache_sync.TMessage.FromString(resp.read())
        actions = getattr(message, message_key)

        assert len(actions) > 0

        if block_or_ban == "ban":
            def is_our_ban(action):
                uid = action.Uid

                return (
                    ip_to_str(uid.IdLo) == ip and
                    uid.IdHi == 0 and
                    uid.Namespace == NAMESPACE_IPV4 and
                    uid.AggrLevel == 0 and
                    action.HostType == HOST_WEB and
                    action.MatrixnetExpirationTime > 0
                )

            assert any(map(is_our_ban, actions))

    def test_process_accepts_actions(self):
        ip = GenRandomIP()

        message = cache_sync.TMessage()
        message.Request.Request = get_test_request(ip)

        ban = message.Bans.add()
        ban.Uid.IdLo = str_to_ip(ip)
        ban.Uid.Namespace = NAMESPACE_IPV4
        ban.HostType = HOST_WEB
        ban.MatrixnetExpirationTime = int((time.time() + 3600) * 10**6)

        resp = self.antirobot.send_request(self.make_process_req(message.SerializeToString()))
        assert resp.getcode() == 200

        assert self.antirobot.is_banned(ip)

        next_message = cache_sync.TMessage()
        next_message.Request.Request = message.Request.Request
        next_resp = self.antirobot.send_request(self.make_process_req(next_message.SerializeToString()))
        assert resp.getcode() == 200

        recv_msg = cache_sync.TMessage.FromString(next_resp.read())

        def is_our_ban(action):
            uid = action.Uid

            return (
                ip_to_str(uid.IdLo) == ip and
                uid.IdHi == 0 and
                uid.Namespace == NAMESPACE_IPV4 and
                uid.AggrLevel == 0 and
                action.HostType == HOST_WEB and
                action.MatrixnetExpirationTime == ban.MatrixnetExpirationTime
            )

        assert any(map(is_our_ban, recv_msg.Bans))
