# -*- coding: utf-8 -*-

import random
import urllib
import re
import types
import json
import datetime


def getRandomString():
    return "rnd%d%d%d" % (random.randrange(1000000), random.randrange(1000000),random.randrange(1000000));

def IntToIpStr(int_str):
    val = int(int_str)
    return str(val >> 24) + '.' +  str((val & 0xFF0000) >> 16) + '.' + str((val & 0xFF00) >> 8) + '.' + str(val & 0xFF)

def empty_if_none(s):
    return s if s else "";


def ToInteger(val, defVal):
    try:
        return int(val)
    except:
        return defVal


def IpStrToNum(ip):
    parts = ip.split(".");
    res = 0;
    try:
        for pp in parts:
            res = res * 256 + int(pp);
    except:
        return 0

    return res;


def FormToJson(form):
    def Default(obj):
        if type(obj) is datetime.date:
            return obj.strftime('%Y%m%d')

    return json.dumps(form.data, default=Default)


# unquote string and convert it into unicode
def DoTryUnquote(s, unquoteFunc):
    try:
        unquoted = unquoteFunc(s)
        if type(unquoted) == types.UnicodeType:
            return unquoted

        try:
            return unquoted.decode('utf-8')
        except:
            pass

        try:
            return unquoted.decode('cp1251')
        except:
            pass

        return unquoted.decode('latin-1')
    except:
        return s.decode('utf-8') if type(s) == types.StringType else s


def TryUnquote(s):
    return DoTryUnquote(s, urllib.unquote)


def TryUnquotePlus(s):
    return DoTryUnquote(s, urllib.unquote_plus)


def FormatTraceback(txt):
    return '''
    <p>
    <pre>%s</pre>
    </p>
    ''' % txt


reSpravka = re.compile(ur"spravka=([^;]+)")
reYp = re.compile(ur"yp=([^;]+)")

def GetSpravkaPos(line):
    def ExtractFromYp():
        match = reYp.search(line)
        if not match:
            return None

        for item in urllib.unquote(match.group(1)).split('#'):
            fields = item.split('.')
            if len(fields) != 3:
                continue

            if fields[1] == 'spravka':
                quoted = urllib.quote(fields[2])
                pos = line.find(quoted)
                return (pos, pos + len(quoted))

        return None

    res = ExtractFromYp()
    if res:
        return res

    match = reSpravka.search(line)
    if match:
        return (match.start(), match.end())

    return None


def ExtractSpravkaStr(line):
    pos = GetSpravkaPos(line)
    if pos:
        return line[pos[0]:pos[1]].split('=', 1)[-1]

    return None
