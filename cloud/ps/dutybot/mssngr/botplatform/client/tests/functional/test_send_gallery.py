from hamcrest import (assert_that, equal_to)
from PIL import Image

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot
from mssngr.botplatform.client.src.types import Photo, Gallery


def _test_response():
    return {
        'message': {
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
            "text": "some text",
            "date": 1594906051,
            "photos": [[{
                "file_id": "file/1093870e",
            }], [{
                "file_id": "file/1093870e",
            }]],
            "message_id": 123
        }
    }


def test_send_gallery_by_file(monkeypatch, tmpdir):
    p = tmpdir.mkdir('tmp').join('cool_photo.jpeg')
    img = Image.new("RGB", (20, 30), color='red')
    img.save(open(str(p), "wb"), "JPEG")
    img.close()

    chat_id = "123"
    m = _test_response()

    def mock_do_request(*args, **kwargs):
        print(kwargs)
        assert_that(args[2].endswith("/sendGallery/"), equal_to(True))  # url
        assert_that(kwargs["data"]["chat_id"], equal_to(chat_id))
        assert_that(kwargs["data"]["payload_id"])
        assert_that(kwargs["files"][0][0], equal_to("photo"))
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    msg = Bot("123", "123").send_gallery(files=[open(str(p), "rb"), open(str(p), "rb")], chat_id=chat_id)
    assert_that(isinstance(msg.gallery.photos[0], Photo), equal_to(True))
    assert_that(msg.text, equal_to("some text"))
    assert_that(msg.gallery.photos[0].file_id, equal_to("file/1093870e"))
    assert_that(msg.gallery.photos[0].width, equal_to(20))
    assert_that(msg.gallery.photos[0].height, equal_to(30))
    assert_that(msg.gallery.photos[0].name, equal_to("photo"))


def test_send_gallery_by_path(monkeypatch, tmpdir):
    p = tmpdir.mkdir('tmp').join('cool_photo.jpeg')
    img = Image.new("RGB", (20, 30), color='red')
    img.save(open(str(p), "wb"), "JPEG")
    img.close()

    chat_id = "123"
    m = _test_response()

    def mock_do_request(*args, **kwargs):
        print(kwargs)
        assert_that(args[2].endswith("/sendGallery/"), equal_to(True))  # url
        assert_that(kwargs["data"]["chat_id"], equal_to(chat_id))
        assert_that(kwargs["data"]["payload_id"])
        assert_that(kwargs["files"][0][0], equal_to("photo"))
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    msg = Bot("123", "123").send_gallery(files=[str(p), str(p)], chat_id=chat_id)
    assert_that(msg.text, equal_to("some text"))
    assert_that(isinstance(msg.gallery.photos[0], Photo), equal_to(True))
    assert_that(msg.gallery.photos[0].file_id, equal_to("file/1093870e"))
    assert_that(msg.gallery.photos[0].width, equal_to(20))
    assert_that(msg.gallery.photos[0].height, equal_to(30))
    assert_that(msg.gallery.photos[0].name, equal_to("photo"))


def test_send_gallery_by_file_id(monkeypatch):
    chat_id = "123"
    m = _test_response()
    photo = Photo.from_dict({
        "file_id": "file/1093870e",
        "size": 10,
        "width": 20,
        "height": 30,
        "file_name": "_name_"
    })

    def mock_do_request(*args, **kwargs):
        print(kwargs)
        assert_that(args[2].endswith("/sendGallery/"), equal_to(True))  # url
        assert_that(kwargs["json"]["chat_id"], equal_to(chat_id))
        assert_that(kwargs["json"]["items"][0]["image"]["filename"], equal_to('_name_'))
        assert_that(kwargs["json"]["items"][0]["image"]["size"], equal_to(10))
        assert_that(kwargs["json"]["items"][0]["image"]["width"], equal_to(20))
        assert_that(kwargs["json"]["items"][0]["image"]["height"], equal_to(30))
        assert_that(kwargs["json"]["payload_id"])

        assert_that(kwargs["json"]["text"], equal_to("hello"))
        m["message"]["text"] = kwargs["json"]["text"]
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    msg = Bot("123", "123").send_gallery(Gallery([photo, photo]), text="hello", chat_id=chat_id)
    assert_that(msg.text, equal_to("hello"))
    assert_that(isinstance(msg.gallery.photos[0], Photo), equal_to(True))
    assert_that(msg.gallery.photos[0].file_id, equal_to("file/1093870e"))
    assert_that(msg.gallery.photos[0].width, equal_to(20))
    assert_that(msg.gallery.photos[0].height, equal_to(30))
    assert_that(msg.gallery.photos[0].size, equal_to(10))
    assert_that(msg.gallery.photos[0].name, equal_to("_name_"))
