#!/usr/bin/env python

import json
import codecs
import BaseHTTPServer
import subprocess
import os.path
import string
import cgi
import sys
import socket
import urllib2
from base64 import b64decode
from lxml import etree
from urlparse import urlparse
import util

HostXPath = {}
Owners = None

def LoadXPathDict(fileName):
    result = {}
    for line in util.read_lines(fileName):
        fields = line.split("\t", 1)
        if len(fields) != 2:
            continue
        owner, xpath = fields
        stuff = result.get(owner, None)
        if not stuff:
            stuff = []
            result[owner] = stuff
        stuff.append(xpath)
    return result

def get_url(path):
    queryParams = {}
    parts = path.split('?', 1)
    if len(parts) > 1:
        path = parts[0]
        queryParams = cgi.parse_qs(parts[1], True)
    web_url = ""
    if "url" in queryParams:
        web_url = queryParams["url"][0]
    return web_url

class Handler(BaseHTTPServer.BaseHTTPRequestHandler):
    def sendError(self, code):
        self.send_error(code)
        self.wfile.close()

    def do_GET(self):
        owner = util.owner_name(get_url(self.path), Owners)
        self.send_response(200)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.end_headers()
        xpaths = {'xpaths' : HostXPath.get(owner, [])}
        json.dump(xpaths, self.wfile, ensure_ascii=False)
        self.wfile.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print "Command line must have the following format: webserver.py XPATH_FILE PORT"
        sys.exit(1)

    HostXPath = LoadXPathDict(sys.argv[1])
    Owners = util.load_owners()

    print "Starting server"

    server_address = (socket.gethostname(), int(sys.argv[2]))
    httpd = BaseHTTPServer.HTTPServer(server_address, Handler)
    httpd.serve_forever()

