import json
import re
import subprocess
import time
import urllib.parse
from xml.etree import ElementTree

import pytest

import yatest.common


def AssertFloatsEqual(x, y, eps):
    assert pytest.approx(x, eps) == y


def AssertMatches(pattern, s):
    assert re.search(pattern, s, re.DOTALL)


def AssertResponseForUser(resp):
    header = "X-ForwardToUser-Y"
    assert resp.info()[header] == "1"


def AssertResponseForBalancer(resp):
    header = "X-ForwardToUser-Y"
    assert resp.info()[header] == "0"


def AssertNotRobotResponse(resp):
    assert resp.getcode() == 200
    AssertResponseForBalancer(resp)


def AssertCaptchaRedirect(resp):
    assert resp.getcode() == 302
    AssertResponseForUser(resp)

    locHeader = "Location"
    location = resp.info()[locHeader]
    assert urllib.parse.urlsplit(location).path == "/showcaptcha"


def AssertRedirectNotToCaptcha(resp, msg="Expected captcha redirect"):
    assert resp.getcode() == 302
    AssertResponseForUser(resp)

    locHeader = "Location"
    location = resp.info()[locHeader]
    assert urllib.parse.urlsplit(location).path != "/showcaptcha", msg


def AssertJsonCaptchaResponse(resp, code=200):
    assert resp.getcode() == code
    AssertResponseForUser(resp)

    content = resp.read()
    try:
        vals = json.loads(content)
    except ValueError:
        assert False, "Invalid JSON: " + content

    assert vals is not None
    assert "key" in vals["captcha"]


def AssertEventuallyTrue(function, secondsBetweenCalls=0.1, waitTimeout=180):
    '''При waitTimeout=inf тест должен упасть по таймауту, если эта проверка не прошла'''
    while waitTimeout > 0:
        if function():
            return
        time.sleep(secondsBetweenCalls)
        waitTimeout -= secondsBetweenCalls
    assert False


def AssertSpravkaValid(suite, spr, domain, key):
    bin_path = yatest.common.build_path(
        "antirobot/tools/check_spravka/check_spravka",
    )

    subprocess.check_call(
        [bin_path, "--domain", domain, "--keys", suite.keys_path(), "--data-key", key, spr],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    # TODO: похоже, функция ничего не проверяет


def AssertBlocked(resp):
    assert resp.getcode() == 403


def AssertNotBlocked(resp):
    assert resp.getcode() != 403


def AssertServerError(resp):
    assert resp.getcode() == 500


def AssertNotServerError(resp):
    assert resp.getcode() != 500


def AssertEventuallyBlocked(suite, request):
    AssertEventuallyTrue(
        lambda: suite.send_request(request).getcode() == 403,
        secondsBetweenCalls=0.5,
    )


def AssertEventuallyNotBlocked(suite, request):
    AssertEventuallyTrue(
        lambda: suite.send_request(request).getcode() != 403,
        secondsBetweenCalls=0.5,
    )


def AssertXmlCaptcha(resp):
    resp_body = resp.read()
    assert resp.getcode() == 200, f"Response code {resp.getcode()}: '{resp_body.decode()}'"

    assert resp_body, "Empty response isn't correct XML"
    try:
        xml = ElementTree.fromstring(resp_body)
    except:
        assert False, '"' + resp_body.decode() + '''" isn't correct XML'''

    assert xml.tag == "yandexsearch"
    xml = xml.find("response")
    assert xml is not None
    xml = xml.find("error")
    assert xml is not None
    assert xml.attrib["code"] == "100"
