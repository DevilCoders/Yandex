import pytest

from fastapi import Request

from asgi_yauth.settings import AsgiYauthConfig
from asgi_yauth.user import (
    AnonymousYandexUser,
    YandexUser,
)
from asgi_yauth.middleware import (
    YauthTestMiddleware,
    YauthMiddleware,
)
from asgi_yauth.backends.tvm2 import (
    AuthTypes,
    AnonymousReasons,
)


@pytest.fixture
def config(monkeypatch):
    monkeypatch.setenv('TVM2_ASYNC', 'true')
    monkeypatch.setenv('QLOUD_TVM_TOKEN', 'test_token')
    monkeypatch.setenv('TVM2_USE_QLOUD', 'true')
    monkeypatch.setenv('YAUTH_TVM2_CLIENT', '28')
    monkeypatch.setenv('YAUTH_TVM2_ALLOWED_CLIENTS', '[2000324]')

    return AsgiYauthConfig()


def test_auth_fail_no_auth_headers(client, app, config):
    app.add_middleware(YauthMiddleware, config=config)

    @app.get('/')
    async def endpoint(request: Request):
        assert request.user.is_authenticated() is False
        assert isinstance(request.user, AnonymousYandexUser)
        return {'data': 'ok'}

    response = client.get('/')

    assert response.status_code == 200, response.text


def test_auth_fail_wrong_service_ticket(client, app, config, test_vcr):
    app.add_middleware(YauthMiddleware, config=config)

    @app.get('/')
    async def endpoint(request: Request):
        assert request.user.is_authenticated() is False
        assert isinstance(request.user, AnonymousYandexUser)
        assert request.user.authenticated_by() == 'tvm2'
        assert request.user.reason == AnonymousReasons.service.value
        return {'data': 'ok'}

    with test_vcr.use_cassette('test_auth_fail_wrong_service_ticket.yaml'):
        response = client.get('/', headers={
            config.tvm2_service_header: 'some_ticket',
        })

    assert response.status_code == 200, response.text


def test_auth_fail_not_allowed_service_ticket(client, app, config, test_vcr):
    app.add_middleware(YauthMiddleware, config=config)

    @app.get('/')
    async def endpoint(request: Request):
        assert request.user.is_authenticated() is False
        assert isinstance(request.user, AnonymousYandexUser)
        assert request.user.authenticated_by() == 'tvm2'
        return {'data': 'ok'}

    with test_vcr.use_cassette('test_auth_fail_not_allowed_service_ticket.yaml'):
        response = client.get('/', headers={
            config.tvm2_service_header: 'some_ticket',
        })

    assert response.status_code == 200, response.text


def test_auth_success_service_ticket(client, app, config, test_vcr):
    app.add_middleware(YauthMiddleware, config=config)

    @app.get('/')
    async def endpoint(request: Request):
        assert request.user.is_authenticated()
        assert isinstance(request.user, YandexUser)
        assert request.user.authenticated_by() == 'tvm2'
        assert request.user.auth_type == AuthTypes.service.value
        assert request.user.service_ticket.src == 2000324
        assert request.user.raw_service_ticket == 'some_ticket'
        return {'data': 'ok'}

    with test_vcr.use_cassette('test_auth_success_service_ticket.yaml'):
        response = client.get('/', headers={
            config.tvm2_service_header: 'some_ticket',
        })

    assert response.status_code == 200, response.text


def test_auth_success_user_ticket(client, app, config, test_vcr):
    app.add_middleware(YauthMiddleware, config=config)

    @app.get('/')
    async def endpoint(request: Request):
        assert request.user.is_authenticated()
        assert isinstance(request.user, YandexUser)
        assert request.user.authenticated_by() == 'tvm2'
        assert request.user.auth_type == AuthTypes.user.value
        assert request.user.service_ticket.src == 2000324
        assert request.user.raw_user_ticket == 'some_user_ticket'
        assert request.user.uid == 456
        assert request.user.user_ticket.uids == [456, 123]
        return {'data': 'ok'}

    with test_vcr.use_cassette('test_auth_success_user_ticket.yaml'):
        response = client.get('/', headers={
            config.tvm2_service_header: 'some_ticket',
            config.tvm2_user_header: 'some_user_ticket',
        })

    assert response.status_code == 200, response.text


def test_yauth_test_middleware(client, app, config,):
    app.add_middleware(YauthTestMiddleware, config=config)

    @app.get('/')
    async def endpoint(request: Request):
        assert request.user.is_authenticated()
        assert isinstance(request.user, YandexUser)
        assert request.user.authenticated_by() == 'tvm2'
        assert request.user.uid == config.test_user_data['uid']
        assert request.user.auth_type == config.test_user_data['auth_type']
        return {'data': 'ok'}

    response = client.get('/')
    assert response.status_code == 200, response.text
