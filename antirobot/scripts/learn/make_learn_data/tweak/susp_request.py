import re

from antirobot.scripts.learn.make_learn_data import data_types

reReqText = re.compile(r'(?:text|query)=([^\s&]*)')

#reSuspText = re.compile(r'(?:numdoc=50|url=|site=|host=|siteurl=|serverurl=|inurl=|inurl:|site:|url%3A|url%3D|site%3A|serverurl:|siteurl:|inurl%3A|site%3D)')
reSuspText = re.compile(r'%22|%28|%7C|(?:url|host|site)[:%]|(?:serverurl|surl|host|site)=|numdoc=[^1]|text=(?:[& ]|$)')
goodCookieRe = re.compile(r"Cookie:\s*[^\s\\]");


def IsSuspicious(rndReqData):
    requestData = rndReqData.Raw.request
    reqFields = requestData.split(' ', 2)
    reqStr = reqFields[1] if len(reqFields) >= 2 else ''

    return (reSuspText.search(reqStr) != None or (not goodCookieRe.search(requestData) and not reqStr.startswith("/xmlsearch")) or
            int(rndReqData.TweakFlags.numDocs) > 15 or rndReqData.TweakFlags.haveSyntax != '0' or
            rndReqData.TweakFlags.haveRestr != '0' or rndReqData.TweakFlags.quotes != '0' or
            rndReqData.TweakFlags.cgiUrlRestr != '0')

