#!/usr/bin/env python

from gevent.pywsgi import WSGIServer

def application(environ, start_response):
    print environ
    status = '200 OK'
    body = 'I am real, and this is test padding: ' + ('A'*65535)

    headers = [
        ('Content-Type', 'text/html')
    ]

    start_response(status, headers)
    return [body]

WSGIServer(('', 8082), application).serve_forever()
