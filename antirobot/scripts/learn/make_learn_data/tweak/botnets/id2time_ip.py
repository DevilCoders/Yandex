from urllib import unquote
import hashlib
import os

from serialize_log import pack_time_ip

from antirobot.scripts.access_log.request import MakeMap
from antirobot.scripts.learn.make_learn_data.tweak import utl


def MD5(text):
    hsh = hashlib.md5()
    hsh.update(text)
    return hsh.hexdigest()


def ShortenKey(cookieName, value):
    if cookieName == 'fuid01':
        value = value.split('.', 1)[0]
        if len(value) != 16:
            value = MD5(value)
    elif cookieName != 'yandex_login':
        value = MD5(value)

    return value


class IpToIdMapper(object):
    def __init__(self, id_cookies):
        self._enumed_cookies = list(enumerate(id_cookies))

    def __call__(self, req):
        for idx, cookie in self._enumed_cookies:
            obj = req.cookies.get(cookie)
            if obj:
                key = ShortenKey(cookie, unquote(obj.value))
                yield {'key': key, 'timestamp': req.time, 'ip': req.ip, '@table_index': idx}
            #yield {'key': req.cookies.keys(), 'timestamp': req.time, 'ip': req.ip}
            #yield {'key': idx, '@table_index': idx}


def id2time_ip(tweakTask, srcPrefix, dstPrefix, cookies, date_begin, date_end):
    '''date_begin and date_end must be of type datetime.date'''

    srcTables = utl.MakeTablesList(srcPrefix, date_begin, date_end)
    tweakTask.RunMap(MakeMap(IpToIdMapper(cookies)), srcTables, [os.path.join(dstPrefix, i) for i in cookies])
