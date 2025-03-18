import random
import socket
import urllib.request
import urllib.parse

from google.protobuf.internal.decoder import _DecodeVarint32
from google.protobuf.internal.encoder import _VarintBytes


# from https://a.yandex-team.ru/arc//trunk/arcadia/library/cpp/lcookie/lcookie_ut.cpp?rev=3578849
VALID_L_COOKIE = "SG0TWlVvB1J9DXZXW0AHWw9neXB1bncCEjgrJkYPUmdMHgIvKx1YAicOERR7XhghDQIPElM6dQc9JwtaXTsDHw==.1311989535.9065.287854.22faf2292da9c7dbabc30e8512cc098d"

# newly registered test user
VALID_I_COOKIE = 520308641538517137
VALID_I_COOKIE_TIMESTAMP = 1538517137
TRUSTED_I_COOKIE = 2520468631462122703
VALID_FUID = "5bb3eae646022036.G5A6ManPomA8VG1hVT3-nXYNqSPmof4U2Pg1-bTwE7uhFPWCoVWcMpfiEYRLPFPC7WCXsg-hS8CvzBVpoPCNpV6jtvgPY80SfC7_MytleX34VRuuiM8iCdqFg28DTaK3"

MY_COOKIE_RUS = "YycCAAEA"
MY_COOKIE_ENG = "YycCAAMA"
MY_COOKIE_UKR = "YycCAAIA"
MY_COOKIE_KAZ = "YycCAAQA"
MY_COOKIE_BEL = "YycCAAUA"
MY_COOKIE_TAT = "YycCAAYA"
MY_COOKIE_UZB = "YycCABcA"
MY_COOKIE_TUR = "YycCAAgA"

IP_HEADER = "X-Forwarded-For-Y"
ICOOKIE_HEADER = "X-Yandex-ICookie"


def IsResponseForUser(resp):
    header = "X-ForwardToUser-Y"
    return header in resp.info() and resp.info()[header] == "1"


def IsResponseForBalancer(resp):
    header = "X-ForwardToUser-Y"
    return header in resp.info() and resp.info()[header] == "0"


def IsCaptchaRedirect(resp):
    if resp.getcode() != 302:
        return False
    if not IsResponseForUser(resp):
        return False

    locHeader = "Location"
    if locHeader not in resp.info():
        return False

    location = resp.info()[locHeader]
    return urllib.parse.urlsplit(location).path == "/showcaptcha"


def IsBlockedResponse(resp):
    return resp.getcode() == 403


def GenRandomIP(v=4):
    if v == 4:
        return "%d.%d.%d.%d" % tuple([random.randint(0, 255) for _ in range(4)])
    if v == 6:
        return "2001:cafe:" + ":".join(("%x" % random.randint(0, 16**4) for i in range(6)))
    assert False, f"Invalid argument for GenRandomIP: {v}"


def MakeReqId():
    return "%d-%d" % tuple([random.randint(1, 10**9) for _ in range(2)])


class Fullreq:
    def __init__(self, req, headers={}, content="", method=None):
        XFFY = "X-Forwarded-For-Y"
        XReqId = "X-Req-Id"

        if isinstance(req, str):
            req = urllib.request.Request(req, method=method)
        assert isinstance(req, urllib.request.Request)

        if not req.has_header("Host"):
            req.add_header("Host", req.host)
        # Нам приходится вызывать capitalize(), потому что именно в таком виде имена
        # заголовков добавляются в словарь в add_header. При этом в has_header capitalize()
        # не вызывается при проверке принадлежности ключа словарю. См. исходники urllib.request.
        if not req.has_header(XFFY.capitalize()):
            req.add_header(XFFY, "127.0.0.1")
        if not req.has_header(XReqId.capitalize()):
            req.add_header(XReqId, MakeReqId())

        for k, v in headers.items():
            req.add_header(k, v)

        if len(content) > 0:
            req.add_header('Content-length', len(content))
            content = "\r\n" + content

        firstLine = ' '.join(
            [req.get_method(), req.selector, 'HTTP/1.1'])
        headers = ["%s: %s" % (k, v) for k, v in req.header_items()]

        parts = [firstLine] + headers + [content]
        if req.data:
            parts.append(req.data)

        self.Data = "\r\n".join(parts).encode()

    def GetRequestForHost(self, host):
        return urllib.request.Request(host + "/fullreq", data=self.Data)


class ProcessReq:
    def __init__(self, location, data=None):
        assert type(data) == bytes
        self.Data = data
        self.Location = location

    def GetRequestForHost(self, host):
        return urllib.request.Request(host + self.Location, data=self.Data)


def service_available(port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        return sock.connect_ex(("localhost", port)) == 0


class ProtoListIO:
    def __init__(self, proto_constructor):
        self.proto_constructor = proto_constructor

    def read(self, file_path):
        with open(file_path, "rb") as file:
            buffer = file.read()

        count = 0
        position = 0
        result = []
        while position < len(buffer):
            message_len, position = _DecodeVarint32(buffer, position)
            message = buffer[position: position + message_len]

            ban_action = self.proto_constructor()
            ban_action.ParseFromString(message)
            count += 1
            position += message_len
            result.append(ban_action)

        return result

    def write(self, file_path, data, header):
        with open(file_path, "wb") as out:
            out.write(_VarintBytes(header.ByteSize()) + header.SerializeToString())
            for message in data:
                out.write(_VarintBytes(message.ByteSize()) + message.SerializeToString())


def random_segment(segment_len, *args, **kwargs):
    base = random.randrange(*args, *kwargs)
    return list(range(base, base + segment_len))
