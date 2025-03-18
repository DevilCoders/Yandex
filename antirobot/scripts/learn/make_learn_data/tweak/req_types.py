#from access_log import Request

# Makes names in module namespace
def Enum(valToNameDictName, tupl):
    G = globals()
    dic = {}
    for i, v in enumerate(tupl, 1):
       G[v] = i
       dic[i] = v
    G[valToNameDictName] = dic


Enum('ClientTypeNames', (
    'CLIENT_GENERAL',
    'CLIENT_XML_PARTNER',
    'CLIENT_AJAX',
    ))


Enum('ReqTypeNames', (
    'REQ_OTHER',
    'REQ_MAIN',
    'REQ_YANDSEARCH',
    'REQ_XMLSEARCH',
    'REQ_CYCOUNTER',
    'REQ_OPENSEARCH',
    'REQ_NEWS_CLICK',
    'REQ_IMAGEPAGER',
    'REQ_MSEARCH',
    'REQ_FAMILYSEARCH',
    'REQ_SCHOOLSEARCH',
    'REQ_LARGESEARCH',
    'REQ_REDIR',
    'REQ_SITESEARCH',
    'REQ_IMAGE_LIKE',
    'REQ_FAVICON',
    'REQ_YANDSEARCH_NO_LR',
    'REQ_SERP_REQID_COUNTER',
    'REQ_NUMTYPES'
    ))

def IsWebSearch(reqType):
    return reqType in (
        REQ_YANDSEARCH, 
        REQ_MSEARCH, 
        REQ_FAMILYSEARCH, 
        REQ_SCHOOLSEARCH, 
        REQ_LARGESEARCH, 
        REQ_IMAGE_LIKE
        )

def IsSearch(reqType):
    return IsWebSearch(reqType) or reqType == REQ_XMLSEARCH

Enum('HostTypeNames', (
    'HOST_OTHER',
    'HOST_WEB',
    'HOST_XMLSEARCH_COMMON',
    'HOST_CLICK',
    'HOST_IMAGES',
    'HOST_NEWS',
    'HOST_YACA',
    'HOST_BLOGS',
    'HOST_HILIGHTER',
    'HOST_MARKET',
    'HOST_NUMTYPES',
    ))

Enum('ReportTypeNames', (
    'REP_OTHER',
    'REP_MARKET_OTHER',
    'REP_MARKET_MAIN',
    'REP_MARKET_STATIC',
    'REP_MARKET_SEARCH',
    'REP_MARKET_CATALOG_OFFERS',
    'REP_MARKET_CATALOG_MODELS',
    'REP_MARKET_CATALOG',
    'REP_MARKET_MODEL_PRICES',
    'REP_MARKET_MODEL',
    'REP_MARKET_GURU',
    'REP_MARKET_OFFERS',
    'REP_MARKET_MODEL_OPINIONS',
    'REP_BLOGS_OTHER',
    'REP_BLOGS_MAIN',
    'REP_BLOGS_THEME',
    'REP_BLOGS_SEARCH',
    'REP_NUMTYPES',
    ))

class RequestFeatures(object):
    __slots__ = ['clientType', 'reqType', 'hostType', 'reportType']

    def __init__(self):
        self.clientType = None
        self.reqType = None
        self.hostType = None
        self.reportType = None


def GetCgi(cgiParams, p):
    pList = cgiParams.get(p)
    if pList:
        return pList[0]
    else:
        return None


def HasCgi(cgiParams, param, value):
    pList = cgiParams.get(param, [])
    return value in pList


def CalcReqType(reqPath, cgiParams, hostType):

    _GetCgi = lambda param: GetCgi(cgiParams, param)
    _HasCgi = lambda param, value: HasCgi(cgiParams, param, value)

    IsNewsClick = lambda: hostType == HOST_NEWS and _GetCgi("cl4url") != None and _GetCgi("text") == None

    reqType = REQ_OTHER
    clientType = CLIENT_GENERAL

    if reqPath in ('/yandsearch', "/yandpage", "/search") or reqPath == "/sitesearch" and _GetCgi("web") == "1":
        if hostType == HOST_IMAGES:
            rpt = _GetCgi("rpt")

            if rpt == "imagepager":
                reqType = REQ_IMAGEPAGER
            elif _GetCgi("like") != None:
                reqType = REQ_IMAGE_LIKE

            if rpt == "imageajax" or rpt == "imagedupsajax" or _GetCgi("format") == "json":
                clientType = CLIENT_AJAX;
        elif hostType == HOST_WEB:
            if _GetCgi("ajax") == "1":
                clientType = CLIENT_AJAX

        if reqType == REQ_OTHER:
            if IsNewsClick():
                reqType = REQ_NEWS_CLICK;
            elif hostType != HOST_WEB or _GetCgi('lr') != None:
                # for yandex.ru/yandsearch there must be cgi parameter "lr".
                reqType = REQ_YANDSEARCH
            else:
                reqType = REQ_YANDSEARCH_NO_LR

    elif reqPath == "/" and len(cgiParams) == 0:
        reqType = REQ_MAIN
    elif reqPath.startswith("/cycounter"):
        reqType = REQ_CYCOUNTER
    elif reqPath == "/opensearch.xml":
        reqType = REQ_OPENSEARCH
    elif reqPath == "/xmlsearch":
        reqType = REQ_XMLSEARCH;
    elif reqPath in ("/msearch", "/telsearch", "/touchsearch"):
        reqType = REQ_MSEARCH;

        # todo
        '''
        if reqPath == "/toucVh":
            if (!!YandexUid && !!cgi.Get(TStringBuf("callback")) && YandexUid == cgi.Get(TStringBuf("yu")) ||
                    cgi.Get(TStringBuf("format")) == "json"sv)
            {
                ClientType = CLIENT_AJAX;
            }
        }
        '''

    elif reqPath == "/search.xml":
        if hostType != HOST_BLOGS:
            reqType = REQ_YANDSEARCH;
        elif _GetCgi("text") != None or not _HasCgi("cat", "theme") or _GetCgi("id") == None:
            reqType = REQ_YANDSEARCH;
    elif reqPath == "/familysearch":
        reqType = REQ_FAMILYSEARCH;
    elif reqPath == "/schoolsearch":
        reqType = REQ_SCHOOLSEARCH;
    elif reqPath == "/largesearch":
        reqType = REQ_LARGESEARCH;
    elif reqPath.startswith("/redir") or reqPath.startswith("/click/") or reqPath.startswith("/safeclick/")\
            or reqPath.startswith("/counter/") or reqPath.startswith("/jclck/")  or reqPath == "/c":
        reqType = REQ_REDIR;
    elif reqPath == "/sitesearch":
        reqType = REQ_SITESEARCH;
    elif reqPath == "/favicon.ico":
        reqType = REQ_FAVICON;
    elif reqPath.startswith("/clck/click") and reqPath.find("/dtype=") != -1 and reqPath.find("/u=") != -1 and reqPath.find("/reqid=") != -1:
        reqType = REQ_SERP_REQID_COUNTER;

    return (reqType, clientType)


def CalcHostType(hostStr):
    if hostStr.startswith("yandex."):
        return HOST_WEB
  
    # TODO: separate hostStr for mobile images
    if hostStr.startswith("images.yandex.") or hostStr.startswith("m.images.yandex.")\
        or  hostStr.startswith("gorsel.") or hostStr.startswith("m.gorsel."):
        return HOST_IMAGES
    
    if hostStr.startswith("xmlsearch") or hostStr.startswith("kaxml"):
        return HOST_XMLSEARCH_COMMON
    
    if hostStr.startswith("clck.yandex."):
        return HOST_CLICK

    if hostStr.startswith("news.") or hostStr.startswith("m.news.") or hostStr.startswith("haber.") or hostStr.startswith("m.haber."):
        return HOST_NEWS

    if hostStr.startswith("market.yandex.") or hostStr.startswith("m.market.yandex.") or hostStr.startswith("market.captcha.yandex."):
        return HOST_MARKET

    if hostStr.find('yaca') != -1:
        return HOST_YACA

    if hostStr.startswith("blogs.yandex.") or hostStr.startswith("m.blogs.yandex."):
        return HOST_BLOGS

    if hostStr == "hghltd.yandex.net":
        return HOST_HILIGHTER

    if hostStr.find('.yandex.') != -1:
        return HOST_WEB

    return HOST_OTHER;


def CalcReportType(reqPath, cgiParams, hostType):
    res = REP_OTHER

    if (hostType == HOST_MARKET):
        if reqPath == "/":
            res = REP_MARKET_MAIN
        elif reqPath == "/search.xml":
            res = REP_MARKET_SEARCH
        elif reqPath == "/catalogoffers.xml":
            res = REP_MARKET_CATALOG_OFFERS
        elif reqPath == "/catalogmodels.xml":
            res = REP_MARKET_CATALOG_MODELS
        elif reqPath == "/catalog.xml":
            res = REP_MARKET_CATALOG
        elif reqPath == "/model-prices.xml":
            res = REP_MARKET_MODEL_PRICES
        elif reqPath == "/model.xml":
            res = REP_MARKET_MODEL
        elif reqPath == "/guru.xml":
            res = REP_MARKET_GURU
        elif reqPath == "/offers.xml":
            res = REP_MARKET_OFFERS
        elif reqPath == "/model-opinions.xml":
            res = REP_MARKET_MODEL_OPINIONS
        elif reqPath.startswith("/i/") or reqPath.startswith("/_c/") or reqPath.startswith("/mvc/static/"):
            res = REP_MARKET_STATIC
        else:
            res = REP_MARKET_OTHER
    elif hostType == HOST_BLOGS:
        if reqPath == "/":
            res = REP_BLOGS_MAIN
        elif reqPath == "/search.xml":
            if GetCgi(cgiParams, "text") == None and HasCgi(cgiParams, "cat", "theme") and GetCgi(cgiParams, "id"):
                res = REP_BLOGS_THEME
            else:
                res = REP_BLOGS_SEARCH
        else:
            res = REP_BLOGS_OTHER

    return res


# req is  access_log.Request()
# returns RequestFeatures instance
def GetRequestFeatures(reqObj):
#req = cgi.urlparse.urlparse(reqStr)
    cgiParams = reqObj.cgiex#cgi.urlparse.parse_qs(req.query, keep_blank_values=True)

    reqFeat = RequestFeatures()
    reqFeat.hostType = CalcHostType(reqObj.host)
    reqFeat.reqType, reqFeat.clientType = CalcReqType(reqObj.path, cgiParams, reqFeat.hostType)
    reqFeat.reportType = CalcReportType(reqObj.path, cgiParams, reqFeat.hostType)

    return reqFeat

if __name__ == "__main__":
    import sys
    import os

    THIS_DIR = os.path.dirname( os.path.realpath(__file__))
    sys.path.append('.')
    sys.path.append(THIS_DIR + '/..')
    sys.path.append(THIS_DIR + '/../..')
    from access_log import Request

    for i in sys.stdin:
        r = Request(i)
        reqFeat = GetRequestFeatures(r)
#prefs = r.cookies.get('YX_SEARCHPREFS')

#        prefs = prefs.value if prefs else ''
        isSearch = IsSearch(reqFeat.reqType)
        print '\t'.join(map(str, (int(isSearch), ClientTypeNames[reqFeat.clientType], HostTypeNames[reqFeat.hostType], ReqTypeNames[reqFeat.reqType], ReportTypeNames[reqFeat.reportType], r.url)))
