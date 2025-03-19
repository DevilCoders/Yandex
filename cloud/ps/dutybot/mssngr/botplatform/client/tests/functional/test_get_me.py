import requests

from hamcrest import (assert_that, equal_to)

from mssngr.botplatform.client.src.test_utils import MockResponse
from mssngr.botplatform.client.src.bot import Bot


def test_get_me(monkeypatch):
    d = {
        "nickname": "robot",
        "display_name": u"Витя",
        "description": None,
        "webhook_url": None,
        "guid": "770c917b",
        "settings_with_defaults": {
            "get_seen_markers": False,
            "send_message_first": True,
            "zora_enabled": False,
            "manual_seen_marker": False
        }
    }

    def mack_request(*args, **kwargs):
        return MockResponse(d, 200)

    monkeypatch.setattr(requests.Session, "request", mack_request)
    bot = Bot("123", "123")
    assert_that(bot.nickname, equal_to(d["nickname"]))
    assert_that(bot.display_name, equal_to(d["display_name"]))
    assert_that(bot.guid, equal_to(d["guid"]))
    assert_that(bot.description, equal_to(None))
    assert_that(bot.webhook_url, equal_to(None))


def test_get_me_full(monkeypatch):
    d = {
        "nickname": "robot",
        "display_name": u"Витя",
        "description": u"Очень хороший бот",
        "webhook_url": "http://coolsite.ru/hook",
        "guid": "770c917b",
        "settings_with_defaults": {
            "get_seen_markers": False,
            "send_message_first": True,
            "zora_enabled": False,
            "manual_seen_marker": False
        }
    }

    def mack_request(*args, **kwargs):
        return MockResponse(d, 200)

    monkeypatch.setattr(requests.Session, "request", mack_request)
    bot = Bot("123", "123")
    assert_that(bot.nickname, equal_to(d["nickname"]))
    assert_that(bot.display_name, equal_to(d["display_name"]))
    assert_that(bot.guid, equal_to(d["guid"]))
    assert_that(bot.description, equal_to(d["description"]))
    assert_that(bot.webhook_url, equal_to(d["webhook_url"]))
