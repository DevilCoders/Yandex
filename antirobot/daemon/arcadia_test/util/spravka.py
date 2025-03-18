import urllib.parse
import http.cookies
import urllib
import json

from antirobot.daemon.arcadia_test.util import (
    Fullreq,
    captcha_page,
)
from antirobot.daemon.arcadia_test.util.asserts import (
    AssertRedirectNotToCaptcha,
)


SET_COOKIE_HEADER = "Set-Cookie"


def DoCheckCaptcha(suite, ip, captcha, rep="123", content=None):
    check_req = captcha.get_check_captcha_url() + f"&rep={urllib.parse.quote(rep)}"

    if content is None:
        js_print = {
            "userAgent": "Some UA",
            "geo": True,
        }
        content = "verochka-data=" + urllib.parse.quote(json.dumps(js_print))
    return suite.send_request(Fullreq(check_req,
                                      headers={'X-Forwarded-For-Y': ip},
                                      content=content,
                                      method="POST"))


def GetSpravkaForAddr(suite, ip):
    captcha = captcha_page.HtmlCaptchaPage(suite, "yandex.ru")

    resp = DoCheckCaptcha(suite, ip, captcha)
    AssertRedirectNotToCaptcha(resp, msg="Possibly you need to set '--check success' captcha_args")

    assert SET_COOKIE_HEADER in resp.info()

    cookObj = http.cookies.SimpleCookie(resp.info()[SET_COOKIE_HEADER])
    assert "spravka" in cookObj

    spravkaObj = cookObj["spravka"]
    assert spravkaObj.value

    return spravkaObj.value
