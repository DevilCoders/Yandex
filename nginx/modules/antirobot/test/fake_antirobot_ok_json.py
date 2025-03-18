#!/usr/bin/env python

from gevent.pywsgi import WSGIServer

def application(environ, start_response):
    print 'got ', len(environ['wsgi.input'].read()), 'bytes'

    status = '200 OK'
    body = 'I am an antirobot, and this is test padding: '+('B'*32768)+('C'*32768)

    headers = [
        ('Content-Type', 'text/javascript'),
        ('X-ForwardToUser-Y', '1')
    ]

    start_response(status, headers)
    return [body]

WSGIServer(('', 8181), application).serve_forever()
