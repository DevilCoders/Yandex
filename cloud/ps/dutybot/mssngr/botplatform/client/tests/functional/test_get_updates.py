import requests

from hamcrest import (assert_that, equal_to)

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot


def test_get_updates(monkeypatch):
    d = [{
        "message": {
            "custom_payload": {
                "test_ids": []
            },
            "seq_no": 780,
            "from": {
                "is_bot": False,
                "login": "nikolay",
                "display_name": "Николай",
                "id": "id",
                "uid": 123123
            },
            "chat": {
                "username": "robot",
                "type": "private",
                "id": "5bd62b7b-2582a1"
            },
            "author": {
                "is_bot": False,
                "login": "nikolay",
                "display_name": "Николай Рулев",
                "id": "id",
                "uid": 123123
            },
            "date": 1231231,
            "text": "123",
            "message_id": 1231233
        },
        "update_id": 44043
    }]

    def mock_request(*args, **kwargs):
        return MockResponse(d, 200)

    monkeypatch.setattr(requests.Session, "request", mock_request)
    monkeypatch.setattr(Bot, "get_me", lambda *_: None)

    b = Bot("123", "123")
    updates = b.get_updates()
    assert_that(len(updates), equal_to(1))
    assert_that(updates[0].id, equal_to(44043))
