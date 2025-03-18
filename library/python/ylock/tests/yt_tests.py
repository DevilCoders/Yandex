# coding: utf-8
from __future__ import absolute_import

from mock import patch

from yt.local import start, stop

from ylock import create_manager
from . import base


class YTTestCase(base.BaseTestCase):
    """
    Здесь запускаются интеграционные тесты, которые требуют запущенного на локальной машине YT кластера
    в специальном режиме. Для этого необходимо выполнить инструкции по следующей ссылке -
    https://wiki.yandex-team.ru/yt/userdoc/localmode/#ubuntulokalnobezsudo
    """
    backend = 'yt'
    proxy = 'test_proxy'
    token = 'test_token'
    hosts = None
    local_yt_instance = None

    def setUp(self):
        assert self.local_yt_instance is None
        self.local_yt_instance = start(fqdn="localhost", path=self.prefix)
        self.client = self.local_yt_instance.create_client()

        self.manager = create_manager(
            self.backend,
            token=self.token,
            proxy=self.proxy,
            prefix=self.prefix,
        )

    def tearDown(self):
        assert self.local_yt_instance is not None
        stop(self.local_yt_instance.id, remove_working_dir=True, path=self.prefix)
        self.local_yt_instance = None

    def get_client_mock(self):
        return self.local_yt_instance.create_client()

    def test_relock(self):
        with patch('ylock.backends.yt.YTLock._get_client', return_value=self.get_client_mock()):
            super(YTTestCase, self).test_relock()

    def test_lock(self):
        with patch('ylock.backends.yt.YTLock._get_client', return_value=self.get_client_mock()):
            super(YTTestCase, self).test_lock()

    def test_check_acquired(self):
        with patch('ylock.backends.yt.YTLock._get_client', return_value=self.get_client_mock()):
            super(YTTestCase, self).test_check_acquired()

    def test_legacy_context_manager_concurrency(self):
        with patch('ylock.backends.yt.YTLock._get_client', return_value=self.get_client_mock()):
            super(YTTestCase, self).test_check_acquired()

    def test_legacy_context_manager(self):
        with patch('ylock.backends.yt.YTLock._get_client', return_value=self.get_client_mock()):
            super(YTTestCase, self).test_legacy_context_manager()

    def test_decorator(self):
        with patch('ylock.backends.yt.YTLock._get_client', return_value=self.get_client_mock()):
            super(YTTestCase, self).test_decorator()

    def test_init_settings(self):
        manager = create_manager(self.backend, proxy=self.proxy, token=self.token)
        self.assertEqual(manager.proxy, self.proxy)
        self.assertEqual(manager.token, self.token)

    def test_context(self):
        self.skipTest('there is not support in YT backend')
