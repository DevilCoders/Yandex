# -*- coding: utf-8 -*-

"""Тесты обработки HTTP- и медиа-ошибок в blackbox.

Поднимает мок-сервер на локальных портах и заставляет blackbox в него
стучаться, отдавая HTTP-ошибки и плохие Content-Type.
"""

import threading
import time
import unittest

from six.moves import BaseHTTPServer, http_client

import blackbox
from blackbox import BlackboxError


def start_mock_server(port, code=200, content_type='text/xml', body='<foo/>',
                      num_requests=1, error_body=None, num_rises=1):
    """Запускает мок-сервер, отдающий HTTP-ответ с указанными свойствами.

    Сервер будет слушать на 127.0.0.1 на порту `port` и отдавать код состояния
    `code`, тип `content_type` (если не `None`) и тело `body`. Он ответит на
    `num_requests` запросов и завершится.
    """
    class ServerThread(threading.Thread):
        def run(self):
            class MockHandler(BaseHTTPServer.BaseHTTPRequestHandler):
                request_count = 0

                def do(self):
                    self.__class__.request_count = self.request_count + 1
                    self.send_response(code)
                    if content_type is not None:
                        self.send_header('Content-Type', content_type)
                    self.end_headers()
                    if error_body and self.__class__.request_count <= num_rises:
                        self.wfile.write(blackbox.smart_str(error_body))
                    else:
                        self.wfile.write(blackbox.smart_str(body))

                do_GET = do_HEAD = do_POST = do_PUT = do_DELETE = do_OPTIONS \
                    = do_TRACE = do
            svr = BaseHTTPServer.HTTPServer(('127.0.0.1', port), MockHandler)
            for i in range(num_requests):
                svr.handle_request()

    ServerThread().start()
    time.sleep(0.1)     # даём серверу время подняться

    def server_killer():
        # добиваем не до конца отработавшие сервера
        time.sleep(5)
        con = http_client.HTTPConnection('localhost', port)
        for _ in range(num_requests):
            try:
                con.request('GET', '')
                con.getresponse()
            except:
                return
    threading.Thread(target=server_killer).start()


class TestBlackboxHTTPErrors(unittest.TestCase):
    def test_403(self):
        """Сервер отдаёт HTTP 403."""
        start_mock_server(8403, code=403, body='<fail/>', num_requests=2)
        blackbox.BLACKBOX_URL = 'http://localhost:8403/'
        self.assertRaises(BlackboxError, blackbox.userinfo,
                          'test-test', '194.84.46.241', by_login=True)

    def test_500(self):
        """Сервер отдаёт HTTP 500."""
        start_mock_server(8500, code=500, num_requests=2,
                          body='<doc><fail kind="epic"/></doc>')
        blackbox.BLACKBOX_URL = 'http://localhost:8500/'
        self.assertRaises(BlackboxError, blackbox.userinfo,
                          'test-test', '194.84.46.241', by_login=True)

    def test_text_html(self):
        """Сервер отдаёт text/html."""
        start_mock_server(
            8200, content_type='text/html', num_requests=2,
            body='<html>something wrong happened</html>'
        )
        blackbox.BLACKBOX_URL = 'http://localhost:8200/'
        self.assertRaises(BlackboxError, blackbox.userinfo,
                          'test-test', '194.84.46.241', by_login=True)

    def test_retry_based_on_error_fail(self):
        """Сервер отдаёт ошибку которая не должна ретраится."""
        start_mock_server(
            8408, body='''<?xml version="1.0" encoding="utf-8"?>
                    <doc>
                    <uid hosted="0" domid="" domain="">57614307</uid>
                    <karma confirmed="0">0</karma>
                    <dbfield id="subscription.login.2">test-test</dbfield>
                    <dbfield id="subscription.login.4"></dbfield>
                    <dbfield id="subscription.login.8">test.test</dbfield>
                    </doc>
                    ''',
            content_type='text/xml',
            error_body='''<?xml version="1.0" encoding="utf-8"?>
                    <doc>
                    <exception id="2">INVALID_PARAMS</exception>
                    <error>Missing userip argument</error>
                    </doc>
                    ''',
        )
        blackbox.BLACKBOX_URL = 'http://localhost:8408/'
        self.assertRaises(BlackboxError, blackbox.userinfo,
                          'test-test', '194.84.46.241', by_login=True)

    def test_retry_based_on_error_success(self):
        """Сервер отдаёт ошибку которая должна ретраится."""
        start_mock_server(
            8409, body='''<?xml version="1.0" encoding="utf-8"?>
                    <doc>
                    <uid hosted="0" domid="" domain="">57614307</uid>
                    <karma confirmed="0">0</karma>
                    <dbfield id="subscription.login.2">test-test</dbfield>
                    <dbfield id="subscription.login.4"></dbfield>
                    <dbfield id="subscription.login.8">test.test</dbfield>
                    </doc>
                    ''',
            content_type='text/xml',
            error_body='''<?xml version="1.0" encoding="utf-8"?>
                    <doc>
                    <exception id="10">DB_EXCEPTION</exception>
                    </doc>
                    ''',
            num_requests=3,
        )
        blackbox.BLACKBOX_URL = 'http://localhost:8409/'
        info = blackbox.userinfo('test-test', '194.84.46.241', by_login=True)
        self.assertEqual(info['uid'], '57614307')

    def test_retry_based_on_error_fail_after_retry(self):
        """Сервер отдаёт ошибку которая не должна ретраится."""
        start_mock_server(
            8408, body='''<?xml version="1.0" encoding="utf-8"?>
                    <doc>
                    <uid hosted="0" domid="" domain="">57614307</uid>
                    <karma confirmed="0">0</karma>
                    <dbfield id="subscription.login.2">test-test</dbfield>
                    <dbfield id="subscription.login.4"></dbfield>
                    <dbfield id="subscription.login.8">test.test</dbfield>
                    </doc>
                    ''',
            content_type='text/xml',
            error_body='''<?xml version="1.0" encoding="utf-8"?>
                    <doc>
                    <exception id="10">DB_EXCEPTION</exception>
                    </doc>
                    ''',
            num_rises=4
        )
        blackbox.BLACKBOX_URL = 'http://localhost:8408/'
        self.assertRaises(BlackboxError, blackbox.userinfo,
                          'test-test', '194.84.46.241', by_login=True)

    def test_no_error(self):
        """Сервер отдаёт нормальный ответ."""
        blackbox.BLACKBOX_URL = 'http://localhost:8201/'
        start_mock_server(8201, body='''<?xml version="1.0" encoding="utf-8"?>
            <doc>
            <uid hosted="0" domid="" domain="">57614307</uid>
            <karma confirmed="0">0</karma>
            <dbfield id="subscription.login.2">test-test</dbfield>
            <dbfield id="subscription.login.4"></dbfield>
            <dbfield id="subscription.login.8">test.test</dbfield>
            </doc>
            '''
        )
        info = blackbox.userinfo('test-test', '194.84.46.241', by_login=True)
        self.assertEqual(info['uid'], '57614307')

    def test_no_content_type(self):
        """Сервер отдаёт ответ без Content-Type."""
        blackbox.BLACKBOX_URL = 'http://localhost:8202/'
        start_mock_server(8202, body='''<?xml version="1.0" encoding="utf-8"?>
            <doc>
            <uid hosted="0" domid="" domain="">57614307</uid>
            <karma confirmed="0">0</karma>
            <dbfield id="subscription.login.2">test-test</dbfield>
            <dbfield id="subscription.login.4"></dbfield>
            <dbfield id="subscription.login.8">test.test</dbfield>
            </doc>
            ''',
            content_type=None
        )
        info = blackbox.userinfo('test-test', '194.84.46.241', by_login=True)
        self.assertEqual(info['uid'], '57614307')


if __name__ == '__main__':
    unittest.main()
