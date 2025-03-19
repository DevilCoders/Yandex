import requests

from hamcrest import (assert_that, equal_to)

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot


def test_get_chats(monkeypatch):
    d = [
        {
            'id': '5bd62b7123',
            'type': 'private',
            'username': 'nikola'
        },
        {
            'id': '0/3/8fa12312',
            'type': 'group',
            'title': '42345',
            'description': '',
        }
    ]

    def mack_request(*args, **kwargs):
        return MockResponse(d, 200)

    monkeypatch.setattr(requests.Session, "request", mack_request)
    monkeypatch.setattr(Bot, "get_me", lambda *_: None)

    chats = Bot("123", "123").get_chats()
    assert_that(len(chats), equal_to(2))
    assert_that(chats[1].id, equal_to('0/3/8fa12312'))
