# -*- coding: utf-8 -*-
import sys
from datetime import datetime
from time import time

try:
    from unittest.mock import MagicMock

except ImportError:  # py 2
    from mock import MagicMock

import pytest


@pytest.fixture
def stub_logbroker_client(monkeypatch):
    """Подкладывает заглушку вместо реальных библиотек Logbroker"""

    class ModuleStub:

        def __getattr__(self, name):
            # type: (str) -> None
            return MagicMock()

    def patch_modules(paths):
        # жалкое подобие pytest-stub
        modules = sys.modules
        for path in paths:
            split = path.split('.')
            while split:
                modules['.'.join(split)] = ModuleStub()
                split.pop()

    # peerdir из тестов на sdk kikimr не работает. симулируем наличие зависимости,
    # чтобы далее её подменить.
    patch_modules([
        'kikimr.public.sdk.python.persqueue.auth',
        'kikimr.public.sdk.python.persqueue.grpc_pq_streaming_api',
        'kikimr.public.sdk.python.persqueue.errors',
    ])

    prefix = 'errorboosterclient.logbroker.'

    auth = MagicMock()
    pqlib = MagicMock()

    errors = MagicMock()
    errors.SessionFailureResult = ValueError

    monkeypatch.setattr('{prefix}auth'.format(prefix=prefix), auth)
    monkeypatch.setattr('{prefix}pqlib'.format(prefix=prefix), pqlib)
    monkeypatch.setattr('{prefix}errors'.format(prefix=prefix), errors)


def test_integration(stub_logbroker_client):

    from errorboosterclient.logbroker import LogbrokerClient

    source = 'test_source'
    topic = 'mdh/test/trash'

    with LogbrokerClient() as logbroker:
        with logbroker.get_producer(source=source, topic=topic) as producer:
            assert producer.seq_num == 0
            producer.write({
                'project': 'mdh',
                'message': 'signaling test',
                'stack': 'some\n{dt}'.format(dt=datetime.now()),
                'level': 'debug',
                'language': 'python',
                'timestamp': round(time() * 1000),
            })
            # Номер должен обновиться.
            assert producer.seq_num != 0

            # Проверим обнуление связи.
            producer.close()
            assert producer._me is None
