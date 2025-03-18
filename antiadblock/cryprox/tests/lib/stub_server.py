# -*- coding: utf8 -*-
import logging
import os
import threading
from wsgiref.simple_server import make_server

import requests
import pytest
import mimetypes

from yatest.common import network, source_path
from werkzeug.wrappers import BaseResponse

STUB_PATH = "antiadblock/cryprox/tests/functional/stubs"

mimetypes.init([])  # запрещаем модулю mimetypes читать системные файлы для обновления словаря типов, иначе этот словарь становится НЕ кроссплатформенным


def default_handler(**request):
    """
    :param request: {'path': '/', 'headers': {}, 'cookies': {}, 'query': ''}
    :return: {'text': '', 'code': 1, 'headers': '{}', 'redirect': 'url we want be redirected to'}
    Defines server default behaviour: returns content of a file at a requested location.
    """
    response = dict()
    stubs_path = source_path(STUB_PATH)
    resource_path = os.path.join(stubs_path, "content", request.get('path', '/')[1:])
    index_path = os.path.join(resource_path, 'index.html')
    try:
        if os.path.isfile(resource_path):
            with open(resource_path, 'r') as f:
                response['text'] = f.read()
                response['code'] = 200
                response['headers'] = {'Content-Type': mimetypes.guess_type(request['path'])[0],
                                       'Content-Length': len(response['text'])}

        elif os.path.isfile(index_path):
            with open(index_path, 'r') as f:
                response['text'] = f.read()
                response['code'] = 200
                response['headers'] = {'Content-Type': 'text/html', 'Content-Length': len(response['text'])}
        else:
            logging.warning("no file in stubs {}".format(resource_path))
            response['text'] = 'Error. Not found.'
            response['code'] = 404
    except IOError:
        response['text'] = 'Error. File was found, but something gone wrong.'
        response['code'] = 500
    return response


class WSGIServer(threading.Thread):

    """
    HTTP server running a WSGI application in its own thread.
    """

    def __init__(self, host='127.0.0.1', port=0, application=None, **kwargs):
        self.app = application
        self._server = make_server(host, port, self.app, **kwargs)
        self.server_address = self._server.server_address

        super(WSGIServer, self).__init__(
            name=self.__class__,
            target=self._server.serve_forever)

    def operate(self):
        try:
            self._server.serve_forever()
        except Exception:
            logging.exception("Stub server is shutting down")
            raise

    def __del__(self):
        self.stop()

    def stop(self):
        self._server.server_close()
        self._server.shutdown()

    @property
    def url(self):
        host, port = self.server_address
        proto = 'http'
        return '%s://%s:%i' % (proto, host, port)


class StubServer(WSGIServer):
    def __init__(self, host="", port=0, **kwargs):
        self.requests_log = []
        super(StubServer, self).__init__(host=host, port=port, application=self.call, **kwargs)
        self.handler = None

    def call(self, environ, start_response):
        headers = {key: val for key, val in environ.items() if 'HTTP_' in key}
        cookie_raw = headers.pop('HTTP_COOKIE', '')
        if cookie_raw:
            cookies = dict((k, v) for k, v in [pair.strip().split('=', 1) for pair in cookie_raw.split(';')])
        else:
            cookies = dict()
        path = environ['PATH_INFO']
        query = environ['QUERY_STRING']
        body = ''
        try:
            length = int(environ.get('CONTENT_LENGTH', '0'))
        except ValueError:
            length = 0
        if length != 0:
            body = environ['wsgi.input'].read(length)
        request = {'headers': headers, 'cookies': cookies, 'path': path, 'query': query, 'body': body}
        self.requests_log.append(request)
        response = dict() if self.handler is None else self.handler(**request)
        if isinstance(response, BaseResponse):
            return response(environ, start_response)
        if 'text' not in response or 'code' not in response:
            add_headers = response.get('headers', {})
            response.update(default_handler(**request))
            if 'headers' in response:
                response['headers'].update(add_headers)
        if 'redirect' in response:
            redirected_response = requests.get(response['redirect'])
            start_response(str(redirected_response.status_code) + " ", [])
            return redirected_response.text.encode('ascii', 'ignore')
        else:
            start_response(str(response.get('code', '404')) + " ", [(k, str(v)) for k, v in (response.get('headers', {}).items())])
            return response.get('text', '')

    def set_handler(self, handler):
        self.handler = handler


@pytest.yield_fixture(scope='session')
def stub_server_and_port():
    with network.PortManager() as pm:
        stub_port = pm.get_port()

        server = StubServer(port=stub_port)
        server.start()
        try:
            yield server, stub_port
        finally:
            server.stop()


@pytest.yield_fixture
def auto_restore_stub(restore_stub):

    yield

    restore_stub()


@pytest.fixture
def restore_stub(stub_server_and_port):

    def action():
        stub_server, _ = stub_server_and_port

        stub_server.set_handler(default_handler)

    return action


@pytest.fixture(scope='session')
def _stub_server(stub_server_and_port):
    stub_server, port = stub_server_and_port
    return stub_server


@pytest.fixture
def stub_server(_stub_server, auto_restore_stub):  # noqa
    return _stub_server


@pytest.fixture(scope='session')
def stub_port(stub_server_and_port):
    _, port = stub_server_and_port
    return port


@pytest.fixture(scope='session')
def content_path():
    stubs_path = source_path(STUB_PATH)
    return os.path.join(stubs_path, "content")


@pytest.fixture
def httpserver(request):
    server = ContentServer()
    server.start()
    request.addfinalizer(server.stop)
    return server


class ContentServer(WSGIServer):

    """
        server = ContentServer(port=8080)
        server.start()
        server.content = 'Hello World!'
        server.code = 503

        # we're done
        server.stop()

    """

    def __init__(self, host='127.0.0.1', port=0):
        super(ContentServer, self).__init__(host, port, self)
        self.content, self.code = ('', '204')  # HTTP 204: No Content
        self.headers = {}
        self.requests = []

    def __call__(self, environ, start_response):
        start_response(self.code, self.headers.items())
        return [self.content]

    def serve_content(self, content, code='200', headers=None):
        """
        Serves string content (with specified HTTP error code) as response to
        all subsequent request.

        :param content: content to be displayed
        :param code: HTTP status code
        :param headers: HTTP headers to be returned
        """
        self.content, self.code = (content, code if len(code) >= 4 else code + ' ')
        if headers:
            self.headers = headers
