# -*- coding: utf-8 -*-
import logging
import sys
from contextlib import contextmanager
from time import sleep

import pytest
import sentry_sdk
from errorboosterclient.sentry import EventConverter, ErrorBoosterTransport, ExceptionCapturer
from sentry_sdk.integrations.logging import LoggingIntegration

PY3 = sys.version_info >= (3, 0)

EVENT_BASIC = {
    'timestamp_': 12345,
    'event_id': 102030,
    'level': 'info',
    'server_name': 'myserver',
    'contexts': {'runtime': {'name': 'CPython', 'version': '3.6.9'}},
    'modules': {'yenv': '0.9', 'requests': '2.21.0'},
    'extra': {'sys.argv': ['some.py', 'do']},
    'breadcrumbs': [
        {'category': 'my', 'message': 'start', 'level': 'info',
         'timestamp': '2020-07-23T02:42:39.303066Z', 'type': 'default'},
        {
            'category': 'hidethis', 'level': 'error',
            'message': 'gpg --batch --passphrase donottell -d',
            'data': {
                'url': 'https://some.com/blackbox/?dbfields=accounts.login.uid&sessionid=donottell&',
                'status_code': 200
            },
            'timestamp': '2020-07-23T02:42:39.303066Z', 'type': 'default'},
    ],
}

EVENT_CAPTURE_MESSAGE = EVENT_BASIC.copy()
EVENT_CAPTURE_MESSAGE['message'] = 'some message'


@pytest.fixture
@contextmanager
def send_error_booster_mock():
    sent = []

    def send_me(event):
        # type: (dict) -> None
        sent.append(event)

    transport = ErrorBoosterTransport(
        project='myproject',
        sender=send_me,
        bgworker=False,
    )

    sentry_sdk.init(
        transport=transport,
        debug=True,
        integrations=[LoggingIntegration(
            level=None,
            event_level=logging.ERROR
        )]
    )
    yield sent


def test_converter_wo_trace():
    # Событие без трассировки.
    result = EventConverter.convert(EVENT_CAPTURE_MESSAGE)

    assert result == {
        'additional': {
            'breadcrumbs': [
                {'category': 'my', 'level': 'info', 'message': 'start',
                 'timestamp': '2020-07-23T02:42:39.303066Z', 'type': 'default'},
                {
                    'category': 'hidethis', 'level': 'error',
                    'message': '*****',
                    'data': {'url': '*****', 'status_code': 200},
                    'timestamp': '2020-07-23T02:42:39.303066Z', 'type': 'default'
                },
            ],
            'contexts': {'runtime': {'name': 'CPython', 'version': '3.6.9'}},
            'eventid': 102030,
            'extra': {'sys.argv': ['some.py', 'do']},
            'modules': {'requests': '2.21.0', 'yenv': '0.9'},
            'vars': []
        },
        'env': '',
        'host': 'myserver',
        'language': 'python',
        'level': 'info',
        'message': 'some message',
        'timestamp': 12345,
        'version': ''
    }


def test_converter_wo_trace_w_extra():
    # Событие без трассировки c дополнительной информацией.
    event = EVENT_CAPTURE_MESSAGE.copy()
    event.update({'user': {'ip_address': '127.0.0.1', 'yandexuid': '1234567'},
                  'tags': {'source': 'backend1', 'isInternal': True}})
    result = EventConverter.convert(event)

    del result['additional']
    assert result == {
        'env': '',
        'host': 'myserver',
        'ip': '127.0.0.1',
        'isInternal': True,
        'language': 'python',
        'level': 'info',
        'message': 'some message',
        'source': 'backend1',
        'timestamp': 12345,
        'version': '',
        'yandexuid': '1234567'
    }


def test_converter_w_trace():
    # Событие с трассировкой.
    event_capture_exception = EVENT_BASIC.copy()
    event_capture_exception.update({
        'exception': {
            'values': [
                {
                    'module': 'json.decoder',
                    'type': 'JSONDecodeError',
                    'value': 'Expecting value: line 1 column 1 (char 0)', 'mechanism': None,
                    'stacktrace': {
                        'frames': [
                            {
                                'filename': 'test_logbroker.py',
                                'abs_path': '/home/idlesign/dev/ydh/src/tests/core/integration/test_logbroker.py',
                                'function': 'test_some',
                                'module': 'test_logbroker',
                                'lineno': 68,
                                'pre_context': [],
                                'context_line': '            b = json.loads(a)',
                                'post_context': [],
                                'vars': {'a': "'asdasdasd'"},
                                'in_app': True
                            },
                            {
                                'filename': '__init__.py',
                                'abs_path': '/usr/lib/python3.6/json/__init__.py',
                                'function': 'loads',
                                'module': 'json',
                                'lineno': 354,
                                'pre_context': [],
                                'context_line': '        return _default_decoder.decode(s)',
                                'post_context': [],
                                'vars': {'kw': {}, 's': "'asdasdasd'"},
                                'in_app': True
                            },
                            {
                                'filename': 'json/decoder.py',
                                'abs_path': '/usr/lib/python3.6/json/decoder.py',
                                'function': 'decode', 'module': 'json.decoder',
                                'lineno': 339,
                                'pre_context': [],
                                'context_line': '        obj, end = self.raw_decode(s, idx=_w(s, 0).end())',
                                'post_context': [],
                                'vars': {'s': "'asdasdasd'"},
                                'in_app': True
                            },
                            {
                                'filename': 'json/decoder.py',
                                'abs_path': '/usr/lib/python3.6/json/decoder.py',
                                'function': 'raw_decode',
                                'module': 'json.decoder',
                                'lineno': 357,
                                'pre_context': [],
                                'context_line': (
                                    '            raise JSONDecodeError("Expecting value", s, err.value) from None'),
                                'post_context': [],
                                'vars': {'idx': '0', 's': "'asdasdasd'"},
                                'in_app': True
                            }
                        ]
                    }  # stacktrace
                }  # value
            ]  # values
        }  # exception
    })

    result = EventConverter.convert(event_capture_exception)

    # Проверяем проброс локальных переменных фреймов.
    assert result['additional']['vars'] == [
        {'loc': '/home/idlesign/dev/ydh/src/tests/core/integration/test_logbroker.py:68 test_some()',
         'vars': {'a': "'asdasdasd'"}},
        {'loc': '/usr/lib/python3.6/json/__init__.py:354 loads()', 'vars': {'kw': {}, 's': "'asdasdasd'"}},
        {'loc': '/usr/lib/python3.6/json/decoder.py:339 decode()', 'vars': {'s': "'asdasdasd'"}},
        {'loc': '/usr/lib/python3.6/json/decoder.py:357 raw_decode()', 'vars': {'idx': '0', 's': "'asdasdasd'"}}
    ]

    del result['additional']

    assert result == {
        'env': '',
        'file': '/usr/lib/python3.6/json/decoder.py',
        'host': 'myserver',
        'language': 'python',
        'level': 'info',
        'message': 'json.decoder.JSONDecodeError: Expecting value: line 1 column 1 '
                   '(char 0)',
        'method': 'raw_decode',
        'stack': (
            'Traceback (most recent call last):\n'
            '  File "/home/idlesign/dev/ydh/src/tests/core/integration/test_logbroker.py", line 68, in test_some\n'
            '    b = json.loads(a)\n'
            '  File "/usr/lib/python3.6/json/__init__.py", line 354, in loads\n'
            '    return _default_decoder.decode(s)\n'
            '  File "/usr/lib/python3.6/json/decoder.py", line 339, in decode\n'
            '    obj, end = self.raw_decode(s, idx=_w(s, 0).end())\n'
            '  File "/usr/lib/python3.6/json/decoder.py", line 357, in raw_decode\n'
            '    raise JSONDecodeError("Expecting value", s, err.value) from None\n'
            'json.decoder.JSONDecodeError: Expecting value: line 1 column 1 (char 0)'
        ),
        'timestamp': 12345,
        'version': ''
    }


def test_transport():
    sent = []

    locvar = 1  # noqa
    locvar_secret = 'not-a-secret'  # noqa
    locvar_nested = {  # noqa
        'a': None,
        'my_secret': 'not-a-secret-too',
    }
    locvar_list = ['bb', {'password': 'meh'}]  # noqa
    locvar_nested_sani = {'a': {'b': 'password=123456&me'}}  # noqa

    def send_me(event):
        # type: (dict) -> None
        sent.append(event)

    transport = ErrorBoosterTransport(
        project='myproject',
        sender=send_me,
        bgworker=False,
    )
    sentry_sdk.init(
        transport=transport,
        environment='development',
        release='0.0.1',
        send_default_pii=True,
    )

    sentry_sdk.add_breadcrumb(
        category='tests', message='other', level='info',
        type='erbclient', data={'a': 'b'}
    )

    with sentry_sdk.configure_scope() as scope:
        scope.user = {'ip_address': '127.0.0.1', 'yandexuid': '1234567'}
        scope.set_tag('source', 'backend1')

        try:
            raise Exception('test it')
        except:
            sentry_sdk.capture_exception()

    assert len(sent) == 1
    sent_1 = sent[0]
    if PY3:
        assert sent_1['message'] == 'Exception: test it'
    else:
        assert sent_1['message'] == 'exceptions.Exception: test it'

    additional = sent_1['additional']
    assert additional['breadcrumbs'][0]['data'] == {'a': 'b'}

    # Проверка запечатления локальных переменных и их санации.
    locvars = additional['vars'][0]['vars']
    assert locvars['locvar'] == '1'
    assert locvars['locvar_secret'] == '*****'
    assert locvars['locvar_nested'] == {'a': 'None', 'my_secret': '*****'}
    assert locvars['locvar_list'] == ["'bb'", {'password': '*****'}]
    assert locvars['locvar_nested_sani'] == {'a': {'b': "'password=123456&me'"}}

    assert sent_1['project'] == 'myproject'
    assert sent_1['ip'] == '127.0.0.1'
    assert sent_1['yandexuid'] == '1234567'
    assert sent_1['source'] == 'backend1'
    assert sent_1['env'] == 'development'
    assert sent_1['version'] == '0.0.1'
    assert isinstance(sent_1['timestamp'], int)

    # проверка санации значений переменных
    transport.sanitize_values = True
    del sent[:]  # то же что и .clear()
    try:
        raise Exception('sweeet')
    except:
        sentry_sdk.capture_exception()

    locvars = sent[0]['additional']['vars'][0]['vars']
    assert locvars['locvar_nested_sani'] == {'a': {'b': '*****'}}

    # Далее проверим работу фоновой нити отправки.

    def send_me(event):
        # type (dict) -> None
        # Слишком долго выполняется эта функция. Фоновая нить её ждать не будет.
        # См. shutdown_timeout
        sleep(3)
        sent.append(event)

    transport = ErrorBoosterTransport(
        project='myproject',
        sender=send_me,
        bgworker=True,
    )

    sentry_sdk.init(
        transport=transport,
        shutdown_timeout=1,
    )

    try:
        raise Exception('test that')
    except:
        sentry_sdk.capture_exception()

    assert len(sent) == 1
    transport.flush(timeout=1)
    transport.kill()


def test_capturer(monkeypatch):
    monkeypatch.setattr('errorboosterclient.sentry.capture_exception', lambda: '123')
    out = ExceptionCapturer.raise_capture(ValueError('some'))
    assert out.uid == '123'
    assert not out.msg
    assert not out.trace

    monkeypatch.setattr('errorboosterclient.sentry.capture_exception', lambda: None)

    out = ExceptionCapturer.raise_capture(ValueError('some'))
    assert not out.uid
    assert not out.trace
    assert out.msg == 'ValueError: some'

    class MyCapturer(ExceptionCapturer):
        capture_expose_trace = True
        raise_capture_exc_cls = IndexError

    out = MyCapturer.raise_capture('generate')
    assert not out.uid
    assert out.trace
    assert out.msg == 'IndexError: generate'


def test_message_from_logging_handler(send_error_booster_mock):
    with send_error_booster_mock as sending_queue:

        logging.error('error is logged with %s', 'params')

        assert len(sending_queue) == 1
        assert sending_queue[0]['message'] == 'error is logged with %s'
        assert sending_queue[0]['additional']['message'] == 'error is logged with params'

        try:
            raise ValueError("Illegal character %r in salt" % '1')
        except ValueError:
            logging.exception('exc is logged with %s', 'params')

        assert len(sending_queue) == 2
        assert sending_queue[1]['message'] == 'exc is logged with %s'
        assert sending_queue[1]['additional']['message'] == 'exc is logged with params'

        logging.error('error is logged with %(named)s', {'named': 'params'})

        assert len(sending_queue) == 3
        assert sending_queue[2]['message'] == 'error is logged with %(named)s'
        assert sending_queue[2]['additional']['message'] == 'error is logged with params'


def test_message_from_logging_handler_unicode(send_error_booster_mock):
    with send_error_booster_mock as sending_queue:

        logging.error(u'error is logged ыы with %s', u'цц')

        assert len(sending_queue) == 1
        assert sending_queue[0]['message'] == u'error is logged ыы with %s'
        assert sending_queue[0]['additional']['message'] == u'error is logged ыы with цц'

        try:
            raise ValueError(u"Illegal character %r in saщщt" % u'ьь')
        except ValueError:
            logging.exception(u'exc is logged wiззth %s', u'pггarams')

        assert len(sending_queue) == 2
        assert sending_queue[1]['message'] == u'exc is logged wiззth %s'
        assert sending_queue[1]['additional']['message'] == u'exc is logged wiззth pггarams'


def test_message_with_bad_char(send_error_booster_mock):
    with send_error_booster_mock as sending_queue:
        try:
            raise ValueError('Some error with \xe2')
        except ValueError:
            logging.exception('exc is logged wiззth %s', 'pггarams')

        assert len(sending_queue) == 1


def test_message_with_bad_char_in_breadcrumb(send_error_booster_mock):
    with send_error_booster_mock as sending_queue:
        logging.info('Some info with \xe2')
        logging.error('Some error')

        assert len(sending_queue) == 1


def test_message_with_percent(send_error_booster_mock):
    with send_error_booster_mock as sending_queue:
        logging.error('Some error %')

        assert len(sending_queue) == 1
