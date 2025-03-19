import requests
import pytest

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot
from mssngr.botplatform.client.src.exceptions import NotFoundError


def test_send_seen_marker(monkeypatch):
    def mack_request(*args, **kwargs):
        return MockResponse({}, 200)

    monkeypatch.setattr(requests.Session, "request", mack_request)
    monkeypatch.setattr(Bot, "get_me", lambda *_: None)

    Bot("123", "123").send_seen_marker(123, chat_id="123")


def test_send_seen_marker_not_found(monkeypatch):
    d = {
        "code": "message_not_found",
        "error": "Message not found"
    }

    def mack_request(*args, **kwargs):
        return MockResponse(d, 404)

    monkeypatch.setattr(requests.Session, "request", mack_request)
    monkeypatch.setattr(Bot, "get_me", lambda *_: None)

    with pytest.raises(NotFoundError):
        Bot("123", "123").send_seen_marker(123, chat_id="123")
