import json
from hamcrest import (assert_that, equal_to)

import mssngr.botplatform.client.src.types as lib_types


def test_chat_from_json_1():
    input_dict = {
        "id": "01231231_123",
        "type": u"group",
        "title": u"42345 имя",
        "description": ""
    }
    s = json.dumps(input_dict)
    chat = lib_types.Chat.from_json(s)
    assert_that(chat.id, equal_to(input_dict["id"]))
    assert_that(chat.type, equal_to(input_dict["type"]))
    assert_that(chat.title, equal_to(input_dict["title"]))
    assert_that(chat.description, equal_to(input_dict["description"]))


def test_chat_from_json_2():
    input_dict = {
        "id": "01231231_123",
        "type": u"private",
        "title": u"42345 имя",
    }
    s = json.dumps(input_dict)
    chat = lib_types.Chat.from_json(s)
    assert_that(chat.id, equal_to(input_dict["id"]))
    assert_that(chat.type, equal_to(input_dict["type"]))
    assert_that(chat.title, equal_to(input_dict["title"]))
    assert_that(chat.description, equal_to(None))


def test_chat_to_json():
    d = {
        "username": "robotsdaf-pers-adf",
        "type": "private",
        "id": "dfdafafc-613c629471a1"
    }
    chat = lib_types.Chat.from_dict(d)
    assert_that(chat.to_dict(), equal_to(d))


def test_user():
    input_dict = {
        "is_bot": False,
        "login": "nikola",
        "display_name": u"Николай Рулев",
        "id": "123123abc123",
        "uid": 1120000000212758
    }
    user = lib_types.User.from_dict(input_dict)
    assert_that(user.to_dict(), equal_to(input_dict))
    assert_that(
        lib_types.User.from_json(user.to_json()).to_dict(),
        equal_to(input_dict)
    )

    assert_that(user.is_bot, equal_to(input_dict["is_bot"]))
    assert_that(user.login, equal_to(input_dict["login"]))
    assert_that(user.display_name, equal_to(input_dict["display_name"]))
    assert_that(user.id, equal_to(input_dict["id"]))
    assert_that(user.uid, equal_to(input_dict["uid"]))


def test_entity():
    input_dict = {
        "length": 37,
        "type": "mention",
        "offset": 0
    }
    entity = lib_types.Entity.from_dict(input_dict)
    assert_that(entity.to_dict(), equal_to(input_dict))
    assert_that(
        lib_types.Entity.from_json(entity.to_json()).to_dict(),
        equal_to(input_dict)
    )

    assert_that(entity.type, equal_to(input_dict["type"]))
    assert_that(entity.length, equal_to(input_dict["length"]))
    assert_that(entity.offset, equal_to(input_dict["offset"]))


def test_message():
    d = {
        "custom_payload": {
            "test_ids": []
        },
        "seq_no": 761,
        "from": {
            "is_bot": False,
            "login": "nikola",
            "display_name": "Николай",
            "id": "5bd62b7b2582adfasdfa1349ac03addd73418f",
            "uid": 112000123
        },
        "reply_to_message": {
            "date": 1591107647,
            "text": "123",
            "from": {
                "is_bot": False,
                "login": "nikola",
                "display_name": "Николай",
                "id": "5bd62b7b25824987a9ac03addd73418f",
                "uid": 1123123123131123
            },
            "message_id": 1591107647004005,
            "chat": {
                "username": "robot",
                "type": "private",
                "id": "5bd62b7b-2582-4987-71a1"
            }
        },
        "chat": {
            "username": "robot-1231231",
            "type": "private",
            "id": "5bewrwec629471a1"
        },
        "author": {
            "is_bot": False,
            "login": "nikola",
            "display_name": "Николай",
            "id": "5bd62b7b2582adfasdfa1349ac03addd73418f",
            "uid": 112000123
        },
        "entities": [
            {
                "length": 37,
                "type": "mention",
                "offset": 0
            }
        ],
        "date": 1590501231231127838,
        "text": "@0000000-0000-1111-a9ac-03addd73418f",
        "message_id": 123231312
    }
    message = lib_types.Message.from_dict(d)
    assert_that(message.to_dict()["entities"], equal_to(d["entities"]))
    assert_that(message.from_user.to_dict(), equal_to(d["from"]))
    assert_that(message.chat.to_dict(), equal_to(d["chat"]))
    assert_that(message.text, equal_to(d["text"]))
    assert_that(message.date, equal_to(d["date"]))
    assert_that(message.id, equal_to(d["message_id"]))


def test_update():
    d = {
        "update_id": 0,
        "message": {
            "from": {
                "is_bot": False,
                "login": "nikola",
                "display_name": u"Николай",
                "id": "5bd62b7b2582adfasdfa1349ac03addd73418f",
                "uid": 112000123
            },
            "chat": {
                "username": "robot-1231231",
                "type": "private",
                "id": "5bewrwec629471a1"
            },
            "author": {
                "is_bot": False,
                "login": "nikola",
                "display_name": u"Николай",
                "id": "5bd62b7b2582adfasdfa1349ac03addd73418f",
                "uid": 112000123
            },
            "entities": [
                {
                    "length": 37,
                    "type": "mention",
                    "offset": 0
                }
            ],
            "date": 1590501231231127838,
            "text": "@0000000-0000-1111-a9ac-03addd73418f",
            "message_id": 123231312
        }
    }
    update = lib_types.Update.from_dict(d)
    assert_that(update.to_dict())
    cd = d.copy()
    cd.update(update.to_dict())
    print(cd)
    assert_that(cd, equal_to(d))
    assert_that(update.id, equal_to(d["update_id"]))


def test_webhook_info():
    d = {
        "url": "http://coolUrl.ru/hook",
        "pending_update_count": 3
    }
    w = lib_types.WebhookInfo.from_dict(d)
    assert_that(w.to_dict(), equal_to(d))
    assert_that(w.url, equal_to(d["url"]))
    assert_that(w.pending_update_count, equal_to(d["pending_update_count"]))
