#!/usr/bin/env python

from gevent.pywsgi import WSGIServer

def application(environ, start_response):
    print 'got ', len(environ['wsgi.input'].read()), 'bytes'

    status = '403 Forbidden'
    body = 'I am an antirobot, and this is test padding: '+('B'*1024)

    headers = [
        ('Content-Type', 'text/html'),
        ('X-ForwardToUser-Y', '1')
    ]

    start_response(status, headers)
    return [body]

WSGIServer(('', 8181), application).serve_forever()
