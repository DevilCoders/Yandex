from hamcrest import (assert_that, equal_to)
from PIL import Image

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot
from mssngr.botplatform.client.src.types import Message, Photo


def test_send_photo_by_file(monkeypatch, tmpdir):
    p = tmpdir.mkdir('tmp').join('cool_photo.jpeg')
    img = Image.new("RGB", (20, 30), color='red')
    img.save(open(str(p), "wb"), "JPEG")
    img.close()

    photo = open(str(p), "rb")

    chat_id = "123"
    m = {
        "message": {
            "seq_no": 1046,
            "from": {
                "is_bot": True,
                "login": "robot-pers-userdata",
                "display_name": "",
                "id": "id1",
                "uid": 123
            },
            "chat": {
                "username": "robot-pers-userdata",
                "type": "private",
                "id": "id1_id2"
            },
            "author": {
                "is_bot": True,
                "login": "robot-pers-userdata",
                "display_name": "",
                "id": "id1",
                "uid": 123
            },
            "date": 1594906051,
            "photo": [
                {
                    "width": 0,
                    "file_id": "file/1093870e?size=small",
                    "height": 150
                },
                {
                    "width": 0,
                    "file_id": "file/1093870e?size=middle",
                    "height": 250
                },
                {
                    "width": 0,
                    "file_id": "file/1093870e?size=middle-400",
                    "height": 400
                },
                {
                    "width": 20,
                    "size": 10,
                    "file_id": "file/1093870e",
                    "height": 30
                }
            ],
            "message_id": 1594906051999008
        }
    }

    def mock_do_request(*args, **kwargs):
        print(kwargs)
        assert_that(args[2].endswith("/sendPhoto/"), equal_to(True))  # url
        assert_that(kwargs["data"]["chat_id"], equal_to(chat_id))
        assert_that(kwargs["data"]["filename"], equal_to('cool_photo.jpeg'))
        assert_that(kwargs["data"]["payload_id"])
        assert_that(kwargs["files"].get("photo"))
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    m = Bot("123", "123").send_photo(photo, chat_id=chat_id)
    assert_that(isinstance(m, Message), equal_to(True))
    assert_that(m.photo.file_id, equal_to("file/1093870e"))
    assert_that(m.photo.width, equal_to(20))
    assert_that(m.photo.height, equal_to(30))
    assert_that(m.photo.size, equal_to(10))
    assert_that(m.photo.name, equal_to(""))


def test_send_photo_by_path(monkeypatch, tmpdir):
    p = tmpdir.mkdir('tmp').join('cool_photo.jpeg')
    img = Image.new("RGB", (20, 30), color='red')
    img.save(open(str(p), "wb"), "JPEG")
    img.close()

    chat_id = "123"
    m = {
        "message": {
            "seq_no": 1046,
            "from": {
                "is_bot": True,
                "login": "robot-pers-userdata",
                "display_name": "",
                "id": "id1",
                "uid": 123
            },
            "chat": {
                "username": "robot-pers-userdata",
                "type": "private",
                "id": "id1_id2"
            },
            "author": {
                "is_bot": True,
                "login": "robot-pers-userdata",
                "display_name": "",
                "id": "id1",
                "uid": 123
            },
            "date": 1594906051,
            "photo": [
                {
                    "width": 0,
                    "file_id": "file/1093870e?size=small",
                    "height": 150
                },
                {
                    "width": 0,
                    "file_id": "file/1093870e?size=middle",
                    "height": 250
                },
                {
                    "width": 0,
                    "file_id": "file/1093870e?size=middle-400",
                    "height": 400
                },
                {
                    "width": 20,
                    "size": 10,
                    "file_id": "file/1093870e",
                    "height": 30
                }
            ],
            "message_id": 1594906051999008
        }
    }

    def mock_do_request(*args, **kwargs):
        print(kwargs)
        assert_that(args[2].endswith("/sendPhoto/"), equal_to(True))  # url
        assert_that(kwargs["data"]["chat_id"], equal_to(chat_id))
        assert_that(kwargs["data"]["filename"], equal_to('cool_photo.jpeg'))
        assert_that(kwargs["data"]["payload_id"])
        assert_that(kwargs["files"].get("photo"))
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    m = Bot("123", "123").send_photo(str(p), chat_id=chat_id)
    assert_that(isinstance(m, Message), equal_to(True))
    assert_that(m.photo.file_id, equal_to("file/1093870e"))
    assert_that(m.photo.width, equal_to(20))
    assert_that(m.photo.height, equal_to(30))
    assert_that(m.photo.size, equal_to(10))
    assert_that(m.photo.name, equal_to(""))


def test_send_photo_by_file_id(monkeypatch):
    chat_id = "123"
    m = {
        "message": {
            "seq_no": 1046,
            "from": {
                "is_bot": True,
                "login": "robot-pers-userdata",
                "display_name": "",
                "id": "id1",
                "uid": 123
            },
            "chat": {
                "username": "robot-pers-userdata",
                "type": "private",
                "id": "id1_id2"
            },
            "author": {
                "is_bot": True,
                "login": "robot-pers-userdata",
                "display_name": "",
                "id": "id1",
                "uid": 123
            },
            "date": 1594906051,
            "photo": [
                {
                    "width": 0,
                    "file_id": "file/1093870e?size=small",
                    "height": 150
                },
                {
                    "width": 0,
                    "file_id": "file/1093870e?size=middle",
                    "height": 250
                },
                {
                    "width": 0,
                    "file_id": "file/1093870e?size=middle-400",
                    "height": 400
                },
                {
                    "width": 20,
                    "size": 10,
                    "file_id": "file/1093870e",
                    "height": 30
                }
            ],
            "message_id": 1594906051999008
        }
    }
    photo = Photo.from_dict({
        'file_id': 'file/1093870e',
        "size": 10,
        "width": 20,
        "height": 30,
    })

    def mock_do_request(*args, **kwargs):
        print(kwargs)
        assert_that(args[2].endswith("/sendPhoto/"), equal_to(True))  # url
        assert_that(kwargs["data"]["chat_id"], equal_to(chat_id))
        assert_that(kwargs["data"]["filename"], equal_to('photo'))
        assert_that(kwargs["data"]["size"], equal_to(10))
        assert_that(kwargs["data"]["width"], equal_to(20))
        assert_that(kwargs["data"]["height"], equal_to(30))
        assert_that(kwargs["data"]["payload_id"])
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    m = Bot("123", "123").send_photo(photo, chat_id=chat_id)
    assert_that(isinstance(m, Message), equal_to(True))
    assert_that(m.photo.file_id, equal_to("file/1093870e"))
    assert_that(m.photo.width, equal_to(20))
    assert_that(m.photo.height, equal_to(30))
    assert_that(m.photo.size, equal_to(10))
    assert_that(m.photo.name, equal_to(""))
