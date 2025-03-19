from hamcrest import (assert_that, equal_to)

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot


def test_edit_message(monkeypatch):
    m = {
        "message": {
            "from": {
                "is_bot": True,
                "login": "robot-pers-userdata",
                "display_name": "",
                "id": "123",
                "uid": 123
            },
            "author": {
                "is_bot": True,
                "login": "robot-pers-userdata",
                "display_name": "",
                "id": "123",
                "uid": 123
            },
            "text": "hello1",
            "custom_payload": {
                "tts": "hello1",
                "replay_to_payload_id": None
            },
            "seq_no": 825,
            "chat": {
                "username": "robot-pers-userdata",
                "type": "private",
                "id": "123-2582-4987"
            },
            "date": 1592381309,
            "message_id": 1592381309509008
        }
    }

    text = "123"
    message_id = 123
    chat_id = "123"
    important = None

    def mock_do_request(*args, **kwargs):
        print("called {} {}".format(args, kwargs))
        assert_that(args[2].endswith("/editMessage/"), equal_to(True))  # url
        assert_that(kwargs["json"]["message_id"], equal_to(message_id))
        assert_that(kwargs["json"]["chat_id"], equal_to(chat_id))
        if text is not None:
            assert_that(kwargs["json"]["text"], equal_to(text))
        if important is not None:
            assert_that(kwargs["json"]["important"], equal_to(True))
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    Bot("123", "123").edit_message(123, chat_id="123", text="123")

    text=None
    important=True

    Bot("123", "123").edit_message(123, chat_id="123", important=True)
