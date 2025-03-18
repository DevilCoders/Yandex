import pytest


from async_clients.exceptions.base import AuthKwargsMissing


def test_base_auth_config_fail(dummy_client):
    with pytest.raises(AuthKwargsMissing):
        dummy_client(
            host='https://test.ru',
        )


def test_base_auth_config_success(dummy_client):
    service_ticket = 'service_ticket'
    user_ticket = 'user_ticket'

    client = dummy_client(
        host='https://test.ru',
        service_ticket=service_ticket,
        user_ticket=user_ticket,
    )
    assert client.auth_headers == {
        'X-Ya-Service-Ticket': service_ticket,
        'X-Ya-User-Ticket': user_ticket,
    }


def test_base_auth_multiple_config_success(dummy_client):
    service_ticket = 'service_ticket'
    user_ticket = 'user_ticket'

    client = dummy_client(
        host='https://test.ru',
        service_ticket=service_ticket,
        user_ticket=user_ticket,
        token='my_token',
    )
    assert client.auth_headers == {
        'X-Ya-Service-Ticket': service_ticket,
        'X-Ya-User-Ticket': user_ticket,
    }


def test_tvm2_auth_config_success(dummy_client):
    service_ticket = 'service_ticket'
    user_ticket = 'user_ticket'

    client = dummy_client(
        host='https://test.ru',
        auth_type='tvm2',
        service_ticket=service_ticket,
        user_ticket=user_ticket,
    )
    assert client.auth_headers == {
        'X-Ya-Service-Ticket': service_ticket,
        'X-Ya-User-Ticket': user_ticket,
    }


def test_tvm2_auth_config_only_service_success(dummy_client):
    service_ticket = 'service_ticket'

    client = dummy_client(
        host='https://test.ru',
        auth_type='tvm2',
        service_ticket=service_ticket,
    )
    assert client.auth_headers == {
        'X-Ya-Service-Ticket': service_ticket,
    }


def test_tvm2_auth_config_fail(dummy_client):
    with pytest.raises(AuthKwargsMissing):
        dummy_client(
            host='https://test.ru',
            auth_type='tvm2',
        )


def test_token_auth_config_fail(dummy_client):
    with pytest.raises(AuthKwargsMissing):
        dummy_client(
            host='https://test.ru',
            auth_type='oauth',
        )


def test_token_auth_config_success(dummy_client):
    token = 'my_token'

    client = dummy_client(
        host='https://test.ru',
        auth_type='oauth',
        token=token,
    )

    assert client.auth_headers == {
        'Authorization': f'OAuth {token}',
    }
