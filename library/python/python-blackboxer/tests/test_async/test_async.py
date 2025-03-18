import pytest

from blackboxer import AsyncBlackbox, FieldRequiredError, InvalidParamsError, ResponseError


@pytest.fixture
async def client():
    url = "http://pass-test.yandex.ru/blackbox/"
    client = AsyncBlackbox(url=url)
    yield client


@pytest.fixture
def user():
    """ Пользователь паспорта

    тестового пользователя можно зарегестрировать на тестовом паспорте: https://passport-test.yandex.ru
    урл для тестового ЧЯ: http://pass-test.yandex.ru/blackbox/
    :return:
    """

    login = "test.tes2017"
    id = "4006929137"
    password = "7jLD/Bo?f98U"
    Session_id = "3:1498489529.5.0.1498126389827:uGhJZIfgHdAEMwAAuAYCKg:9.1|4006929137.0.2|279813.581085.CNfRjjtu9U2Goz9Bi9ecnt4YYdE"
    sessionid2 = "3:1498489529.5.0.1498126389827:uGhJZIfgHdAEMwAAuAYCKg:9.1|4006929137.0.2|279813.923638.w67dXFT7yF_0RCsF0QpXKchy_wg"
    oauth_token_expired = "24dc10049bec4f353452809ec2b82282"
    oauth_token = "AQAAAADu1OLxAAAMpZqzOpdVqUL_iO8IE_-J3zA"
    L = "RztfdV9DSW8OdwlEdw52VU15U1gLY0JMIgFEEHwACyJcYHJa.1498126389.9629.343809.5a638346ff7fe08d7048e3f8edeb7143"

    yield dict(
        login=login,
        id=id,
        password=password,
        sessionid=Session_id,
        sessionid2=sessionid2,
        oauth_token=oauth_token,
        oauth_token_expired=oauth_token_expired,
        L=L,
    )


@pytest.mark.asyncio
@pytest.mark.async_http_mock
async def test_userinfo(client, user):
    """

    :param dict user:
    :param Blackbox client:
    :return:
    """
    user_ip = "127.0.0.1"
    resp = await client.userinfo(user_ip, login=user["login"])

    resp_user = resp["users"][0]

    assert resp_user["id"] == user["id"]
    assert resp_user["login"] == user["login"]

    with pytest.raises(FieldRequiredError):
        await client.userinfo(user_ip)

    # поля uid и login взаимоисключающие - берем одно из них
    resp = await client.userinfo(user_ip, uid=user["id"], login=user["login"])
    resp_user = resp["users"][0]

    assert resp_user["id"] == user["id"]
    assert resp_user["login"] == user["login"]


@pytest.mark.asyncio
@pytest.mark.async_http_mock(http_method="POST")
async def test_login(client, user):
    """

    :param Blackbox client:
    :param dict user:
    :return:
    """
    user_ip = "127.0.0.1"
    resp = await client.login(
        user_ip, password=user["password"], authtype="auth", login=user["login"]
    )

    assert resp["login"] == user["login"]
    assert resp["error"] == "OK"
    assert resp["status"]["value"] == "VALID"

    with pytest.raises(FieldRequiredError):
        await client.login(user_ip, password="", authtype="")

    # поля uid и login взаимоисключающие - берем одно из них
    resp = await client.login(
        user_ip, password=user["password"], authtype="auth", login=user["login"], uid=user["id"]
    )

    assert resp["login"] == user["login"]
    assert resp["error"] == "OK"
    assert resp["status"]["value"] == "VALID"


@pytest.mark.asyncio
@pytest.mark.async_http_mock
async def test_sessionid(client, user):
    """

    :param Blackbox client:
    :param dict user:
    :return:
    """
    user_ip = "127.0.0.1"
    resp = await client.sessionid(user_ip, user["sessionid"], host="yandex.ru")

    assert resp["login"] == user["login"]
    assert resp["error"] == "OK"
    assert resp["status"]["value"] == "VALID"


@pytest.mark.asyncio
@pytest.mark.async_http_mock
async def test_oauth(client, user):
    """

    как получить валидный токен:
    1. https://oauth-test.yandex.ru завести новое приложение
    2. https://tech.yandex.ru/oauth/doc/dg/tasks/get-oauth-token-docpage/ воспользоваться инструкцией

    или сходить по урлу
    https://oauth-test.yandex.ru/authorize?response_type=token&client_id=70d46ba60c37455bafe7577999321ee5

    :param Blackbox client:
    :param dict user:
    :return:
    """
    user_ip = "127.0.0.1"
    resp = await client.oauth(user_ip, user["oauth_token"])

    assert resp["login"] == user["login"]
    assert resp["error"] == "OK"
    assert resp["status"]["value"] == "VALID"


@pytest.mark.asyncio
@pytest.mark.async_http_mock
async def test_oauth_expired(client, user):
    user_ip = "127.0.0.1"
    resp = await client.oauth(user_ip, user["oauth_token_expired"])
    assert resp["error"] == "expired_token"


@pytest.mark.asyncio
@pytest.mark.async_http_mock
async def test_lcookie(client, user):
    """

    :param Blackbox client:
    :param dict user:
    :return:
    """
    resp = await client.lcookie(user["L"])
    assert resp["login"] == user["login"]
    assert resp["uid"] == user["id"]


@pytest.mark.asyncio
@pytest.mark.async_http_mock
async def test_lcookie_wrong_value(client, dump_as_json):
    with pytest.raises(ResponseError):
        await client.lcookie("qwerty")


@pytest.mark.asyncio
@pytest.mark.async_http_mock
async def test_check_ip(client):
    """

    :param Blackbox client:
    :return:
    """

    nets = "yandexusers"
    ip = "127.0.0.1"

    resp = await client.checkip(ip, nets)

    assert not resp["yandexip"]


@pytest.mark.asyncio
@pytest.mark.async_http_mock
async def test_check_ip_wrong_nets(client):
    ip = "127.0.0.1"

    with pytest.raises(InvalidParamsError):
        await client.checkip(ip, "wrong nets")
