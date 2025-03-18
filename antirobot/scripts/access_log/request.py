import sys
import os
import time
import re
import urllib
from urlparse import urlsplit, parse_qsl, parse_qs

from Cookie import SimpleCookie, CookieError
from .subnet import subnet
import splitter

from antirobot.scripts.utils.filter_yandex import YANDEX_SUBNETS

yandex_net = subnet(*YANDEX_SUBNETS)

_numdoc_cookie = re.compile('(?:^|,|%2[cC])numdoc(?::|%3[aA])([0-9]+)')
_ys_path = re.compile('/+(?:(?:yand|m|site|school|family|touch||tel|large|cgi-bin/yand|xml)search|yandpage)/*$')
has_ys_prefix = lambda s: _ys_path.match(s) is not None
_spravka_in_yp = re.compile('\.spravka\.([^#\.]+)')


class const_property(object):
    def __get__(self, obj, type=None):
        value = self.meth(obj)
        setattr(obj, self.meth.__name__, value)
        return value

    def __init__(self, meth):
        self.meth = meth


class BaseRequest(object):
    @const_property
    def fields(self):
        pass

    @const_property
    def ip(self):
        pass

    @const_property
    def referer(self):
        pass

    @const_property
    def useragent(self):
        pass

    @const_property
    def http_status(self):
        pass

    @const_property
    def fuid(self):
        pass

    @const_property
    def is_ys(self):
        return has_ys_prefix(self.path)

    @const_property
    def url(self):
        pass

    @const_property
    def size(self):
        return '-'

    @const_property
    def method(self):
        pass

    # 'host,port'
    @const_property
    def host(self):
        pass

    @const_property
    def protocol(self):
        pass

    @const_property
    def headers(self):
        pass

    @const_property
    def time(self):
        pass

    @const_property
    def timestring(self):
        pass

    @const_property
    def reqid(self):
        return self._fields[self.Indices.REQID]

    @const_property
    def agent(self):
        return self._fields[self.Indices.USERAGENT]

    @const_property
    def cookies_raw(self):
        pass

    @const_property
    def cookies(self):
        cookies = SimpleCookie()

        try:
            s = self.cookies_raw
            if type(s) == type(u''):
                s = s.encode('utf8')
            cookies.load(s)
        except CookieError:
            pass

        return cookies

    @const_property
    def yandexuid(self):
        pass

    def _split_url(self):
        fields = 'netloc', 'path', 'query', 'fragment'
        parsed = urlsplit(self.url)
        [setattr(self, field, getattr(parsed, field)) for field in fields]
        return parsed

    @const_property
    def netloc(self):
        return self._split_url().netloc

    @const_property
    def path(self):
        return self._split_url().path

    @const_property
    def query(self):
        return self._split_url().query

    @const_property
    def fragment(self):
        return self._split_url().fragment

    @const_property
    def cgi(self):
        cgi = {}
        for (k, v) in parse_qsl(self.query, True):
            cgi.setdefault(k, v)
        return cgi

    @const_property
    def cgiex(self):
        return parse_qs(self.query, keep_blank_values=True)

    @const_property
    def numdoc(self, default=10):
        try:
            nd = int(self.cgi['numdoc'])
            if nd < 0 or nd > 50:
                return 10
            elif 0 < nd:
                return nd
        except (KeyError, ValueError):
            pass

        try:
            prefs = self.cookies['YX_SEARCHPREFS'].value
            nd = int(_numdoc_cookie.search(prefs).group(1))
            if 0 < nd <= 50:
                return nd
        except (KeyError, ValueError, AttributeError):
            pass

        return default

    def GetSpravkaStr(self):
        yp = self.cookies.get('yp')
        if yp:
            p = _spravka_in_yp.search(urllib.unquote(yp.value))
            if p:
                return urllib.unquote(p.group(1))

        spr = self.cookies.get('spravka')
        if spr:
            return spr.value

        return None

    def GetYandexLogin(self):
        login = self.cookies.get('yandex_login')
        if login:
            return login.value

        return None

    def __str__(self):
        pass


class RequestString(BaseRequest):
    @staticmethod
    def float_convert(s):
        try:
            return float(s)
        except:
            return None

    def CheckValid(self):
        return True

    @staticmethod
    def Splitter(rec):
        return None

    def __init__(self, rec):
        req = getattr(rec, 'value', rec)
        self.full = req
        self._fields = self.Splitter(req)
        if not self.IsValid():
            raise Exception("Could not parse access log record")

    @const_property
    def fields(self):
        return self._fields

    @const_property
    def ip(self):
        return self._fields[self.Indices.IP]

    @const_property
    def referer(self):
        return self._fields[self.Indices.REFERER]

    @const_property
    def useragent(self):
        return self._fields[self.Indices.USERAGENT]

    @const_property
    def http_status(self):
        return self._fields[self.Indices.HTTP_STATUS]

    @const_property
    def fuid(self):
        cook = self.cookies.get('fuid01')
        return cook.value if cook else None

    @const_property
    def is_ys(self):
        return has_ys_prefix(self.path)

    @const_property
    def url(self):
        s = self._fields[self.Indices.REQUEST].strip(' ')
        return s[s.find(' '):s.rfind(' ')].strip(' ')

    @const_property
    def method(self):
        s = self._fields[self.Indices.REQUEST].strip(' ')
        return s[:s.find(' ')]

    # 'host,port'
    @const_property
    def host(self):
        return self._fields[self.Indices.HOST_PORT]

    @const_property
    def protocol(self):
        s = self._fields[self.Indices.REQUEST].strip(' ')
        return s[s.rfind(' ')+1:]

    @const_property
    def time(self):
        return float(self._fields[self.Indices.TIMESTAMP])

    @const_property
    def timestring(self):
        return float(self._fields[self.Indices.TIME_STRING])

    @const_property
    def reqid(self):
        return self._fields[self.Indices.REQID]

    @const_property
    def agent(self):
        return self._fields[self.Indices.USERAGENT]

    @const_property
    def cookies_raw(self):
        return self._fields[self.Indices.COOKIES]

    @const_property
    def yandexuid(self):
        cook = self.cookies.get('yandexuid')
        return cook.value if cook else None


    def __str__(self):
        return self.full


class Request(RequestString):
    class Indices:
        IP = 0
        TIME_STRING = 3
        REQUEST = 4
        HTTP_STATUS = 5
        REPLY_SIZE = 6
        REFERER = 7
        USERAGENT = 8
        HOST_PORT = 9
        COOKIES = 11
        TIMESTAMP = 12
        REQID = 16

    @staticmethod
    def Splitter(req):
        return splitter.SplitAccessLog(req)

    def __init__(self, rec):
        BaseRequest.__init__(self, rec)

    def IsValid(self):
        if len(self._fields) < 17 or self.float_convert(self._fields[self.Indices.TIMESTAMP]) is None:
            return False

        return True


class MarketRequest(RequestString):
    class Indices:
        TIME_STRING = 0
        HOST = 1
        HOST_PORT = HOST
        IP = 2
        REQUEST = 3
        HTTP_STATUS = 4
        REFERER = 5
        USERAGENT = 6
        COOKIES = 7

    @staticmethod
    def Splitter(rec):
        return splitter.SplitMarketLog(rec)

    def IsValid(self):
        return len(self._fields) >= 10

    @const_property
    def time(self):
        return ConvertApacheTimestamp(self._fields[self.Indices.TIME_STRING])

    # 'host,port'
    @const_property
    def host(self):
        return '%s,0' % self._fields[self.Indices.HOST]


    def __str__(self):
        return self.full


class RequestYTDict(BaseRequest):
    "A parsed request as a dict gotten from YT access-log table"

    def __init__(self, dictRequest):
        self.row = dictRequest

    @const_property
    def fields(self):
        raise Exception, "Not applicable"

    @const_property
    def ip(self):
        return self.row.get("ip")

    @const_property
    def referer(self):
        return self.row["referer"]

    @const_property
    def useragent(self):
        return self.row["user_agent"]

    @const_property
    def http_status(self):
        return self.row["status"]

    @const_property
    def fuid(self):
       return self.row.get("fuid")

    @const_property
    def is_ys(self):
        return has_ys_prefix(self.path)

    @const_property
    def url(self):
        return self.row["request"]

    @const_property
    def method(self):
        return self.row["method"]

    # 'host,port'
    @const_property
    def host(self):
        return self.row['vhost']

    @const_property
    def protocol(self):
        return self.row["protocol"]

    @const_property
    def headers(self):
        return self.row.get('headers', '')

    @const_property
    def time(self):
        return float(self.row["request_time"])

    @const_property
    def timestring(self):
        return self.row['timestamp']

    @const_property
    def reqid(self):
        return self.row.get("req_id")

    @const_property
    def agent(self):
        return self.row["user_agent"]

    @const_property
    def cookies_raw(self):
        return self.row.get('cookies', '')

    @const_property
    def yandexuid(self):
        return self.row["yandexuid"]

    def _split_url(self):
        if not getattr(self, 'splittedUrl', None):
            fields = 'netloc', 'path', 'query', 'fragment'
            self.splittedUrl = urlsplit(self.url)
            [setattr(self, field, getattr(self.splittedUrl, field)) for field in fields]
        return self.splittedUrl

    @const_property
    def netloc(self):
        return self._split_url().netloc

    @const_property
    def path(self):
        return self._split_url().path

    @const_property
    def query(self):
        return self._split_url().query

    @const_property
    def fragment(self):
        return self._split_url().fragment

    @const_property
    def cgi(self):
        cgi = {}
        for (k, v) in parse_qsl(self.query, True):
            cgi.setdefault(k, v)
        return cgi

    @const_property
    def cgiex(self):
        return parse_qs(self.query, keep_blank_values=True)

    @const_property
    def numdoc(self, default=10):
        try:
            nd = int(self.cgi['numdoc'])
            if nd < 0 or nd > 50:
                return 10
            elif 0 < nd:
                return nd
        except (KeyError, ValueError):
            pass

        try:
            prefs = self.cookies['YX_SEARCHPREFS'].value
            nd = int(_numdoc_cookie.search(prefs).group(1))
            if 0 < nd <= 50:
                return nd
        except (KeyError, ValueError, AttributeError):
            pass

        return default

    def __str__(self):
       fields = ('timestring', 'method', 'url', 'http_status', 'size', 'referer', 'useragent', 'host', 'cookies_raw', 'time', 'reqid')
       return ' '.join(str(getattr(self, f)) for f in fields)


class MarketRequestYTDict(RequestYTDict):
    def __init__(self, rec):
        RequestYTDict.__init__(self, rec)

    @const_property
    def ip(self):
        return self.row.get("x_remote_ip")

    @const_property
    def fuid(self):
       return None

    @const_property
    def time(self):
        ts = self.row["iso_eventtime"]
        return time.mktime(time.strptime(ts, '%Y-%m-%d %H:%M:%S'))

    @const_property
    def timestring(self):
        return self.row['timestamp']

    @const_property
    def reqid(self):
        return self.row.get("reqid")

def _skip_none(req):
    return False


def _skip_requests(req):
    return req.ip in yandex_net


def _skip_requests_ys(req):
    return req.ip in yandex_net or not req.is_ys


class MakeMap(object):
    def __init__(self, map_func, filter=0):
        self.must_skip = [
            _skip_none,
            _skip_requests,
            _skip_requests_ys,
        ][filter]
        self.map_func = map_func

    def __call__(self, rec):
        try:
            req = RequestYTDict(rec)
            skip = self.must_skip(req)
        except:
            skip = True
        if skip:
            return ()
        return self.map_func(req)


def __GetMonthNames():
    import locale
    curLocale = locale.getlocale()

    locale.setlocale(locale.LC_ALL, ('en_US', 'UTF-8'))

    names = (locale.nl_langinfo(x) for x in (
        locale.ABMON_1,
        locale.ABMON_2,
        locale.ABMON_3,
        locale.ABMON_4,
        locale.ABMON_5,
        locale.ABMON_6,
        locale.ABMON_7,
        locale.ABMON_8,
        locale.ABMON_9,
        locale.ABMON_10,
        locale.ABMON_11,
        locale.ABMON_12
        )
    )

    result = dict(zip(names, xrange(1,13)))

    locale.setlocale(locale.LC_ALL, curLocale)

    return result

__MonthNames = __GetMonthNames()

# ex: 28/Oct/2013:00:21:18 +0400
__reDate = re.compile(r'^(\d\d)/([^/]+)/(\d\d\d\d):(\d\d):(\d\d):(\d\d) ([\+\-]\d\d)(\d\d)$')

def ConvertApacheTimestamp(dateTimeStr):
    p = __reDate.match(dateTimeStr)

    if not p:
        return None

    tm_str = time.struct_time((
        int(p.group(3)),
        __MonthNames[p.group(2)],
        int(p.group(1)),
        int(p.group(4)),
        int(p.group(5)),
        int(p.group(6)),
        0,
        0,
        1
        ))

    timeSec = time.mktime(tm_str)

    ofs = int(p.group(7)) * 60 * 60 + int(p.group(8)) * 60

    timeSec += (-time.timezone - ofs)

    return timeSec
