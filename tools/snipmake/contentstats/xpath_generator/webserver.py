#!/usr/bin/env python

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

HostXPath = {}

def CleanBackgroundImage(root):
    for child in root.getchildren():
        child.attrib["style"] = "background-image:none; background-color:red"
        CleanBackgroundImage(child)

def PaintNodes(docTree, xpathStr):
    r = docTree.xpath(xpathStr)
    for node in r:
        CleanBackgroundImage(node)
        node.attrib["style"] = "background-color:red;"


def ProcessPage(url):
    try:
        url_parse = urlparse(url)
        response = urllib2.urlopen(url)
        html = response.read()
        tree = etree.HTML(html)

        host = url_parse.netloc.replace("www.", "")
        print host
        print len( HostXPath[host])
        for xpath_str in HostXPath[host]:
            PaintNodes(tree, "/html/body" + xpath_str)
        return etree.tostring(tree)
    except:
        return "<b style='color:red'>Error occured. There are several reasons of this error: <br/>1. You've choosen the host which hasn't been crawled yet.</br> 2. Resource hasn't answered</b>"

def LoadXPathDict(fileName):
    with open(fileName, "r") as file:
        for line in file:
            fields = line.split("\t")
            if not fields[0] in HostXPath:
                HostXPath[fields[0]] = []
            HostXPath[fields[0]].append(fields[1])

def GetWebUrl(path):
    queryParams = {}
    parts = path.split('?', 1)
    if len(parts) > 1:
        path = parts[0]
        queryParams = cgi.parse_qs(parts[1], True)
    web_url = ""
    if "web_url" in queryParams:
        web_url = queryParams["web_url"][0]
    return web_url

class Handler(BaseHTTPServer.BaseHTTPRequestHandler):
    def sendError(self, code):
        self.send_error(code)
        self.wfile.close()

    def do_GET(self):
        web_url = GetWebUrl(self.path)

        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=windows-1251')
        self.end_headers()
        self.wfile.write("<form id=\"main_form\"><input name=\"web_url\" size='100' value='%s'><br/><input type=\"submit\" value=\"Submit\"></form>" % web_url)

        if len(web_url) > 0:
            self.wfile.write("<div style=\"padding: 50px; border: Solid 2px Green \">")
            if string.find(web_url, "http://") != 0:
                web_url = "http://" + web_url
            self.wfile.write(ProcessPage(web_url))
            self.wfile.write("</div>")

        self.wfile.close()

if len(sys.argv) != 2:
    print "Command line must have the following format: webserver.py XPATH_FILE PORT"
    sys.exit(0)

LoadXPathDict(sys.argv[1])
print "Server is ready"

server_address = (socket.gethostname(), sys.argv[2])
httpd = BaseHTTPServer.HTTPServer(server_address, Handler)
httpd.serve_forever()
