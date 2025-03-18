# -*- coding: utf-8 -*-

from __future__ import print_function, absolute_import, division

import mimetypes
import sys

import tornado.web

import library.python.resource as lpy

import luigi.cmdline
import luigi.server

from luigi.server import (
    RPCHandler,
    RootPathHandler,
    AllRunHandler,
    SelectedRunHandler,
    RecentRunHandler,
    ByNameHandler,
    ByIdHandler,
    ByParamsHandler
)


class StaticHandler(tornado.web.RequestHandler):
    DATA = {
        path: {
            'content': content,
            'mimetype': mimetypes.guess_type(path)[0] or 'application/octet-stream',
        }
        for path, content in lpy.iteritems(prefix='/luigi/static/', strip_prefix=True)
    }

    def get(self, path):
        data = self.DATA.get(path)
        if data is None:
            self.send_error(404)
            return

        self.set_header('Content-Type', data['mimetype'])
        self.finish(data['content'])


def static_app(scheduler):
    settings = {
        "unescape": tornado.escape.xhtml_unescape,
        "compress_response": True,
    }
    handlers = [
        (r'/static/(.*)', StaticHandler),
        (r'/api/(.*)', RPCHandler, {"scheduler": scheduler}),
        (r'/', RootPathHandler, {'scheduler': scheduler}),
        (r'/tasklist', AllRunHandler, {'scheduler': scheduler}),
        (r'/tasklist/(.*?)', SelectedRunHandler, {'scheduler': scheduler}),
        (r'/history', RecentRunHandler, {'scheduler': scheduler}),
        (r'/history/by_name/(.*?)', ByNameHandler, {'scheduler': scheduler}),
        (r'/history/by_id/(.*?)', ByIdHandler, {'scheduler': scheduler}),
        (r'/history/by_params/(.*?)', ByParamsHandler, {'scheduler': scheduler})
    ]
    api_app = tornado.web.Application(handlers, **settings)
    return api_app


def luigid_static(argv=sys.argv[1:]):
    # monkeypatch it to use our handlers
    luigi.server.app = static_app
    return luigi.cmdline.luigid(argv)
