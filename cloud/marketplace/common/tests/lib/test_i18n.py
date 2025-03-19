import pytest

from cloud.marketplace.common.yc_marketplace_common.lib.i18n import I18n
from cloud.marketplace.common.yc_marketplace_common.models.i18n import I18n as I18nScheme


@pytest.mark.parametrize(
    ["func_kwargs", "translation", "expected_result"],
    [
        ({"id": "id", "lang": "ru"}, [{"i18n.lang": "ru", "i18n.text": "val"}], "val"),
        ({"id": "id", "lang": "en", "fallback": ["en", "ru"]}, [{"i18n.lang": "ru", "i18n.text": "val"}], "val"),
        ({"id": "id", "lang": "en", "fallback": ["en"]}, [{"i18n.lang": "ru", "i18n.text": "val"}], ""),
        ({"id": "id", "lang": "en", "fallback": ["en"]}, None, ""),
        ({"id": "id"}, [{"i18n.lang": "ru", "i18n.text": "val"}], "val"),
    ],
)
def test_get(mocker, func_kwargs, translation, expected_result, monkeypatch):
    request_mock = mocker.Mock()
    monkeypatch.setattr("cloud.marketplace.common.yc_marketplace_common.lib.i18n.request", request_mock)
    request_mock.accept_languages.best_match.return_value = "ru"
    mocker.patch("yc_common.clients.kikimr.client._KikimrBaseConnection.select", return_value=translation)

    result = I18n.get(**func_kwargs)

    assert result == expected_result


@pytest.mark.parametrize(
    ["func_kwargs", "expected_result"],
    [
        ({"id": "id", "translations": {}}, "%sid" % I18nScheme.PREFIX),
        ({"id": "%sid" % I18nScheme.PREFIX, "translations": {}}, "%sid" % I18nScheme.PREFIX),
    ],
)
def test_set(mocker, func_kwargs, expected_result):
    mocker.patch("yc_common.clients.kikimr.client._KikimrBaseConnection.query")

    result = I18n.set(**func_kwargs)

    assert result == expected_result


@pytest.mark.parametrize(
    ["obj", "expected_keys"],
    [
        ({"name": "val"}, []),
        ({"name": "%scode" % I18nScheme.PREFIX}, ["%scode" % I18nScheme.PREFIX]),
        ({"name": ["%scode" % I18nScheme.PREFIX]}, ["%scode" % I18nScheme.PREFIX]),
        ({"name": {"n": "%scode" % I18nScheme.PREFIX}}, ["%scode" % I18nScheme.PREFIX]),
        ({"name": [{"n": "%scode" % I18nScheme.PREFIX}]}, ["%scode" % I18nScheme.PREFIX]),
        ("%scode" % I18nScheme.PREFIX, ["%scode" % I18nScheme.PREFIX]),
    ],
)
def test_get_keys(obj, expected_keys):
    keys = I18n._get_keys(obj)

    assert keys == expected_keys


@pytest.mark.parametrize(
    ["func_kwargs", "translation", "expected_result"],
    [
        ({"keys": ["id"], "lang": "ru"}, [{"id": "id", "lang": "ru", "text": "val"}], {"id": "val"}),
        ({"keys": ["id"], "lang": "ru"}, [{"id": "id2", "lang": "ru", "text": "val"}], {"id": ""}),
        ({"keys": ["id2"], "lang": "ru"}, [{"id": "id", "lang": "ru", "text": "val"}], {"id2": ""}),
        ({"keys": ["id"], "lang": "ru"}, None, {}),
        ({"keys": ["id"]}, [{"id": "id", "lang": "ru", "text": "val"}], {"id": "val"}),
        (
            {"keys": ["id"], "lang": "ru", "fallback": ["en", "ru"]},
            [{"id": "id", "lang": "en", "text": "val"}],
            {"id": "val"},
        ),
        (
            {"keys": ["id"], "lang": "ru", "fallback": ["ru"]},
            [{"id": "id", "lang": "en", "text": "val"}],
            {"id": ""},
        ),
    ],
)
def test_get_batch(mocker, func_kwargs, translation, expected_result, monkeypatch):
    request_mock = mocker.Mock()
    monkeypatch.setattr("cloud.marketplace.common.yc_marketplace_common.lib.i18n.request", request_mock)
    request_mock.accept_languages.best_match.return_value = "ru"
    # mocker.patch("yc_common.clients.kikimr.client._KikimrBaseConnection.prepare_query")
    mocker.patch("yc_common.clients.kikimr.client._KikimrBaseConnection.select", return_value=translation)

    result = I18n._get_batch(**func_kwargs)

    assert result == expected_result


@pytest.mark.parametrize(
    ["obj", "translations", "expected_translated"],
    [
        ({"name": "val"}, {"%scode" % I18nScheme.PREFIX: "val2"}, {"name": "val"}),
        ({"name": "%scode" % I18nScheme.PREFIX}, {"%scode" % I18nScheme.PREFIX: "val"}, {"name": "val"}),
        ({"name": "code"}, {"code": "val"}, {"name": "code"}),
        ({"name": ["%scode" % I18nScheme.PREFIX]}, {"%scode" % I18nScheme.PREFIX: "val"}, {"name": ["val"]}),
        ({"name": {"n": "%scode" % I18nScheme.PREFIX}}, {"%scode" % I18nScheme.PREFIX: "val"}, {"name": {"n": "val"}}),
    ],
)
def test_fill_items(obj, translations, expected_translated):
    translated = I18n._fill_items(obj, translations)

    assert translated == expected_translated


@pytest.mark.parametrize(
    ["func_kwargs", "get_batch_result", "expected_result"],
    [
        ({"obj": {"name": "%scode" % I18nScheme.PREFIX}}, {"%scode" % I18nScheme.PREFIX: "val"}, {"name": "val"}),
    ],
)
def test_traverse_simple(mocker, func_kwargs, get_batch_result, expected_result):
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.lib.i18n.I18n._get_batch", return_value=get_batch_result)

    result = I18n.traverse_simple(**func_kwargs)

    assert result == expected_result
