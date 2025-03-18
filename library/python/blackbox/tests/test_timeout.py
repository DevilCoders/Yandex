# -*- coding: utf-8 -*-

"""Проверка обработки таймаута на TCP-соединение.

В этом наборе тестов принимается, что боевой чёрный ящик *не* доступен с
тестовой машины, а именно, все соединения с ним таймаутятся.
"""

import socket
import time
import threading
import unittest
import six

from six.moves import BaseHTTPServer, http_client

import blackbox


# Делается две попытки доступа, поэтому реальный таймаут таков:
TIMEOUT = 2 * blackbox.HTTP_TIMEOUT + 0.5
USER_IP = '127.0.0.1'


def start_server_with_retries(port, num_retries, timeout=None):
    """Запускает мок-сервер, первые num_retries-1 возвращающий ошибку
    (если timeout == None) или возвращающий ответ с таймаутом.
    В последний раз ответ отдаётся без таймаута и без ошибки.

    Сервер будет слушать на 127.0.0.1 на порту `port`. Он ответит на
    `num_requests` запросов и завершится.
    """
    class ServerThread(threading.Thread):
        def run(self):
            class MockHandler(BaseHTTPServer.BaseHTTPRequestHandler):
                request_count = 0

                def do(self):
                    self.__class__.request_count = self.request_count + 1
                    if self.__class__.request_count == num_retries:
                        self.send_response(200)
                        self.send_header(b'Content-Type', b'text/xml')
                        self.end_headers()
                        self.wfile.write(b'''<?xml version="1.0" encoding="utf-8"?>
                                            <doc>
                                            <uid hosted="0" domid="" domain="">57614307</uid>
                                            <karma confirmed="0">0</karma>
                                            <dbfield id="subscription.login.2">test-test</dbfield>
                                            <dbfield id="subscription.login.4"></dbfield>
                                            <dbfield id="subscription.login.8">test.test</dbfield>
                                            </doc>
                                            ''')
                        return
                    if not timeout:
                        self.send_response(500)
                    else:
                        time.sleep(timeout)

                do_GET = do_HEAD = do_POST = do_PUT = do_DELETE = do_OPTIONS \
                    = do_TRACE = do
            svr = BaseHTTPServer.HTTPServer(('127.0.0.1', port), MockHandler)
            for i in range(num_retries):
                svr.handle_request()

    ServerThread().start()
    time.sleep(0.1)  # даём серверу время подняться
    def server_killer():
        # добиваем не до конца отработавшие сервера
        time.sleep(5)
        con = http_client.HTTPConnection('localhost', port)
        for _ in range(num_retries):
            try:
                con.request('GET', '')
                con.getresponse()
            except:
                return
    threading.Thread(target=server_killer).start()


class TestBlackboxTimeOut(unittest.TestCase):
    def test_connect_timeout(self):
        """Таймаут при установке TCP-соединения."""
        # Надеюсь никто не слушает этот порт :)
        blackbox.BLACKBOX_URL = 'http://127.0.0.1:12321/blackbox/'
        time1 = time.time()
        try:
            blackbox.userinfo('test', USER_IP)
        except blackbox.BlackboxError:

            import traceback
            print(traceback.print_exc())
            time2 = time.time()
            self.assertAlmostEqual(time2 - time1, blackbox.HTTP_TIMEOUT + 0.5, 0)
        except Exception as ex:
            self.fail('ожидали BlackboxError, получили %s' % ex)

    def test_read_timeout(self):
        """Таймаут при чтении HTTP-ответа."""
        sock = socket.socket()
        port = 8765
        if six.PY3:
            port = 8766
        sock.bind(('127.0.0.1', port))
        sock.listen(1)
        blackbox.BLACKBOX_URL = 'http://127.0.0.1:{}/blackbox/'.format(port)
        time1 = time.time()
        try:
            blackbox.userinfo('test', USER_IP)
        except blackbox.BlackboxError:
            time2 = time.time()
            self.assertAlmostEqual(time2 - time1, TIMEOUT, 0)
        except Exception as ex:
            self.fail('ожидали BlackboxError, получили %s' % ex)
        finally:
            sock.close()


class TestRetries(unittest.TestCase):
    def test_successfull_retry_error(self):
        """Успешный повторный запрос, когда несколько первых запросов
        возвращают ошибки"""
        start_server_with_retries(8403, num_retries=3)
        b = blackbox.Blackbox(url='http://localhost:8403/', timeout=[0.5, 0.5, 0.5])
        info = b.userinfo('test-test', USER_IP, by_login=True)
        self.assertEqual(info['uid'], '57614307')

    def test_unsuccessfull_retry_error(self):
        """Безуспешный повторный запрос, когда сервер возвращает ошибки"""
        start_server_with_retries(8404, num_retries=3)
        b = blackbox.Blackbox(url='http://localhost:8404/', timeout=[0.5, 0.5])
        self.assertRaises(blackbox.BlackboxError, b.userinfo, 'test-test',
                          USER_IP, by_login=True)
        time.sleep(1)

    def test_successfull_retry_timeout(self):
        """Успешный повторный запрос, когда сервер отвечает слишком медленно"""
        start_server_with_retries(8405, num_retries=3, timeout=1)
        b = blackbox.Blackbox(url='http://localhost:8405/', timeout=[0.5, 1, 1.5])
        info = b.userinfo('test-test', USER_IP, by_login=True)
        self.assertEqual(info['uid'], '57614307')

        start_server_with_retries(8406, num_retries=3, timeout=1)
        b = blackbox.Blackbox(url='http://localhost:8406/', timeout=1, retry_count=3)
        info = b.userinfo('test-test', USER_IP, by_login=True)
        self.assertEqual(info['uid'], '57614307')

    def test_unsuccessfull_retry_timeout(self):
        """Безуспешный повторный запрос, когда сервер отвечает слишком медленно"""
        start_server_with_retries(8407, num_retries=4, timeout=1)
        b = blackbox.Blackbox(url='http://localhost:8407/', timeout=0.5, retry_count=3)
        self.assertRaises(blackbox.BlackboxError, b.userinfo, 'test-test',
                          USER_IP, by_login=True)
        time.sleep(2)

        start_server_with_retries(8408, num_retries=3, timeout=1)
        b = blackbox.Blackbox(url='http://localhost:8408/', timeout=[0.5, 0.6])
        self.assertRaises(blackbox.BlackboxError, b.userinfo, 'test-test',
                          USER_IP, by_login=True)
        time.sleep(2)


if __name__ == '__main__':
    unittest.main()
