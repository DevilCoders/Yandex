import sys
import re
import traceback
import urllib
import math
import time
import datetime
import StringIO

from antirobot.scripts.utils import (
    unquote_safe,
    spravka2,
    flash_uid,
)

import eventlog_adapter
import misc
import domain_tools
import str_tools


REQUEST_CATEGORIES = (
        'Undefined category',
        'Non search request',
        'Search request',
        'Search request with spravka',
        'Any request from whitelist',
        )


def Safe(text):
    return str_tools.Escape(misc.TryUnquotePlus(text))


def FormatDate(microseconds):
    microseconds = int(microseconds)
    ctime = math.floor(microseconds / 1000000)

    return time.strftime('%d.%m.%y %H:%M:%S', time.localtime(ctime))


reFuid = re.compile(ur"fuid01=([^;]+)")
reRetPath = re.compile(ur'retpath=([^&]+)')
reOriginQuery = re.compile(ur'query=([^&]+)')


def Format_TRequestData(ctx, event, keysFile=None):
    def GetParams(lines):
        res = {'host': '', 'user_agent': '', 'fuid': '', 'spravka': '', 'origin_query': '', 'from_partner': False}

        for i in lines:
            line = i.lower()
            if line.startswith('host:') and not res.get('host'):
                res['host'] = i.split(': ')[1]
                continue

            if line.startswith('x-host-y:'):
                 res['host'] = i.split(': ')[1]
                 continue

            if line.startswith('user-agent:'):
                res['user_agent'] = i.split(': ', 1)[1]
                continue

            if line.startswith('cookie:'):
                match = reFuid.search(i)
                if match:
                    res['fuid'] = match.group(1)

                spr = misc.ExtractSpravkaStr(i)
                if spr:
                    res['spravka'] = spr #.encode('utf-8')

                continue

            if line.startswith(u'get /checkcaptcha'):
                line = urllib.unquote(line)
                match = reRetPath.search(line)
                if match:
                    res['origin_request'] = (False, misc.TryUnquotePlus(match.group(1)))

                continue

            if line.startswith(u'referer: '):
                line = urllib.unquote(i)
                match = reOriginQuery.search(line)
                if match:
                    res['origin_request'] = (True, misc.TryUnquotePlus(match.group(1)))
                    res['from_partner'] = True

                continue

        return res

    def EnableSearchUrl(lines, params):
        getStr = lines[0]
        fields = getStr.split(' ')
        if len(fields) > 1:
            lines[0] = '<a target="_blank" href="http://%s%s">%s</a>' % (params['host'], fields[1], getStr)

    def ExtractImportantCookies(lines):
        r = {}
        for line in lines:
            if line.startswith('Cookie:'):
                match = reFuid.search(line)
                if match:
                    r['fuid'] = match.group(1)

                spr = ExtractSpravkaStr(line)
                if spr:
                    r['spravka'] = spr
                break

        return r

    def ExtractOriginRequest(lines):
        for line in lines:
            if line.startswith(u'GET /checkcaptcha'):
                line = urllib.unquote(line)
                match = reRetPath.search(line)
                if match:
                    return (False, misc.TryUnquotePlus(match.group(1)))
            if line.startswith(u'Referer: '):
                line = urllib.unquote(line)
                match = reOriginQuery.search(line)
                if match:
                    return (True, misc.TryUnquotePlus(match.group(1)))
                break
        return (None, None)

    def SetupCookieDetails(res, params, requestDate):
        if params.get('spravka'):
            # TODO: check requestDate, spravka may be expired
            res['spravka'] = spravka2.Spravka(params['spravka'], domain_tools.GetCookiesDomainFromHost(params['host']))

        if params.get('fuid'):
            fuid = flash_uid.Fuid.FromCookieValue(params['fuid'])
            res['fuid'] = {'issue_date': datetime.datetime.fromtimestamp(fuid.Timestamp)}

    def HighlightImportantCookies(lines):
        for (i, line) in enumerate(lines):
            if line.startswith('Cookie:'):
                match = reFuid.search(line)
                if match:
                    line = line[:match.start()] + '<b>' + line[match.start():match.end()] + '</b>' + line[match.end():]

                pos = misc.GetSpravkaPos(line)
                if pos:
                    line = line[:pos[0]] + '<b>' + line[pos[0]:pos[1]] + '</b>' + line[pos[1]:]

                lines[i] = line
                break

    lines = unquote_safe.SafeUnquote(event.TextData())
    lines = lines.split(u"\r\n")

    params = GetParams(lines)

    EnableSearchUrl(lines, params)

    res = {}

    SetupCookieDetails(res, params, datetime.datetime.fromtimestamp(event.Timestamp() / 1000000))
    HighlightImportantCookies(lines)

    if ctx.prevEventType == 'TCaptchaCheck':
        if params['origin_query']:
            res['origReq'] = {'fromPartner': params['from_query'], 'req': params['origin_query']}

    out = StringIO.StringIO()

    if res.get('spravka'):
        spr = res.get('spravka')
        if spr.valid:
            print >>out, '<b>spravka</b> is valid, ip: %s, issue date: %s' % (spr.ip, spr.issueDate)
            if spr.expired:
                print >>out, ' (expired)'
        else:
            print >>out, '<b>spravka is invalid: %s</b>' % spr.reason

        print >>out, '<br>'

    if res.get('fuid'):
        print >>out, '<b>fuid</b> present, issue date: %s<br>' % res['fuid']['issue_date']


    if res.get('origReq'):
        origReq = res['origReq']
        if origReq.fromPartner:
            print >>out, '<b>Origin request to partner</b>: %s' % origReq.req
        else:
            print >>out, '<b>Origin request: </b><a target="_blank" href="%s">%s</a>' % (origReq.req, Safe(origReq.req))

        print >>out, '<br>'

    for line in lines:
        print >>out, line, '<br>'

    return out.getvalue()


def Format_TAntirobotFactors(ctx, event, **kw):
    factors = str(event.pbEvent.Event.Factors).replace(' ', '')
    l = []
    if int(event.pbEvent.Event.IsRobot):
        l.append('Robot;')
    if int(event.pbEvent.Event.InitiallyWasXmlsearch):
        l.append('XmlSearch;')

    res = ', '.join(l)

    return res


def DateTimeFromTimestamp(timeStamp):
    return datetime.datetime.fromtimestamp(timeStamp / 1000000)


reDomainFromSetCookie = re.compile('domain=\.?([^;\ ]+)')


def format_client_type(event):
    return 'ClientType: ' + {
        0: "general",
        1: "xml_partner",
        2: "ajax",
        3: "captcha_api",
    }.get(event.pbEvent.Event.ClientType, "UNKNOWN: " + str(event.pbEvent.Event.ClientType))


def format_ban_reasons(event):
    ban_reaspns = [r for r in ('Matrixnet', 'Yql', 'Cbb') if getattr(event.pbEvent.Event.BanReasons, r)]
    return 'BanReasons: (' + ', '.join(ban_reaspns) + ')'


def format_host_type(event):
    return 'HostType: ' + str(event.pbEvent.Event.HostType)  # TODO: расшифровывать из enum-а


def format_spravka(event):
    def GetDomain():
        p = reDomainFromSetCookie.search(event.pbEvent.Event.NewSpravka)
        if p:
            return p.group(1)

        return None

    if not event.pbEvent.Event.NewSpravka or event.pbEvent.Event.NewSpravka == "-":
        return "No spravka"

    spravkaStr = misc.ExtractSpravkaStr(event.pbEvent.Event.NewSpravka)
    spr = spravka2.Spravka(spravkaStr, GetDomain())

    degradations = [k for k in ("Web", "Market", "Uslugi", "Autoru") if spr.degradation[k]]

    return "<br>".join([
        str(event.pbEvent.Event.NewSpravka),
        "Spravka " + ('<span class="green">valid</span>' if spr.valid else '<span class="red">invalid</span>'),
        "Spravka uid: " + str(spr.uid),
        'Spravka degradations: <span class="red">' + ", ".join(degradations) + '</span>'
    ])


def Format_TCaptchaCheck(ctx, event, keysFile=None):
    res = []

    if event.pbEvent.Event.Success:
        s = 'Result: <b class="green">success</b>'
    else:
        s = 'Result: <b class="red">fail</b>'

    if event.pbEvent.Event.PreprodSuccess:
        s += ', <span class="green">preprod_success</span>'
    else:
        s += ', <span class="red">preprod_fail</span>'

    if event.pbEvent.Event.Success:
        s += ', ' + event.pbEvent.Event.NewSpravka

    res.append(s)

    for attr in ('CaptchaConnectError', 'FuryConnectError', 'FuryPreprodConnectError'):
        if getattr(event.pbEvent.Event, attr):
            res.append('<span class="red">' + attr + ': ' + str(getattr(event.pbEvent.Event, attr)) + '</span>')

    if event.pbEvent.Event.WarningMessages:
        res.append('<span class="red">' + str_tools.Escape(event.pbEvent.Event.WarningMessages) + '</span>')

    res.append(format_spravka(event))

    return '<br>'.join(res)


def Format_TCaptchaShow(ctx, event, **kw):
    l = []
    if event.pbEvent.Event.Again:
        l.append('Again')

    if event.pbEvent.Event.TestCookieSet:
        ctx.isTestSpravka = True
        l.append('Test cookie has been set')
        if int(event.pbEvent.Event.TestCookieSuccess):
            l.append('Test cookie result: success')
        else:
            l.append('Test cookie result: fail')
    else:
        ctx.isTestSpravka = False

    l.append(format_client_type(event))
    l.append(format_host_type(event))
    l.append('CaptchaType: ' + event.pbEvent.Event.CaptchaType)
    l.append(format_ban_reasons(event))
    l.append('Key: ' + event.pbEvent.Event.Key)

    return "<br>".join(l)


def Format_TCaptchaRedirect(ctx, event, **kw):
    ctx.isTestSpravka = False
    l = []
    if event.pbEvent.Event.Again:
        l.append('Again')

    l.append(format_client_type(event))

    if event.pbEvent.Event.CaptchaType:
        l.append('CaptchaType: ' + event.pbEvent.Event.CaptchaType)

    if event.pbEvent.Event.Key:
        l.append('Key: ' + event.pbEvent.Event.Key)

    l.append(format_host_type(event))
    l.append(format_ban_reasons(event))

    return "<br>".join(l)


def Format_TCaptchaTokenExpired(ctx, event, **kw):
    return ''


def Format_TCaptchaImageShow(ctx, event, **kw):
    return ''


def Format_Default(ctx, event, **kw):
    return ''


def Format_TCaptchaImageError(ctx, event, **kw):
    return ['Error: ' + str(event.pbEvent.Event.ErrorCode),
            'Request: ' + event.pbEvent.Event.Request]

def Format_TRequestGeneralMessage(ctx, event, **kw):
    return ['Level: ' + str(event.pbEvent.Event.Level),
            'Message: ' + str_tools.Escape(misc.TryUnquotePlus(event.pbEvent.Event.Message))]


def Format_TBlockEvent(ctx, event, **kw):
    ev = event.pbEvent.Event
    return ['<b>%s</b>' % ('Blocked' if ev.BlockType == 0 else 'Unblocked'),
            '%s' % REQUEST_CATEGORIES[ev.Category],
            ev.Description
            ]


NotesFormatters = {
    'TRequestData': Format_TRequestData,
    'TAntirobotFactors': Format_TAntirobotFactors,
    'TCaptchaCheck': Format_TCaptchaCheck,
    'TCaptchaShow': Format_TCaptchaShow,
    'TCaptchaRedirect': Format_TCaptchaRedirect,
    'TCaptchaTokenExpired': Format_TCaptchaTokenExpired,
    'TCaptchaImageShow': Format_TCaptchaImageShow,
    'TCaptchaImageError': Format_TCaptchaImageError,
    'TRequestGeneralMessage': Format_TRequestGeneralMessage,
    'TBlockEvent': Format_TBlockEvent,
}


def FormatResult(pbEvents, keysFile):
    i = 0
    prevToken = ''
    type = None
    trClassesIndex = 0
    trClasses = ('sel1', 'sel2')

    class Context:
        def __init__(self):
            self.isTestSpravka = False
            self.prevEventType = None

    ctx = Context()

    result_rows = []
    for pbEvent in pbEvents:
        row_class = ''
        event = None
        try:
            event = eventlog_adapter.MakeEventAdapter(pbEvent)
            if not event:
                continue

            curToken = event.Token()
            if prevToken != curToken:
                trClassesIndex += 1
                prevToken = curToken

            row_class = trClasses[trClassesIndex % 2] if curToken else ''
        except:
            print >>sys.stderr, traceback.format_exc()

        if not event:
            continue

        event_type = event.EventType()
        result_rows.append({
            'cols': [
                '<nobr>%s</nobr>' % FormatDate(event.Timestamp()).decode('utf-8'), # timestamp
                event.Addr().decode('utf-8'),  # addr
                event.UidStr(), # uid
                str_tools.Escape(misc.TryUnquotePlus(event.YandexUid())), # yandexuid
                event_type, # eventtype
                event.Reqid().decode('utf-8'),  # requid
                event.UniqueKey().decode('utf-8'),  # uniquekey
                event.FormatToken(),  # token
                NotesFormatters.get(event_type, Format_Default)(ctx, event, keysFile=keysFile), # Notes
            ],
            'cls': row_class
        })
        i += 1
        ctx.prevEventType = type

    return result_rows
