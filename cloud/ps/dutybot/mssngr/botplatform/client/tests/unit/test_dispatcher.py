import pytest
from hamcrest import (assert_that, equal_to)

from mssngr.botplatform.client.src.dispatcher import Dispatcher
from mssngr.botplatform.client.src.types import Update


def test_handlers_registration_1():
    d = Dispatcher()

    def check_func(m):
        return True

    @d.message_handler(content_type=['document', 'text'])
    def foo(message):
        pass

    @d.message_handler(func=check_func, content_type=['text'])
    def bar(message):
        pass

    assert_that(
        d._handlers,
        equal_to([
            (foo, {'content_type': ['document', 'text']}),
            (bar, {'content_type': ['text'], 'func': check_func}),
        ])
    )


def test_handlers_registration_wrong_check():
    d = Dispatcher()

    with pytest.raises(ValueError, match='Unexpected argument: strange_check'):
        @d.message_handler(content_type=['document', 'text'], strange_check=None)
        def foo1(message):
            pass

    with pytest.raises(ValueError, match=r'content_type element should be one of.*'):
        @d.message_handler(content_type=['document', 'text', 'what'])
        def foo2(message):
            pass

    with pytest.raises(ValueError, match='func should be callable'):
        @d.message_handler(func=123)
        def foo3(message):
            pass

    with pytest.raises(ValueError, match='regexp should be string'):
        @d.message_handler(regexp=123)
        def foo4(message):
            pass


def test_handlers_matching(monkeypatch):
    resp = [
        {
            "message": {
                "from": {
                    "is_bot": False,
                    "login": "nikola",
                    "display_name": "Николай",
                    "id": "5bd62",
                    "uid": 123
                },
                "chat": {
                    "username": "robot-pers-userdata",
                    "type": "private",
                    "id": "5bd62b7b_770c917b"
                },
                "date": 1593780447,
                "text": "/start",
                "message_id": 1593780447215008
            },
            "update_id": 46088
        },
        {
            "message": {
                "custom_payload": {
                    "test_ids": []
                },
                "seq_no": 863,
                "from": {
                    "is_bot": False,
                    "login": "nikola",
                    "display_name": "Николай",
                    "id": "5bd62",
                    "uid": 123
                },
                "chat": {
                    "username": "robot-pers-userdata",
                    "type": "private",
                    "id": "5bd62b7b_770c917b"
                },
                "date": 1593780458,
                "photo": [
                    {
                        "width": 959,
                        "size": 172963,
                        "file_id": "file/6ea1df65-b30b-4249-af2d-f669df8f392a",
                        "height": 1280
                    }
                ],
                "message_id": 1593780458303008
            },
            "update_id": 46090
        },
        {
            "message": {
                "custom_payload": {
                    "test_ids": []
                },
                "seq_no": 864,
                "from": {
                    "is_bot": False,
                    "login": "nikola",
                    "display_name": "Николай",
                    "id": "5bd62",
                    "uid": 123
                },
                "chat": {
                    "username": "robot-pers-userdata",
                    "type": "private",
                    "id": "5bd62b7b_770c917b"
                },
                "date": 1593780468,
                "document": {
                    "file_name": "2_3104.pdf",
                    "file_id": "file/eada2443-cb18-4506-bddf-b7a97334fa96",
                    "file_size": 129594
                },
                "message_id": 1593780468290008
            },
            "update_id": 46092
        },
        {
            "message": {
                "from": {
                    "is_bot": False,
                    "login": "nikola",
                    "display_name": "Николай",
                    "id": "5bd62",
                    "uid": 123
                },
                "chat": {
                    "username": "robot-pers-userdata",
                    "type": "private",
                    "id": "5bd62b7b_770c917b"
                },
                "date": 1593780447,
                "text": "Help",
                "message_id": 1593780468290010
            },
            "update_id": 460932
        },
    ]
    updates = [Update.from_dict(i) for i in resp]

    d = Dispatcher()
    calls = []

    @d.message_handler(commands=['start'])
    def foo1(message):
        calls.append(1)

    @d.message_handler(content_type=['document'])
    def foo2(message):
        calls.append(2)

    @d.message_handler(content_type=['photo'])
    def foo3(message):
        calls.append(3)

    @d.message_handler(func=lambda m: True)
    def foo4(message):
        calls.append(4)

    for u in updates:
        d.serve_update(u)

    assert_that(calls, equal_to([1, 3, 2, 4]))
