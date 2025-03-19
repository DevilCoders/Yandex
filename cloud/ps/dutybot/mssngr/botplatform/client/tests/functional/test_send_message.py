from hamcrest import (assert_that, equal_to)

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot


def test_send_message(monkeypatch):
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
    chat_id = "123"

    def mock_do_request(*args, **kwargs):
        assert_that(args[2].endswith("/sendMessage/"), equal_to(True))  # url
        assert_that(kwargs["json"]["text"], equal_to(text))
        assert_that(kwargs["json"]["chat_id"], equal_to(chat_id))
        assert_that(kwargs["json"]["payload_id"])
        assert_that(kwargs["json"]["card"])
        assert_that(kwargs["json"]["reply_markup"]["inline_keyboard"][0]["text"], equal_to("hello"))
        assert_that(len(kwargs["json"]["reply_markup"]["inline_keyboard"]), equal_to(2))
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    Bot("123", "123").send_message(
        text="123",
        reply_markup=[
            {"text": "hello", "url": "hello.com"},
            {"text": "hello", "callback_data": "hello.com"}
        ],
        card={"data": {}},
        chat_id="123"
    )
