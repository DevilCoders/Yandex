# -*- coding: utf-8 -*-

import sys
import urllib
from collections import defaultdict
import traceback
import time
import socket
import argparse
import urllib.parse
import re

from http.server import BaseHTTPRequestHandler, HTTPServer
from io import StringIO


SKEW = 0
RULE_ID = 1
RANGE_TYPE_TO_PARAMS = {
    "ip": {"range_src", "range_dst"},
    "txt": {"range_txt"},
    "re": {"range_re"},
}


class Record:
    def __init__(
        self,
        rule_id=None,
        range_src=None,
        range_dst=None,
        range_txt=None,
        range_re=None,
        expire=None,
        description=None
    ):
        params = (range_src or range_dst, range_txt, range_re)
        num_params = sum(1 for param in params if param is not None)

        if num_params != 1:
            raise Exception("expected exactly one of range_src/range_dst, range_txt and range_re")

        param_types = ("ip", "txt", "re")
        param_index = next(i for i, param in enumerate(params) if param is not None)
        self.range_type = param_types[param_index]

        self.rule_id = f"rule_id={rule_id}"
        self.range_src = range_src
        self.range_dst = range_dst
        self.range_re = range_re
        self.expire = expire or 0
        self.description = description
        self.range_txt = urllib.parse.unquote(range_txt) if range_txt else None

    def Format(self, formatList, flag):
        items = []

        def Str(x):
            return '' if x is None else str(x)

        params = RANGE_TYPE_TO_PARAMS[self.range_type]

        for p in formatList:
            if p == 'flag':
                items.append(str(flag))
            elif not p.startswith("range_") or p in params:
                items.append(Str(getattr(self, p)))
            else:
                raise Exception(
                    "invalid format param {} for record type {}".format(p, self.range_type)
                )

        return '; '.join(items)


class Group:
    def __init__(self):
        self.items = []
        self.timestamp = int(time.time()) + SKEW

    def Add(self, **kwargs):
        global RULE_ID

        if "rule_id" not in kwargs:
            kwargs["rule_id"] = RULE_ID
            RULE_ID += 1

        self.items.append(Record(**kwargs))
        self.timestamp = int(time.time()) + SKEW

    def Remove(self, rangeParam):
        for i, v in enumerate(self.items):
            if v.range_src == rangeParam or v.range_txt == rangeParam:
                self.items.pop(i)
                self.timestamp = int(time.time()) + SKEW
                return

    def Items(self):
        return self.items


BASE = defaultdict(lambda: Group())


class RequestHandler(BaseHTTPRequestHandler):
    OK = b'Ok'
    ERROR = b'Error'

    def GetCgiParam(self, param, defVal=None):
        val = self.CgiParams.get(param)
        if val is None:
            return defVal

        return val[-1]

    def Send404(self):
        self.send_response(404)
        self.end_headers()

    def SendOK(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html; charset=UTF-8')
        self.end_headers()
        self.wfile.write(self.OK)

    def SendError(self):
        self.send_response(400)
        self.send_header('Content-type', 'text/html; charset=UTF-8')
        self.end_headers()
        self.wfile.write(self.ERROR)

    def SetRange(self):
        global SKEW

        SKEW += 1

        flag = int(self.GetCgiParam('flag'))
        group = BASE[flag]
        operation = self.GetCgiParam('operation')
        if operation == 'add':
            group.Add(**{
                key: self.GetCgiParam(key)
                for key in (
                    "rule_id",
                    "range_src",
                    "range_dst",
                    "range_txt",
                    "range_re",
                    "expire",
                    "description",
                )
                if self.GetCgiParam(key)
            })
        elif operation == 'del':
            rangeParam = self.GetCgiParam('range_src') or self.GetCgiParam('range_txt')
            rangeParam = re.sub('; rule_id=\\d+', '', rangeParam)  # HACK: зачем-то сюда передают невалидный range_txt
            group.Remove(rangeParam)
        else:
            print(f"Bad operation type {operation}", file=sys.stderr)
            self.SendError()
            return

        BASE[flag] = group
        self.SendOK()

    def GetRange(self):
        flag = int(self.GetCgiParam('flag'))

        Format = [x.strip() for x in self.CgiParams.get(
            'with_format', ['range_src,range_dst,range_txt,flag'])[0].split(',')]

        group = BASE[flag]

        out = StringIO()
        status = 200

        for r in group.Items():
            try:
                print(r.Format(Format, flag), file=out)
            except Exception as exc:
                print(str(exc), file=out)
                status = 400
                break

        self.send_response(status)
        self.send_header('Content-type', 'text/html; charset=UTF-8')
        self.send_header('Content-Length', len(out.getvalue()))
        self.end_headers()

        self.wfile.write(out.getvalue().encode())

    def AddIps(self):
        global SKEW
        SKEW += 1

        flag = int(self.GetCgiParam('flag'))
        group = BASE[flag]
        for line in self.Body.split('\n'):
            group.Add(range_src=line.strip(),
                      range_dst=line.strip())

        BASE[flag] = group
        self.SendOK()

    def CheckFlag(self):
        flagStrs = self.GetCgiParam('flag').split(",")
        out = StringIO()

        for flagStr in flagStrs:
            flag = int(flagStr)
            group = BASE[flag]
            print(str(group.timestamp), file=out)

        self.send_response(200)
        self.send_header('Content-type', 'text/html; charset=UTF-8')
        self.send_header('Content-Length', len(out.getvalue()))
        self.end_headers()
        self.wfile.write(out.getvalue().encode())

    def Clear(self):
        global SKEW

        SKEW += 1

        flags = self.GetCgiParam("flags", "")
        if flags:
            for flag in flags.split(","):
                BASE[int(flag)] = Group()
        else:
            BASE.clear()

        self.SendOK()

    HANDLERS = {
        '/cgi-bin/set_range.pl': SetRange,
        '/cgi-bin/get_range.pl': GetRange,
        '/cgi-bin/check_flag.pl': CheckFlag,
        '/cgi-bin/debug/clear.pl': Clear,
    }

    POST_HANDLERS = {
        '/cgi-bin/add_ips.pl': AddIps,
    }

    def do_GET(self):
        self.CgiString = ''
        self.Host = self.headers.get('host')

        reqSplit = self.path.split('?', 1)
        doc = reqSplit[0]
        if len(reqSplit) > 1:
            self.CgiString = reqSplit[1]

        self.CgiParams = urllib.parse.parse_qs(self.CgiString)

        handler = self.HANDLERS.get(doc)
        if not handler:
            self.Send404()
            return

        try:
            handler(self)
        except:
            print(traceback.format_exc(), file=sys.stderr)
            self.SendError()

    def do_POST(self):
        self.CgiString = ''
        self.Host = self.headers.get('host')
        self.ContentLength = int(self.headers.get('Content-Length'))

        reqSplit = self.path.split('?', 1)
        doc = reqSplit[0]
        if len(reqSplit) > 1:
            self.CgiString = reqSplit[1]

        self.CgiParams = urllib.parse.parse_qs(self.CgiString)
        self.Body = self.rfile.read(self.ContentLength).decode("utf-8")

        handler = self.POST_HANDLERS.get(doc)
        if not handler:
            self.Send404()
            return

        try:
            handler(self)
        except:
            print(traceback.format_exc(), file=sys.stderr)
            self.SendError()


class HTTPServer6(HTTPServer):
    address_family = socket.AF_INET6

    def __init__(self, addr, handler):
        HTTPServer.__init__(self, addr, handler)


def Usage():
    print("Usage: %s <port>" % sys.argv[0].split("/")[-1], file=sys.stderr)


def ParseArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument('-4', '--use-ipv4',
                        action='store_true', dest='v4', help='use ipv4')
    parser.add_argument('port', help='port to listen', type=int, nargs='?')

    return parser.parse_args()


def main():
    try:
        opts = ParseArgs()

        serverClass = HTTPServer if opts.v4 else HTTPServer6
        server = serverClass(("", opts.port), RequestHandler)
        server.serve_forever()

    except KeyboardInterrupt:
        print("^C received, shutting down server")
        server.socket.close()
