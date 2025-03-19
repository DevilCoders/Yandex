from hamcrest import (assert_that, equal_to)

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot
from mssngr.botplatform.client.src.types import Message, File


def test_send_document_by_file(monkeypatch, tmpdir):
    p = tmpdir.mkdir('tmp').join('cool_file.txt')
    p.write("abcde")
    f = open(str(p))

    chat_id = "123"
    m = {
        'message': {
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
                "id": "id_id"
            },
            "author": {
                "is_bot": True,
                "login": "robot",
                "display_name": "",
                "id": "id",
                "uid": 1120000000084378
            },
            "date": 1594902663,
            'document': {
                'file_id': 'file/1093870e',
                'file_name': "cool_file.txt",
                'file_size': 5
            },
            'message_id': 123
        }
    }

    def mock_do_request(*args, **kwargs):
        assert_that(args[2].endswith("/sendDocument/"), equal_to(True))  # url
        assert_that(kwargs["data"]["chat_id"], equal_to(chat_id))
        assert_that(kwargs["data"]["filename"], equal_to('cool_file.txt'))
        assert_that(kwargs["data"]["payload_id"])
        assert_that(kwargs.get("files"))
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    m = Bot("123", "123").send_document(f, chat_id=chat_id)
    assert_that(isinstance(m, Message), equal_to(True))
    assert_that(m.document.size, equal_to(5))
    assert_that(m.document.file_id, equal_to("file/1093870e"))
    assert_that(m.document.name, equal_to("cool_file.txt"))


def test_send_document_by_path(monkeypatch, tmpdir):
    p = tmpdir.mkdir('tmp').join('cool_file.txt')
    p.write("abcde")

    chat_id = "123"
    m = {
        'message': {
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
                "id": "id_id"
            },
            "author": {
                "is_bot": True,
                "login": "robot",
                "display_name": "",
                "id": "id",
                "uid": 1120000000084378
            },
            "date": 1594902663,
            'document': {
                'file_id': 'file/1093870e',
                'file_name': "cool_file.txt",
                'file_size': 5
            },
            'message_id': 123
        }
    }

    def mock_do_request(*args, **kwargs):
        assert_that(args[2].endswith("/sendDocument/"), equal_to(True))  # url
        assert_that(kwargs["data"]["chat_id"], equal_to(chat_id))
        assert_that(kwargs["data"]["filename"], equal_to('cool_file.txt'))
        assert_that(kwargs["data"]["payload_id"])
        assert_that(kwargs.get("files"))
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    m = Bot("123", "123").send_document(str(p), chat_id=chat_id)
    assert_that(isinstance(m, Message), equal_to(True))
    assert_that(m.document.size, equal_to(5))
    assert_that(m.document.file_id, equal_to("file/1093870e"))
    assert_that(m.document.name, equal_to("cool_file.txt"))


def test_send_document_by_file_id(monkeypatch):
    chat_id = "123"
    m = {
        'message': {
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
                "id": "id_id"
            },
            "author": {
                "is_bot": True,
                "login": "robot",
                "display_name": "",
                "id": "id",
                "uid": 1120000000084378
            },
            "date": 1594902663,
            'document': {
                'file_id': 'file/1093870e',
                'file_name': "name",
                'file_size': 123
            },
            'message_id': 123
        }
    }
    file = File.from_dict({
        "file_id": 'file/1093870e',
        "file_name": 'name',
        "file_size": 123,
    })

    def mock_do_request(*args, **kwargs):
        assert_that(args[2].endswith("/sendDocument/"), equal_to(True))  # url
        assert_that(kwargs["data"]["chat_id"], equal_to(chat_id))
        assert_that(kwargs["data"]["filename"], equal_to('name'))
        assert_that(kwargs["data"]["size"], equal_to(123))
        assert_that(kwargs["data"]["file_id"], equal_to('file/1093870e'))
        assert_that(kwargs["data"]["payload_id"])
        return MockResponse(m, 200)

    monkeypatch.setattr(Bot, "get_me", lambda *_: None)
    monkeypatch.setattr(Bot, "_do_request_with_retries", mock_do_request)

    m = Bot("123", "123").send_document(file, chat_id=chat_id)
    assert_that(isinstance(m, Message), equal_to(True))
    assert_that(m.document.size, equal_to(123))
    assert_that(m.document.file_id, equal_to("file/1093870e"))
    assert_that(m.document.name, equal_to("name"))
