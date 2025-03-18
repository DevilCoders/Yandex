#!/usr/bin/env python2.6
# -*- coding: utf-8 -*-

import cgi
import BaseHTTPServer
import httplib
import urllib
import Cookie
import xsl_transform
import re
import xml.etree.ElementTree as xmltree
import sys

YANDEX_HOST = "xmlsearch.yandex.ru"
YANDEX_PORT = 80
YANDEX_TIMEOUT = 10
YANDEX_QUERY="/xmlsearch?showmecaptcha=yes&user=partner&key=xml-key"

MY_DOMAIN = ".blah-blah.ru"

CaptchaTemplate = "<html>Captcha page here</html>"

CAPTCHA_ERROR_CODE = 100
ACCESS_BLOCKED_CODE = 101

def QueryToXmlQuery(query):
    return """<?xml version='1.0' encoding='UTF-8'?>
<request><query>%s</query></request>""" % query 

XsltSrc = None

def GetYaParams(cgiParams):
    result = ''
    for k in cgiParams.keys():
        if k.startswith('ya'):
            result += '&' + k + '=' + ''.join(cgiParams[k])

    return result

def PassSpravkaCookie(setCookieHeader):
    cookie = Cookie.BaseCookie(setCookieHeader)
    spr_cook = cookie.get("spravka")
    if spr_cook:
        cook_str = spr_cook.OutputString()
        cook_str = re.sub("domain\s*=([^\s;]*)", "", cook_str)
        cook_str += ";domain=" + MY_DOMAIN
        return cook_str
    else:
        return ''

def ParseCaptchaXml(xml):
    if not xml:
        return None;

    try:
        resp_tree = xmltree.fromstring(xml)
        
        error = resp_tree.find("response/error")
        if error == None:
            return None

        result = {}
        result["error_code"] = int(error.attrib["code"])

        if result["error_code"] != CAPTCHA_ERROR_CODE:
            return result

        result["img_url"] = resp_tree.find("captcha-img-url").text
        result["key"] = resp_tree.find("captcha-key").text
        result["status"] = resp_tree.find("captcha-status").text
        
        return result
    except Exception as ex:
        print >>sys.stderr, ex
        return None

class RequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def SendIndex(self):
            f = open("index.html")
            
            self.send_response(200)
            self.send_header("Content-type", "text/html; charset=UTF-8")
            self.send_header("Accept-Charset", "utf-8;q=0.7,*;q=0.7")
            self.end_headers()
            self.wfile.write(f.read())
            f.close()

    def HandleSearchQuery(self):
            
            search_req = self.CgiParams['request']
            if search_req:
                search_req = search_req[0]

                # Делаем xml запрос в Яндекс
                resp = self.RequestYandex(search_req)
                self.HandleYandexResponse(resp)

            else:
                self.send_response(404)
                self.end_headers()

    def RequestYandex(self, search_req):
            conn = httplib.HTTPConnection(YANDEX_HOST, YANDEX_PORT, YANDEX_TIMEOUT)
            query = YANDEX_QUERY
      
            yaParams = GetYaParams(self.CgiParams)
            if yaParams:
                query += yaParams

            headers = \
                    {
                        'User-Agent'    : self.headers['User-Agent'],
                        'Accept'        : 'application/xhtml+xml,application/xml',
                        'Accept-Language' : 'ru,en-us',
                        'Accept-Charset' : 'utf-8',
                        'Referer'       : 'http://' + self.headers['host'] + self.path,
                        'X-Real-Ip'     : self.client_address[0], # пробросить IP клиента
                        'Connection'    : 'close', 
                    }

            # Пробросить куку spravka
            spr = self.Cookies.get('spravka', None)
            if spr:
                headers['Cookie'] =  spr.OutputString()

            conn.request(\
                    'POST', 
                    query,
                    QueryToXmlQuery(search_req),
                    headers
                    )
            return conn.getresponse()
            
    def HandleYandexResponse(self, resp):
        if resp.status == 200:
            xml_rep = resp.read()

            captcha = ParseCaptchaXml(xml_rep)
            if captcha:
                if captcha["error_code"] == CAPTCHA_ERROR_CODE:
                    retPath = urllib.quote_plus("http://" + self.host + self.path)
                    self.SendCaptchaPage(retPath, captcha["img_url"], captcha["key"], captcha["status"])
                elif captcha["error_code"] == ACCESS_BLOCKED_CODE:
                    self.send_response(403)
                    self.send_header('Content-Type', 'text/html; charset=UTF-8')
                    self.end_headers()
                    self.wfile.write(open('forbidden.html').read())
            else:
                res = xsl_transform.Transform(xml_rep, XsltSrc)
                self.send_response(200)
                self.send_header('Content-Type', 'text/html; charset=UTF-8')
                self.end_headers()
            
                self.wfile.write(res)
        else:
            # Какая-то тошибка - переправляем ответ яндекса как есть
            self.send_response(resp.status)

            for i in resp.getheaders():
                self.send_header(i[0], i[1])
            self.end_headers()

            self.wfile.write(resp.read())
            
    def SendCaptchaPage(self, retPath, captcha_img_url, captcha_key, status):
        html = CaptchaTemplate.replace('%IMG_URL%', captcha_img_url) 
        html = html.replace("%CAPTCHA_KEY%", captcha_key)
        html = html.replace("%RET_PATH%", retPath)

        if (status):
            html = html.replace("%STATUS%", "Вы неверно ввели цифровой код.")
        else:
            html = html.replace("%STATUS%", "")
        
        self.send_response(200)
        self.send_header('Content-type', 'text/html; charset=UTF-8')
        self.end_headers()
        
        self.wfile.write(html)

    def HandleCheckCaptcha(self):
        key = self.CgiParams['key'] 
        if len(key): key = key[0]
        rep = self.CgiParams['rep']
        if len(rep): rep = rep[0]

        retPath = self.CgiParams['retpath']
        if len(retPath): retPath = retPath[0]

        # Отправляем в Яндекс запрос на проверку капчи
        conn = httplib.HTTPConnection(YANDEX_HOST, YANDEX_PORT, YANDEX_TIMEOUT)
        host = YANDEX_HOST
        query = "/xcheckcaptcha?key=%s&rep=%s" % (urllib.quote(key), urllib.quote(rep))
      
        conn.request(\
                    'GET', 
                    query,
                    None,
                    {
                        'X-Real-Ip'     : self.client_address[0], # пробросить IP клиента
                    }
                    )
        resp = conn.getresponse()
        
        status = resp.status
        content = resp.read()
        # Если капча введена неправильно, яндекс снова вернёт xml с капчей
        captcha = ParseCaptchaXml(content)

        if status != 200 or captcha == None:
            # Редиректим на оригинальный запрос
            # Устанавливаем также куку spravka в своём домене
            self.send_response(302)
            self.send_header('location', urllib.unquote(retPath))
            setCookieHeader = resp.getheader("set-cookie", None)
            if setCookieHeader:
                self.send_header('set-cookie', PassSpravkaCookie(setCookieHeader))
        elif captcha["error_code"] == CAPTCHA_ERROR_CODE:
            # Снова показываем капчу
            self.SendCaptchaPage(retPath, captcha["img_url"], captcha["key"], captcha["status"])
        elif captcha["error_code"] == ACCESS_BLOCKED_CODE:
            self.send_response(403)
            self.send_header('Content-Type', 'text/html; charset=UTF-8')
            self.end_headers()
            self.wfile.write(open('forbidden.html').read())

        self.end_headers()

    def do_GET(self):
        self.CgiString = ''
        self.Cookies = Cookie.BaseCookie()
        self.host = self.headers.get('host')

        sp = self.path.split('?')

        if len(sp) > 1:
            self.CgiString = sp[1]

        self.CgiParams = cgi.parse_qs(self.CgiString)

        cookies = self.headers.get('Cookie', None)
        if cookies:
            self.Cookies.load(cookies)

        if sp[0] == "/" or self.path == "/index.html":
            self.SendIndex()
        elif sp[0] == "/q":
            # Обработка поискового запроса
            self.HandleSearchQuery()
        elif sp[0] == "/checkcaptcha":
            # Проверка капчи
            self.HandleCheckCaptcha()
        else:
            self.send_response(404)
            self.end_headers()

def Usage():
    print >>sys.stderr, "Usage: %s <port>" % sys.argv[0].split("/")[-1];

if __name__ == "__main__":
    try:
        if len(sys.argv) < 2:
            Usage();
            sys.exit(1);

        port = int(sys.argv[1]);

        f = open("yand.xsl")
        XsltSrc = f.read()
        f.close()

        f = open("captcha.html")
        CaptchaTemplate = f.read()
        f.close()

        server = BaseHTTPServer.HTTPServer(("", port), RequestHandler)
        print "started httpserver..."
        server.serve_forever()
    except KeyboardInterrupt:
        print "^C received, shutting down server"
        server.socket.close()
