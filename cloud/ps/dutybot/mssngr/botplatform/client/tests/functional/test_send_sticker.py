from hamcrest import (assert_that, equal_to)

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot
from mssngr.botplatform.client.src.types import Sticker


def test_send_sticker(monkeypatch):
    sticker_id = "123.png"
    set_id = "123"
    chat_id = "123"

    msg = {
        "message": {
            "seq_no": 1044,
            "from": {
                "is_bot": True,
                "login": "robot",
                "display_name": "",
                "id": "id",
                "uid": 1120000000084378
            },
            "chat": {
                "username": "robot-pers-userdata",
                "type": "private",
                "id": "5bd62b7b-2582-4987-a9ac-03addd73418f_770c917b-ce32-4457-b29c-613c629471a1"
            },
            "author": {
                "is_bot": True,
                "login": "robot",
                "display_name": "",
                "id": "id",
                "uid": 1120000000084378
            },
            "date": 1594902663,
            "sticker": {
                "file_id": "stickers/images/55/1029.png",
                "set_id": "55"
            },
            "message_id": 1594902663373008
        }
    }

    def mock_do_request(*args, **kwargs):
        print("called {} {}".format(args, kwargs))
        assert_that(args[2].endswith("/sendSticker/"), equal_to(True))  # url
        assert_that(kwargs["data"]["sticker_id"], equal_to(sticker_id))
        assert_that(kwargs["data"]["set_id"], equal_to(set_id))
        assert_that(kwargs["data"]["chat_id"], equal_to(chat_id))
        return MockResponse(msg, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    Bot("123", "123").send_sticker(Sticker(set_id, sticker_id), chat_id=chat_id)
