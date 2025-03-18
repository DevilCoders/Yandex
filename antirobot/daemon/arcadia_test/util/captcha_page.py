import json
import re
import urllib.parse
import urllib.request

from . import Fullreq, GenRandomIP, IsCaptchaRedirect
from .asserts import AssertCaptchaRedirect


FORCED_CAPTCHA_REQUEST = 'Капчу!'
CAPTCHA_IMG_RE = re.compile(r'https?://[^/]+/captchaimg\?[^">]*[">]')


def get_captcha_key(suite):
    """
    Чтобы тестировать обработку запросов /checkcaptcha, нам нужен ключ, который мы будем передавать
    в параметре key. Чтобы его получить, отправляем в Ловилку запрос, на который гарантированно
    будет выдана капча.

    Маленький хак - независимо от того, каким способом был получен ключ: через XML, AJAX или обычную
    капчу, - он может быть использован для любого проверочного запроса (/checkcaptcha, /xcheckcaptcha,
    /checkcaptchajson). Поэтому мы всегда получаем ключ через AJAX-запрос, независимо от того, какой
    /check-запрос тестируем
    """
    def SendAjaxSearch():
        return suite.send_request(Fullreq(r"http://yandex.ru/yandsearch?ajax=1&text=%s" % FORCED_CAPTCHA_REQUEST))

    resp = SendAjaxSearch()
    assert resp.getcode() == 200
    vals = json.loads(resp.read())
    assert "captcha" in vals
    assert "key" in vals["captcha"]
    key = vals["captcha"]["key"]

    return key


class BaseHtmlCaptchaPage(object):
    ImgRe = re.compile(r'<img[^>]*src="(http(?:s?)://[^/]+/captchaimg\?[^"]*)"')
    FormActionRe = re.compile(r'action="([^"]*?)"')

    def get_check_captcha_url(self):
        form_actions = self.FormActionRe.findall(self.page)
        assert len(form_actions) == 1
        return f"http://{self.host}{form_actions[0]}"

    def get_ret_path(self):
        return urllib.parse.parse_qs(urllib.parse.urlparse(self.get_check_captcha_url()).query)['retpath'][0]

    def get_key(self):
        return urllib.parse.parse_qs(urllib.parse.urlparse(self.get_check_captcha_url()).query)['key'][0]

    def get_img_urls(self):
        image_urls = self.ImgRe.findall(self.page)
        assert len(image_urls) > 0
        return image_urls

    def get_content(self):
        return self.page

    def get_content_type(self):
        return self.content_type

    def _set_page(self, suite, host, doc, headers, request_text, pass_checkbox, additional_query=""):
        headers = headers or {}
        request = f"http://{host}{doc}?text={urllib.request.quote(request_text)}{additional_query}"
        resp = suite.send_request(Fullreq(request, headers=headers))
        AssertCaptchaRedirect(resp)

        resp = suite.send_request(Fullreq(resp.info()["Location"], headers=headers))

        assert resp.getcode() == 200
        self.page = resp.read().decode()
        self.content_type = resp.headers.get("Content-Type")
        self.host = host

        if pass_checkbox:
            resp = suite.send_request(Fullreq(self.get_check_captcha_url(), headers=headers))
            assert IsCaptchaRedirect(resp), "Probably you need to set --button-strategy to fail this checkbox captcha"
            resp = suite.send_request(Fullreq(resp.info()["Location"], headers=headers))
            assert resp.getcode() == 200
            self.page = resp.read().decode()
            self.content_type = resp.headers.get("Content-Type")
            self.host = host


class HtmlCheckboxCaptchaPage(BaseHtmlCaptchaPage):
    def __init__(self, suite, host, doc="/yandsearch", headers=None, additional_query=""):
        headers = headers or {}
        if host.endswith('eda.yandex'):
            headers['X-Antirobot-MayBanFor-Y'] = '1'
        self._set_page(suite, host, doc, headers, FORCED_CAPTCHA_REQUEST, False, additional_query=additional_query)


class HtmlCaptchaPage(BaseHtmlCaptchaPage):
    def __init__(self, suite, host, doc="/yandsearch", headers=None, additional_query=""):
        headers = headers or {}
        if host.endswith('eda.yandex'):
            headers['X-Antirobot-MayBanFor-Y'] = '1'
        self._set_page(suite, host, doc, headers, FORCED_CAPTCHA_REQUEST, True, additional_query=additional_query)


class HtmlPage(BaseHtmlCaptchaPage):
    def __init__(self, suite, host, doc="/yandsearch", headers=None, additional_query=""):
        self._set_page(suite, host, doc, headers, "hello", True, additional_query=additional_query)


class InstantCaptchaPage(object):
    REQUEST = None

    def __init__(self, suite, host, additional_query=""):
        resp = suite.send_request(Fullreq(self.REQUEST % (
            host, FORCED_CAPTCHA_REQUEST) + additional_query, headers={'X-Real-Ip': GenRandomIP()}))
        assert resp.getcode() == 200
        self.page = resp.read().decode()
        self.content_type = resp.headers.get('Content-Type')

    def get_content(self):
        return self.page

    def get_content_type(self):
        return self.content_type


class JsonCaptchaPage(InstantCaptchaPage):
    REQUEST = r"http://%s/yandsearch?text=%s&ajax=1"


class JsonPCaptchaPage(InstantCaptchaPage):
    REQUEST = r"http://%s/yandsearch?text=%s&ajax=1&callback=func"


class XmlCaptchaPage(InstantCaptchaPage):
    REQUEST = r"http://%s/xmlsearch?user=user&key=key1&showmecaptcha=yes&query=%s"


class XmlNonBrandedPartnerCaptchaPage(InstantCaptchaPage):
    REQUEST = r"http://%s/xmlsearch?user=rambler-xml&key=key1&showmecaptcha=yes&query=%s"


class XmlImagesCaptchaPage(InstantCaptchaPage):
    REQUEST = r"http://%s/images-xml?user=user&key=key1&showmecaptcha=yes&query=%s"


class XmlImagesNonBrandedPartnerCaptchaPage(InstantCaptchaPage):
    REQUEST = r"http://%s/images-xml?user=rambler-xml&key=key1&showmecaptcha=yes&query=%s"


class BaseCheckCaptcha(object):
    REQUEST = None
    HTTP_CODE = None

    def __init__(self, suite, host, key, rep, retpath, additionalHeaders={}):
        headers = {'X-Real-Ip': GenRandomIP()}
        headers.update(additionalHeaders)
        resp = suite.send_fullreq(
            self.CHECK_REQUEST % {
                'host': host,
                'key': key,
                'rep': rep,
                'retpath': urllib.request.quote(retpath),
            },
            headers=headers,
        )
        assert resp.getcode() == self.HTTP_CODE
        self.page = resp.read().decode()
        self.content_type = resp.headers.get('Content-Type')

    def get_content(self):
        return self.page

    def get_content_type(self):
        return self.content_type


class HtmlCheckCaptcha(BaseCheckCaptcha):
    CHECK_REQUEST = r"http://%(host)s/checkcaptcha?key=%(key)s&rep=%(rep)s&retpath=%(retpath)s"
    HTTP_CODE = 302


class JsonCheckCaptcha(BaseCheckCaptcha):
    CHECK_REQUEST = r"http://%(host)s/checkcaptchajson?key=%(key)s&rep=%(rep)s"
    HTTP_CODE = 200


class JsonPCheckCaptcha(BaseCheckCaptcha):
    CHECK_REQUEST = r"http://%(host)s/checkcaptchajson?key=%(key)s&rep=%(rep)s&callback=func"
    HTTP_CODE = 200


class XmlCheckCaptcha(BaseCheckCaptcha):
    CHECK_REQUEST = r"http://%(host)s/xcheckcaptcha?key=%(key)s&rep=%(rep)s"
    HTTP_CODE = 200


def GetCaptchaParamsFromImageUrl(url, suite):
    resp = suite.send_request(Fullreq(url))
    return urllib.parse.parse_qs(urllib.parse.urlparse(resp.headers['Location']).query)


def GetCaptchaTypeFromImageUrl(url, suite):
    try:
        return GetCaptchaParamsFromImageUrl(url, suite)['type'][0]
    except KeyError:
        return "std"  # Default type in captcha api


def GetLanguageFromImageUrl(url, suite):
    try:
        return GetCaptchaParamsFromImageUrl(url, suite)['vtype'][0]
    except KeyError:
        return "ru"  # Default language in captcha api


def GetCaptchaPageUrlFromJsonResponse(jsonString):
    js = json.loads(jsonString)
    return js['captcha']['captcha-page']


def GetRetpathFromJsonResponse(jsonString):
    captchaUrl = GetCaptchaPageUrlFromJsonResponse(jsonString)
    return urllib.parse.parse_qs(urllib.parse.urlparse(captchaUrl).query)['retpath'][0]
