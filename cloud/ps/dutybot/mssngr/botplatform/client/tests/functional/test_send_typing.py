import requests
import pytest

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot
from mssngr.botplatform.client.src.exceptions import NotFoundError


def test_send_typing(monkeypatch):
    def mack_request(*args, **kwargs):
        return MockResponse({}, 200)

    monkeypatch.setattr(requests.Session, "request", mack_request)
    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    Bot("123", "123").send_typing(chat_id="123")


def test_send_typing_not_found(monkeypatch):
    d = {
        "code": "fanout_status_no_such_chat",
        "error": "Fanout request failed"
    }

    def mack_request(*args, **kwargs):
        return MockResponse(d, 404)

    monkeypatch.setattr(requests.Session, "request", mack_request)
    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    with pytest.raises(NotFoundError):
        Bot("123", "123").send_typing(chat_id="123")
