import re
import datetime
import os

reLogin = re.compile(r'yandex_login=([^;\ \"]+)')
reFlash = re.compile(r'fuid01=([^;\ \"]+)')
reSpravka = re.compile(r'spravka=([^;\ \"]+)')
reYandexUid = re.compile(r'yandexuid=([^;\"\ ]+)')

def IsSearchReq(req, checkMethod=False, method='GET', checkImagePager=True, ignoreAjax=False):
    if checkMethod and method  != 'GET':
        return False

    if not (req.startswith("/yandsearch") or
            req.startswith("/xmlsearch") or
            req.startswith("/msearch") or
            req.startswith("/telsearch") or
            req.startswith("/touchsearch") or
            req.startswith("/largesearch") or
            req.startswith("/schoolsearch") or
            req.startswith("/familysearch") or
            req.startswith("/sitesearch") or
            req.startswith("/search")):
        return False

    if checkImagePager and req.find('rpt=imagepager') >= 0:
        return False

    if ignoreAjax and (req.find('callback=jQuery') >= 0 or req.find('format=json') >= 0):
        return False

    return True

def GetYandexLogin(req):
    m = reLogin.search(req)
    if m:
        return m.group(1)
    else:
        return ''

def GetFlashUid(req, shorten = False):
    m = reFlash.search(req)
    if m:
        res = m.group(1)
        if shorten:
            res = res.split('.',1)[0]
        return res
    else:
        return ''

def GetSpravka(req):
    m = reSpravka.search(req)
    if m:
        return m.group(1)
    else:
        return ''

def GetYandexUid(req):
    m = reYandexUid.search(req)
    if m:
        return m.group(1)[:20]
    else:
        return ''

def MakeTablesList(prefix, dayStart, dayEnd):
    res = []
    i = dayStart
    while i <= dayEnd:
        res.append(os.path.join(prefix, i.strftime('%Y-%m-%d')))
        i += datetime.timedelta(days=1)

    return res
