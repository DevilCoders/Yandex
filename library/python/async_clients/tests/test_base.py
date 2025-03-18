import pytest
from unittest import mock
import aiohttp

from async_clients.exceptions.base import (
    NoRetriesLeft,
    BadResponseStatus,
    AIOHTTPClientException,
)

pytestmark = pytest.mark.asyncio


async def test_get_request_success(dummy_client, test_vcr):
    dummy_client.AUTH_TYPES = tuple()

    client = dummy_client(
        host='https://httpbin.org',
    )
    with test_vcr.use_cassette('test_get_request_success.yaml'):
        result = await client._make_request(
            path='get',
            params={'some': 'test'}
        )
    assert result['url'] == 'https://httpbin.org/get?some=test'


async def test_post_request_success(dummy_client, test_vcr):
    dummy_client.AUTH_TYPES = tuple()

    client = dummy_client(
        host='https://httpbin.org',
    )
    data = {
        'some': 'one',
        'other': 'two',
    }
    with test_vcr.use_cassette('test_post_request_success.yaml'):
        result = await client._make_request(
            path='post',
            method='post',
            json=data,
        )
    assert result['form'] == data


async def test_combine_headers_success(dummy_client, test_vcr):
    dummy_client.get_headers = lambda self: {'default_header': 'test'}

    client = dummy_client(
        host='https://httpbin.org',
        token='my_token'
    )

    with test_vcr.use_cassette('test_headers_success.yaml'):
        result = await client._make_request(
            path='get',
            headers={'other_header': 'test_other'}
        )
    assert result['headers']['Authorization'] == 'OAuth my_token'
    assert result['headers']['Default-Header'] == 'test'
    assert result['headers']['Other-Header'] == 'test_other'


async def test_text_type_success(dummy_client, test_vcr):
    dummy_client.RESPONSE_TYPE = 'text'
    dummy_client.AUTH_TYPES = tuple()

    client = dummy_client(
        host='https://httpbin.org',
    )

    with test_vcr.use_cassette('test_text_type_success.yaml'):
        result = await client._make_request(
            path='get',
        )
    assert isinstance(result, str)


async def test_json_type_success(dummy_client, test_vcr):
    dummy_client.RESPONSE_TYPE = 'json'
    dummy_client.AUTH_TYPES = tuple()

    client = dummy_client(
        host='https://httpbin.org',
    )

    with test_vcr.use_cassette('test_json_type_success.yaml'):
        result = await client._make_request(
            path='get',
        )
    assert isinstance(result, dict)


async def test_retries_success(dummy_client, test_vcr):
    dummy_client.AUTH_TYPES = tuple()

    client = dummy_client(
        host='https://httpbin.org',
        retries=1,
    )

    with test_vcr.use_cassette('test_retries_success.yaml'):
        result = await client._make_request(
            path='get',
        )
    assert result['url'] == 'https://httpbin.org/get'


async def test_hide_headers(dummy_client, test_vcr):
    dummy_client.AUTH_TYPES = tuple()

    client = dummy_client(
        host='https://httpbin.org',
        retries=1,
        log_headers_value={'smth', }
    )

    with mock.patch.object(
        dummy_client,
        'parse_response',
        side_effect=aiohttp.ClientResponseError(
            request_info='test',
            history='test',
            headers={'smth': 'test', 'another': 'one'}
        )
    ):
        with test_vcr.use_cassette('test_json_type_success.yaml'):
            with pytest.raises(AIOHTTPClientException) as e_info:
                await client._make_request(
                    path='get',
                )
    assert e_info.value.args[0] == ("Got aiohttp exception: ClientResponseError('test', "
                                    "'test', headers={'smth': 'test', 'another': '******'})")


async def test_retries_fail(dummy_client, test_vcr):
    dummy_client.AUTH_TYPES = tuple()

    client = dummy_client(
        host='https://httpbin.org',
        retries=1,
    )

    with test_vcr.use_cassette('test_retries_fail.yaml'):
        with pytest.raises(NoRetriesLeft):
            await client._make_request(
                path='get',
            )


async def test_no_retries_on_bad_status_success(dummy_client, test_vcr):
    dummy_client.AUTH_TYPES = tuple()

    client = dummy_client(
        host='https://httpbin.org',
        retries=100,
    )

    with test_vcr.use_cassette('test_no_retries_on_bad_status_success.yaml'):
        with pytest.raises(BadResponseStatus):
            await client._make_request(
                path='get',
            )
