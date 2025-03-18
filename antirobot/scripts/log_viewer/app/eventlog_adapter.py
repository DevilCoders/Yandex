import urllib

from antirobot.scripts.antirobot_eventlog import event
from antirobot.scripts.utils import flash_uid


NS = ('UNK', 'IP', 'SPRV', 'XML', 'SID', 'AGGR', 'FUID', 'LCOOK', 'IP6', 'AGGR6', 'ICOOKIE')
NS_SPRV = NS[2]
NS_FUID_INDEX = 6

IGNORED_EVENT_TYPES = (
    'TBadRequest',
    'TGeneralMessage',
    'TCbbRuleParseResult',
    'TCbbRulesUpdated',
    )

def UidNsToStr(ns):
    ins = int(ns)
    if ins >= len(NS):
        return '?'
    else:
        return NS[ins]


class BaseEventAdapter(object):
    def __init__(self, protobufEvent):
        self.pbEvent = protobufEvent

    def EventType(self):
        return self.pbEvent.EventType

    def Timestamp(self):
        return int(self.pbEvent.Timestamp)

    def Addr(self):
        return '-'

    def YandexUid(self):
        return ''

    def Reqid(self):
        return ''

    def UniqueKey(self):
        return ''

    def TextData(self):
        return ''

    def Token(self):
        return ''

    def FormatToken(self):
        return ''

    def UidStr(self):
        return ''


class EventWithHeader(BaseEventAdapter):
    def Addr(self):
        return self.pbEvent.Event.Header.Addr

    def YandexUid(self):
        return str(self.pbEvent.Event.Header.YandexUid)

    def Reqid(self):
        return self.pbEvent.Event.Header.Reqid

    def UniqueKey(self):
        return self.pbEvent.Event.Header.UniqueKey

    def UidStr(self):
        ns = '?'
        iNs = int(self.pbEvent.Event.Header.UidNs)
        if iNs < len(NS):
            ns = NS[iNs]

        if iNs == NS_FUID_INDEX:
            fuid = flash_uid.Fuid.FromId(int(self.pbEvent.Event.Header.UidId))
            return "%s-%s-%s" % (ns, fuid.Random, fuid.Timestamp)

        return '%s-%s' % (ns, str(self.pbEvent.Event.Header.UidId))


class EventWithToken(EventWithHeader):
    def Token(self):
        return self.pbEvent.Event.Token

    def FormatToken(self):
        token = self.pbEvent.Event.Token
        token_type = {
            "": "-",
            "0": "Legacy",
            "1": "Keys",
            "2": "Checkbox",
            "3": "Image",
        }[token.split('/')[0]]
        return token_type + ": " + token


class EventRequestData(EventWithHeader):
    def TextData(self):
        return urllib.unquote_plus(self.pbEvent.Event.Data)


def MakeEventAdapter(event):
    if event.EventType in IGNORED_EVENT_TYPES:
        return None

    if event.EventType in ('TCaptchaRedirect', 'TCaptchaShow', 'TCaptchaImageShow', 'TCaptchaTokenExpired', 'TCaptchaImageError', 'TCaptchaCheck'):
        return EventWithToken(event)

    if event.EventType == 'TRequestData':
        return EventRequestData(event)

    return EventWithHeader(event)

