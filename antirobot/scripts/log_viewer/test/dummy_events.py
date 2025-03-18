import time
from antirobot.scripts.antirobot_eventlog.event import GetMsgClassByName, AntirobotEvent


DEFAULT_IP = '1.2.3.4'

def DefHeader():
    return {
        'Reqid': 'reqid',
        'IpDeprecated': 1234,
        'Addr': DEFAULT_IP,
        'UidNs': 1,
        'UidId': 1234,
        'Uid': 'uid',
        'YandexUid': '123456790',
        'PartnerIpDeprecated': 1234,
        'PartnerAddr': '4.3.2.1'
    }


def DefBanReasons():
    return {
        'Matrixnet': False,
        'Yql': False,
        'Cbb': False,
    }


def AntirobotFactors(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TAntirobotFactors'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'IsRobot': True,
            'Factors': [1, 2, 3, 4],
            'FactorsVersion': 30,
            'InitiallyWasXmlsearch': False,
        })


def CaptchaRedirect(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TCaptchaRedirect'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'Again': 0,
            'IsXmlPartner': 0,
            'IsRandom': 1,
            'Token': 't',
            'ClientType': 0,
            'CaptchaType': 't',
            'BanReasons': DefBanReasons(),
            })


def CaptchaShow(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TCaptchaShow'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'Again': 0,
            'IsXmlPartner': 0,
            'Key': 'k',
            'Token': 't',
            'TestCookieSet': 0,
            'TestCookieSuccess': 0,
            'ClientType': 0,
            'CaptchaType': 't',
            'BanReasons': DefBanReasons(),
            })


def CaptchaCheck(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TCaptchaCheck'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'Key': 'k',
            'Token': 't',
            'Success': 1,
            'NewSpravka': 's',
            'EnteredHiddenImage': 0,
            'CaptchaConnectError': 0,
            'PreprodSuccess': 0,
            'FuryConnectError': 0,
            'FuryPreprodConnectError': 0,
            })

def BadRequest(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TBadRequest'),
        timestamp=timestamp,
        params={
            'Message': 'error message'
        })


def RequestData(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TRequestData'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'Data': 'request data',
        })


def CaptchaTokenExpired(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TCaptchaTokenExpired'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'Token': 't',
        })


def CaptchaImageShow(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TCaptchaImageShow'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'Token': 't',
        })


def CaptchaImageError(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TCaptchaImageError'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'Token': 't',
            'ErrorCode': 1,
            'Request': 'request',
        })


def RequestGeneralMessage(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TRequestGeneralMessage'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'Level': 0,
            'Message': 'some message',
        })


def GeneralMessage(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TGeneralMessage'),
        timestamp=timestamp,
        params={
            'Message': 'general message',
            'Level': 0,
        })


def WizardError(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TWizardError'),
        timestamp=timestamp,
        params={
            'Header': header(),
        })


def BlockEvent(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TBlockEvent'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'BlockType': 0,
            'Category': 1,
            'Description': 'descr',
        })


def CaptchaVoice(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TCaptchaVoice'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'Token': 't',
        })


def CaptchaVoiceIntro(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TCaptchaVoiceIntro'),
        timestamp=timestamp,
        params={
            'Header': header(),
            'Token': 't',
        })


def CbbRuleParseResult(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TCbbRuleParseResult'),
        timestamp=timestamp,
        params={
            'Status': 'ok',
        })


def CbbRulesUpdated(header=DefHeader, timestamp=time.time()):
    return AntirobotEvent.Create(
        GetMsgClassByName('TCbbRulesUpdated'),
        timestamp=timestamp,
        params={
            'ParseResults': [CbbRuleParseResult().Event for _ in xrange(3)]
        })
