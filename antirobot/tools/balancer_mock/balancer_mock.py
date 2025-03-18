# -*- coding: utf-8 -*-

from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from SocketServer import ThreadingMixIn
from StringIO import StringIO
import argparse
import pprint
import random
import socket
import sys
import time
import urllib2


def ParseArgs():
    YANDEX_PREFIX_DEFAULT = 'https://yandex.ru'

    parser = argparse.ArgumentParser(description="Balancer mock for antirobot")
    parser.add_argument('-p', '--port', dest='port', action='store', type=int, help='port for incoming requests')
    parser.add_argument('--antirobot', dest='antirobot', action='store', help='antirobot host:port')
    parser.add_argument('--yandex', dest='yandexPrefix', action='store', default=YANDEX_PREFIX_DEFAULT, help='prefix for yandex requests. Default is %s' % YANDEX_PREFIX_DEFAULT)
    parser.add_argument('--antirobot-service', dest='antirobotService', action='store', help='add X-Antirobot-Service-Y with specified value')
    parser.add_argument('--add-header', dest='moreHeaders', metavar='"name: value"', action='append', help='add header to a fullreq')

    opts = parser.parse_args()
    try:
        if not opts.port:
            raise Exception("You must specify a port for incoming requests")

        if not opts.antirobot:
            raise Exception("You must specify target antirobot host and port")

        return opts
    except Exception, ex:
        print >>sys.stderr, str(ex)
        sys.exit(2)


class NoRedirection(urllib2.HTTPErrorProcessor):
    def http_response(self, request, response):
        return response

    https_response = http_response


def GetCurTimeInMicrosec():
    return time.time() * 1000000


def MakeReqid():
    randomPart = int(random.random() * 10000000000)
    return '9%d-%d' % (GetCurTimeInMicrosec(), randomPart)


class InputReq:
    def __init__(self, server, userAddr, method, path=None, headers=None, data=None, version='HTTP/1.1'):
        self.userAddr = userAddr
        self.method = method
        self.version = version
        self.path = path
        self.headers = headers
        self.data = data
        self.server = server
        self.AddAuxHeaders()

    def AddAuxHeaders(self):
        self.headers['X-Forwarded-For-Y'] = self.userAddr[0]
        self.headers['X-Source-Port-Y'] = str(self.userAddr[1])
        self.headers['X-Start-Time'] = str(int(time.time() * 1000000))
        self.headers['X-Req-Id'] = MakeReqid()

        if self.server.antirobotService:
            self.headers['X-Antirobot-Service-Y'] = self.server.antirobotService

        if self.server.moreHeaders:
            for val in self.server.moreHeaders:
                k, v = (x.strip() for x in val.split(':', 1))
                self.headers[k] = v

    def __str__(self):
        stream = StringIO()

        print >>stream, '%s %s %s' % (self.method, self.path, self.version)
        for header in self.headers:
            print >>stream, '%s: %s' % (header, self.headers[header])
        print >>stream
        if self.data:
            print >>stream, self.data

        return stream.getvalue()


def IsForUser(antirobotResp):
    return antirobotResp.headers.get('X-ForwardToUser-Y') == '1'


class RequestHandler(BaseHTTPRequestHandler):
    def AskAntirobot(self, inputReq):
        reqStr = 'http://%s/fullreq' % self.server.antirobotHost
        req = urllib2.Request(reqStr, str(inputReq))
        return server.urlOpener.open(req)

    def AskYandex(self, inputReq):
        reqStr = '%s%s' % (server.yandexPrefix, inputReq.path)
        req = urllib2.Request(reqStr, inputReq.data)
        for k in self.headers:
            req.add_header(k, self.headers[k])

        return server.urlOpener.open(req)

    def SendToUser(self, resp, body=None):
        if not resp:
            self.send_response(500, "Empty 'resp' to send")
            self.end_headers()
            return

        if body is None:
            body = resp.read()

        self.send_response(resp.getcode())
        for k, v in resp.headers.items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(body)

    def HandleInputReq(self, inputReq):
        antirobotResp = self.AskAntirobot(inputReq)
        antirobotRespBody = antirobotResp.read()

        pprint.pprint({
            "antirobotResp": antirobotResp.__dict__,
            "antirobotResp.headers": dict(antirobotResp.headers),
            "antirobotRespBody": antirobotRespBody,
        })

        if IsForUser(antirobotResp):
            self.SendToUser(antirobotResp, body=antirobotRespBody)
        else:
            self.SendToUser(self.AskYandex(inputReq))

    def do_GET(self):
        inputReq = InputReq(self.server, self.client_address, method='GET', path=self.path,
                            headers=self.headers, version=self.request_version)
        self.HandleInputReq(inputReq)

    def do_POST(self):
        contentLength = int(self.headers.get('Content-Length', 0))
        inputReq = InputReq(self.server,
                            self.client_address, method='POST', path=self.path,
                            headers=self.headers,
                            data=self.rfile.read(contentLength),
                            version=self.request_version)

        self.HandleInputReq(inputReq)


class Balancer(ThreadingMixIn, HTTPServer):
    address_family = socket.AF_INET6

    def __init__(self, opts):
        HTTPServer.__init__(self, ('', opts.port), RequestHandler)

        self.antirobotHost = opts.antirobot
        self.yandexPrefix = opts.yandexPrefix
        self.urlOpener = urllib2.build_opener(NoRedirection)
        self.antirobotService = opts.antirobotService
        self.moreHeaders = opts.moreHeaders


def main():
    try:
        opts = ParseArgs()
        global server
        server = Balancer(opts)

        print "started httpserver..."
        server.serve_forever()
    except KeyboardInterrupt:
        print "^C received, shutting down server"
        server.socket.close()


if __name__ == "__main__":
    main()
