from antirobot.scripts.access_log import RequestYTDict

import misc
import domain_tools
import str_tools


def HighlightAccesslogLine(req, shortLog):
    host = req.host.split(',')[0]
    Escape = lambda text: str_tools.Escape(misc.TryUnquotePlus(text.encode('utf-8') if text else ''))
    handlers = {
        'time': lambda text: '[%s]' % text,
        'url': lambda text:\
            u'<a href="http://%s%s">%s</a>' % (host, Escape(text), Escape(text)),
        'useragent': lambda text: u'<span class=ua>%s</span>' % Escape(text),
        'http_status': lambda text: u'<span class=status>%s</span>' % Escape(str(text)),
        'referer': lambda text: u'<span class=referer>"%s"</span>' % Escape(text),
        'cookies_raw': lambda text: u'<span class=cookies>"%s"</span>' % Escape(text),
        'host': lambda text: u'<span class=host>%s</span>' % Escape(text),
    }

    def JoinProps(propList):
        res = []
        for prop in propList:
            handler = handlers.get(prop, Escape)
            res.append(handler(getattr(req, prop)))

        return u' '.join(res)

    shortPropList = ('timestring', 'url', 'useragent')
    fullPropList =  ('timestring', 'url', 'http_status', 'size', 'referer', 'useragent', 'host', 'cookies_raw', 'time', 'reqid', 'headers')

    return '<nobr>%s</nobr>' % JoinProps(shortPropList if shortLog else fullPropList)


def GetSpravkaHtml(req, reqDate, host):
    sprStr = req.GetSpravkaStr()
    if not sprStr:
        return ''

    try:
        # TODO: check reqDate, spravka may be expired
        spr = spravka2.Spravka(sprStr, domain_tools.GetCookiesDomainFromHost(host))

        if spr.valid:
            if spr.expired:
                return '<span class="expired">SPRAVKA_EXPIRED</span>'
            else:
                return 'SPRAVKA_VALID  '
        else:
            return '<span class="invalid">SPRAVKA_INVALID</span>'
    except:
        return '<span class="invalid">SPRAVKA_INVALID</span>'


def FormatRows(keysFile, itemList, date, shortLog=False, reqParser=None):
    if not reqParser:
        raise Exception('Request Parser is not specified')

    result_rows = []
    for item in itemList:
        try:
            req = reqParser(item)
        except:
            continue

        ip = req.ip
        reqTime = req.time
        host = req.host.split(',')[0]
        spravkaHtml = GetSpravkaHtml(req, date, host)
        hasYandexLogin = u'LOGIN' if req.cookies.get(u'yandex_login') else '';

        hasYandexUid = u'YUID' if req.cookies.get(u'yandexuid') else ''

        if not shortLog:
            fields = (hasYandexUid, hasYandexLogin, spravkaHtml, ip, HighlightAccesslogLine(req, shortLog))
        else:
            fields = (ip, HighlightAccesslogLine(req, shortLog),)

        result_rows.append({
            'cols': fields,
            'cls': None,
            })

    return result_rows


