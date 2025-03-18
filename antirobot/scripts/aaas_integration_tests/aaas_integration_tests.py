#!/usr/bin/env python
# coding: utf-8

import os
import imp
import re
import time
import socket
import subprocess
import argparse
import threading
from urlparse import urlparse
import urllib2
import cgi
import SimpleHTTPServer
import SocketServer
from termcolor import colored
import selenium.common.exceptions
from selenium import webdriver
from selenium.webdriver.common.desired_capabilities import DesiredCapabilities

if __name__ == '__main__':
    hookSourceFile = os.path.join('..', '..', '..', 'devtools', 'fleur', 'imports', 'installhook.py')
    imp.load_source('installhook', os.path.join(os.path.dirname(__file__), hookSourceFile))

from devtools.fleur.util.selenium import Guard as SeleniumGuard

MAGIC_COOKIE_NAME = 'YX_SHOW_CAPTCHA'
MAGIC_QUERY = 'search?text=Капчу!'

COLOR_GOOD = 'green'
COLOR_ERROR = 'red'
COLOR_INPUT = 'yellow'
COLOR_URL = 'blue'
COLOR_MINOR = 'grey'
COLOR_MAJOR = 'cyan'

def ResultColor(conditionIsGood):
    return COLOR_GOOD if conditionIsGood else COLOR_ERROR

def ResultColored(resultValue, conditionIsGood):
    return colored(str(resultValue), ResultColor(conditionIsGood))

XPATH_CAPTCHA_IMAGE = '//img[@class="image form__captcha"]'
XPATH_METRIKA = '//script[@type="text/javascript"]'
XPATH_YANDEX_INTENET_IP_TEMPLATE = "//*[@class='client__item client__item_type_ipv%d']//*[@class='client__desc']"

MESSAGES = {
    'BASE_URL': 'Base url for testing:',
    'CACHERS': 'Cachers:',
    'CALC_CACHERS': 'Calc cachers',
    'CAPTCHA_TOKEN': 'Captcha token:',
    'EVLOG_FOUND_ADDRS': 'EventLog record count for Selenium IP\'s:',
    'EVLOG_MESSAGES_TYPES': 'EventLog records counts:',
    'EVLOG_RECORDS': 'EventLog record count for requested Captcha token:',
    'FEEDBACK_RESPONSE_HTTP_CODE': 'Feedback page HTTP response code:',
    'FEEDBACK_URL': 'Feedback URL:',
    'HTTP_SERVER_START': 'Http serving at port:',
    'HTTP_SERVER_STOP': 'Http server stopping',
    'IMAGE_SOURCE': 'Captcha image URL:',
    'INPUT_CODE': 'Input captcha symbols: ',
    'INPUT_EXIT': 'Press ENTER key for exit',
    'IS_CAPTCHA_PAGE': 'Current doc is Captcha page:',
    'LOG_INVESTIGATION': 'Start log records investigation. Please, wait',
    'NEW_IS_CAPTCHA_PAGE': 'Doc after captcha input is Captcha page:',
    'NEW_TITLE': 'Page title after captcha input:',
    'NEW_URL': 'URL after captcha input:',
    'NOT_FOUND': 'not found',
    'SAVED_SCREEN': 'Captcha page screenshot:',
    'SELENIUM_IP4': 'Selenium IPv4:',
    'SELENIUM_IP6': 'Selenium IPv6:',
    'TITLE': 'Page title:',
    'TEST_MAGIC_COOKIE': "MagicCookie testing: %s" % MAGIC_COOKIE_NAME,
    'TEST_MAGIC_QUERY': "MagicQuery testing: %s" % MAGIC_QUERY,
    'URL': 'Current URL:',
    'YA_METRIKA_ID': 'Yandex.Metrika ID:',
}


def Msg(key):
    return MESSAGES[key] if key in MESSAGES else key


def Split(char, size=80, color=None):
    if color is None:
        print char * size
    else:
        print colored(char * size, color)


class HttpServer:
    def __init__(self, port):
        self.Port = port
        self.Handler = SimpleHTTPServer.SimpleHTTPRequestHandler
        SocketServer.TCPServer.allow_reuse_address = True
        self.HttpServer = SocketServer.TCPServer(('', port), self.Handler)
        print colored("%s %d" % (Msg('HTTP_SERVER_START'), port), COLOR_MINOR)
        self.ServerThread = threading.Thread(target=self.HttpServer.serve_forever)
        self.ServerThread.start()

    def Shutdown(self):
        print colored(Msg('HTTP_SERVER_STOP'), COLOR_MINOR)
        self.HttpServer.shutdown()
        self.HttpServer.server_close()
        self.ServerThread.join()

    def MakeServedUrl(self, fileName):
        return "http://%s:%d/%s" % (socket.gethostname(), self.Port, fileName)


class TestSuiteAntirobotWithServiceIntegration:
    def __init__(self, webDriver, baseUrl, httpServer, ip2backendAllVerticals):
        self.WebDriver = webDriver
        self.BaseUrl = baseUrl
        self.CaptchaToken = None
        self.SeleniumAddr = self.GetSeleniumIp()
        self.HttpServer = httpServer
        self.Ip2backendAllVerticals = ip2backendAllVerticals
        print Msg('SELENIUM_IP4'), self.SeleniumAddr.Ipv4
        print Msg('SELENIUM_IP6'), self.SeleniumAddr.Ipv6

    def GetSeleniumIp(self):
        class Addr:
            Ipv4 = None
            Ipv6 = None
        addr = Addr()
        self.WebDriver.get('https://yandex.ru/internet')

        def getIp(ipVersion):
            return self.WebDriver.find_element_by_xpath(XPATH_YANDEX_INTENET_IP_TEMPLATE % ipVersion).text
        addr.Ipv4 = getIp(4)
        addr.Ipv6 = getIp(6)
        return addr

    def IsCaptchaPage(self, pageSource):
        # Эвристика, что на странице найдутся все ключи.
        keys = [
            'IP',
            'автоматически',
            'введите символы',
        ]
        foundKeys = 0
        for key in keys:
            foundKeys += int(pageSource.find(key.decode('utf-8')) >= 0)
        return foundKeys == len(keys)

    def TestFeedback(self):
        try:
            feedbackUrl = self.WebDriver.find_element_by_partial_link_text('обратной связи'.decode('utf-8')).get_attribute('href')
            print Msg('FEEDBACK_URL'), feedbackUrl
            feedbackHttpCode = urllib2.urlopen(feedbackUrl).getcode()
            print Msg('FEEDBACK_RESPONSE_HTTP_CODE'), ResultColored(feedbackHttpCode, feedbackHttpCode == 200)
        except selenium.common.exceptions.NoSuchElementException:
            print Msg('FEEDBACK_URL'), colored(Msg('NOT_FOUND'), COLOR_ERROR)

    def TestMetrika(self):
        metrikaFound = False
        for script in self.WebDriver.find_elements_by_xpath(XPATH_METRIKA):
            scriptText = script.get_attribute('text')  # не используется поле .text, т.к. в нем содержится только видимый контент.
            if len(scriptText) and scriptText.index('Ya.Metrika') >= 0:
                print Msg('YA_METRIKA_ID'), re.search('id:(\d*)', scriptText).group(1)
                metrikaFound = True
        if not metrikaFound:
            print Msg('YA_METRIKA_ID'), colored(Msg('NOT_FOUND'), COLOR_ERROR)

    def MakeScreenShot(self):
        foutName = 'screen-%d.png' % time.time()
        self.WebDriver.get_screenshot_as_file(foutName)
        print Msg('SAVED_SCREEN'), colored(self.HttpServer.MakeServedUrl(foutName), COLOR_URL)

    def TestCaptchaPage(self):
        captchaToken = cgi.parse_qs(urlparse(self.WebDriver.current_url).query)['t'][0]
        print Msg('CAPTCHA_TOKEN'), captchaToken
        self.TestFeedback()
        self.TestMetrika()
        self.MakeScreenShot()
        return captchaToken

    def InputCaptchaValue(self):
        print Msg('IMAGE_SOURCE'), colored(self.WebDriver.find_element_by_xpath(XPATH_CAPTCHA_IMAGE).get_attribute('src'), COLOR_URL)
        captchaResponse = raw_input(colored(Msg('INPUT_CODE'), COLOR_INPUT))
        self.WebDriver.find_element_by_id('rep').send_keys(captchaResponse.decode('utf-8'))
        self.WebDriver.find_element_by_tag_name('form').submit()

    def CheckCaptcha(self, justOneInputAttempt=False):
        print Msg('URL'), self.WebDriver.current_url
        print Msg('TITLE'), self.WebDriver.title
        isCaptchaPage = self.IsCaptchaPage(self.WebDriver.page_source)
        print Msg('IS_CAPTCHA_PAGE'), ResultColored(isCaptchaPage, isCaptchaPage)
        captchaPageTested = False
        captchaToken = None
        while isCaptchaPage:
            if not captchaPageTested:
                captchaToken = self.TestCaptchaPage()
                captchaPageTested = True
            self.InputCaptchaValue()
            print Msg('NEW_URL'), self.WebDriver.current_url
            print Msg('NEW_TITLE'), self.WebDriver.title
            isCaptchaPage = self.IsCaptchaPage(self.WebDriver.page_source)
            print Msg('NEW_IS_CAPTCHA_PAGE'), isCaptchaPage if justOneInputAttempt else ResultColored(isCaptchaPage, not isCaptchaPage)
            if justOneInputAttempt:
                break
        if captchaToken:
            self.CheckLog(captchaToken, self.SeleniumAddr)

    def CalcCachers(self, addr):
        def Impl(ip):
            if ip is None:
                return []
            return subprocess.check_output([
                self.Ip2backendAllVerticals,
                '--fields',
                'Cachers',
                '--dont-print-fieldname',
                '--noupdate',  # XXX just for development
                ip,
            ]).rstrip().split('\n')
        cachers = Impl(addr.Ipv4) + Impl(addr.Ipv6)
        assert len(cachers) > 0, 'empty cachers list'
        return cachers

    def GrepEventLog(self, captchaToken, cachers):
        evlogGrepCmd = ('/db/iss3/services/13512/active/production_antirobot_iss#production_antirobot_iss-*/evlogdump'
                        ' /db/www/logs/current-antirobot_events.log | fgrep %s || true' % captchaToken)
        print colored(Msg('LOG_INVESTIGATION'), COLOR_MINOR, attrs=['bold'])
        logRecordsRaw = subprocess.check_output([
            'sky',
            'run',
            '-U',
            '--cqudp',
            evlogGrepCmd,
        ] + cachers)
        return logRecordsRaw

    def AnalyzeEventLog(self, logRecordsRaw, addr):
        evlogRecodsNum = 0
        evlogFoundAddrs = 0
        messagesTypes = dict()
        for logLine in logRecordsRaw.rstrip().split('\n'):
            if 'THeader' in logLine:
                evlogRecodsNum += 1
                parsedLogLine = logLine.split('\t')
                messageType = parsedLogLine[2]
                userAddr = parsedLogLine[6]
                if userAddr == addr.Ipv4 or userAddr == addr.Ipv6:
                    evlogFoundAddrs += 1
                if messageType in messagesTypes:
                    messagesTypes[messageType] += 1
                else:
                    messagesTypes[messageType] = 1
        # Выводим статистики.
        print Msg('EVLOG_RECORDS'), ResultColored(evlogRecodsNum, evlogRecodsNum > 0)
        evlogFoundAddrsColor = ResultColor(evlogRecodsNum and evlogFoundAddrs == evlogRecodsNum)
        evlogFoundAddrsRel = 100.0 * evlogFoundAddrs / evlogRecodsNum if evlogRecodsNum else 0.0
        print "%s: %s (%s%%)" % (Msg('EVLOG_FOUND_ADDRS'),
                                 colored(str(evlogFoundAddrs), evlogFoundAddrsColor),
                                 colored(str(evlogFoundAddrsRel), evlogFoundAddrsColor))
        print Msg('EVLOG_MESSAGES_TYPES'),  messagesTypes

    def CheckLog(self, captchaToken, addr):
            # Получаем список кешеров.
            print colored(Msg('CALC_CACHERS'), COLOR_MINOR)
            cachers = self.CalcCachers(addr)
            print Msg('CACHERS'), cachers

            # Грепаем лог на кешерах.
            logRecordsRaw = self.GrepEventLog(captchaToken, cachers)
            print colored(logRecordsRaw, COLOR_MINOR)

            # Анализируем лог с кешеров.
            self.AnalyzeEventLog(logRecordsRaw, addr)

    def TestMagicQuery(self):
        print colored(Msg('TEST_MAGIC_QUERY'), COLOR_MAJOR)
        self.WebDriver.get(self.BaseUrl + MAGIC_QUERY)
        self.CheckCaptcha(justOneInputAttempt=True)  # капча с MagicQuery - непроходимая.

    def TestMagicCookie(self):
        print colored(Msg('TEST_MAGIC_COOKIE'), COLOR_MAJOR)
        owner = '.'.join(urlparse(self.BaseUrl).hostname.rsplit('.', 2)[-2:])
        self.WebDriver.get(self.BaseUrl)
        self.WebDriver.AddCookies([
            {
                'name': MAGIC_COOKIE_NAME,
                'value': '1',
                'domain': '.' + owner,
                'path': '/',
            },
        ])
        self.WebDriver.refresh()
        self.CheckCaptcha()

    def Run(self):
        # В каждой из Test-функций .get() явно вызывается только один раз.
        print Msg('BASE_URL'), self.BaseUrl
        Split('-', color=COLOR_MINOR)
        self.TestMagicQuery()
        Split('-', color=COLOR_MINOR)
        self.TestMagicCookie()


def ParseArgs():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('base_url', metavar='BASE_URL', help='base url for testing. eg: https://myservice.yandex.ru/ or http://www.kinopoisk2.ru/news/')
    parser.add_argument('--selenium-login', help='Login string to Selenium-grid', default='http://test:test@sg.yandex-team.ru:4444/wd/hub')
    parser.add_argument('--http-port', help='Port for internal http server', type=int, default=8915)
    parser.add_argument('--ip2backend-all-verticals', help='Path to ip2backend-all-verticals.py', default='ip2backend-all-verticals.py')
    return parser.parse_args()


def main():
    ops = ParseArgs()
    httpServer = HttpServer(ops.http_port)
    try:
        with SeleniumGuard(webdriver.Remote(ops.selenium_login, desired_capabilities=DesiredCapabilities.FIREFOX)) as webDriver:
            TestSuiteAntirobotWithServiceIntegration(webDriver, ops.base_url, httpServer, ops.ip2backend_all_verticals).Run()
        raw_input(colored(Msg('INPUT_EXIT'), COLOR_INPUT))
    finally:
        httpServer.Shutdown()

if __name__ == "__main__":
    main()
