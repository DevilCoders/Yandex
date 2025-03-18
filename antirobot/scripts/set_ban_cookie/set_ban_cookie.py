#!/usr/bin/env python2.6
# -*- coding: utf-8 -*-

import BaseHTTPServer
import sys

class RequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html; charset=UTF-8")
        self.send_header("Accept-Charset", "utf-8;q=0.7,*;q=0.7")
        self.send_header("Set-Cookie", "YX_SHOW_CAPTCHA=1; domain=.yandex.ru; path=/; expires=Wed, 01-Aug-2012 16:05:28 GMT")
        self.end_headers()
        self.wfile.write("""
        <html><body>Ban cookie has been set</body><html>
        """)

def Usage():
    print >>sys.stderr, "Usage: %s <port>" % sys.argv[0].split("/")[-1];

if __name__ == "__main__":
    try:
        if len(sys.argv) < 2:
            Usage();
            sys.exit(1);

        port = int(sys.argv[1]);

        server = BaseHTTPServer.HTTPServer(("", port), RequestHandler)
        print "started httpserver..."
        server.serve_forever()
    except KeyboardInterrupt:
        print "^C received, shutting down server"
        server.socket.close()
