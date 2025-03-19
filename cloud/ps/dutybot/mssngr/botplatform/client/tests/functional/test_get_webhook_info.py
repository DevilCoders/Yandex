import requests

from hamcrest import (assert_that, equal_to)

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot


def test_get_webhook_info(monkeypatch):
    d = {
        "url": "",
        "pending_update_count": 2,
    }

    def mack_request(*args, **kwargs):
        return MockResponse(d, 200)

    monkeypatch.setattr(requests.Session, "request", mack_request)
    monkeypatch.setattr(Bot, "get_me", lambda *_: None)

    info = Bot("123", "123").get_webhook_info()
    assert_that(info.url, equal_to(""))
    assert_that(info.pending_update_count, equal_to(2))
