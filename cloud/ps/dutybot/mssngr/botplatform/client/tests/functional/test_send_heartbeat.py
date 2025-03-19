import requests
import pytest

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot
from mssngr.botplatform.client.src.exceptions import AuthError


def test_send_heartbeat(monkeypatch):
    def mack_request(*args, **kwargs):
        return MockResponse({}, 200)

    monkeypatch.setattr(requests.Session, "request", mack_request)
    monkeypatch.setattr(Bot, "get_me", lambda *_: None)

    Bot("123", "123").send_heartbeat(123)


def test_send_heartbeat_auth_error(monkeypatch):
    d = {
        "error": "By UID: None Message: expired_token"
    }

    def mack_request(*args, **kwargs):
        return MockResponse(d, 401)

    monkeypatch.setattr(requests.Session, "request", mack_request)
    monkeypatch.setattr(Bot, "get_me", lambda *_: None)

    with pytest.raises(AuthError):
        Bot("123", "123").send_heartbeat()
