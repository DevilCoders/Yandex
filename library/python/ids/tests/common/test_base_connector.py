# coding: utf-8

from __future__ import unicode_literals

import pytest
from mock import patch
from pretend import stub

from ids.connector.plugins_lib import PluginBase
from ids.connector import HttpConnector as Connector
from ids.exceptions import AuthError, BackendError, ConfigurationError

DUMMY_HOST = 'wf.net'
DUMMY_PROTOCOL = 'http'


def init_connector(cls=None, **init_params):
    """
    Хелпер, чтобы проставить некоторые параметры по умолчанию
    """
    host = init_params.pop('host', DUMMY_HOST)
    protocol = init_params.pop('protocol', DUMMY_PROTOCOL)
    cls = cls or Connector
    return cls(host=host, protocol=protocol, user_agent='ids-test', **init_params)


@patch(
    'ids.connector.http.get_config',
    new=lambda *a, **kw: {'host': DUMMY_HOST, 'protocol': DUMMY_PROTOCOL}
)
def test_init_with_config():
    connector = init_connector()
    assert connector.host == DUMMY_HOST
    assert connector.protocol == DUMMY_PROTOCOL


def test_init_with_params():
    connector = init_connector(host='api.ya.ru', protocol='ftp')
    assert connector.host == 'api.ya.ru'
    assert connector.protocol == 'ftp'


def test_init_with_kwargs():
    connector = init_connector(with_kitchen_knife=True)
    assert connector.with_kitchen_knife


def test_get_resource_pattern():
    class SomeConnector(Connector):
        url_patterns = {'user_list': 'users/'}

    get_pattern = SomeConnector(
        user_agent='dummy',
        host='dummmy.yandex-team.ru',
        protocol='https',
    ).get_resource_pattern
    assert get_pattern(resource='user_list') == 'users/'
    assert get_pattern(resource='unknown') == ''
    assert get_pattern(resource=None) == ''


def test_build_url_without_resource():
    connector = init_connector()

    url_prefix = DUMMY_PROTOCOL + '://' + DUMMY_HOST

    assert connector.build_url() == url_prefix

@patch(
    'ids.connector.http.HttpConnector.get_resource_pattern',
    new=lambda *a, **kw: 'resource_list/'
)
def test_build_url_with_resource():
    connector = init_connector()

    url_prefix = DUMMY_PROTOCOL + '://' + DUMMY_HOST

    expected = url_prefix + 'resource_list/'
    assert connector.build_url(resource='WHATEVER') == expected


@patch(
    'ids.connector.http.HttpConnector.get_resource_pattern',
    new=lambda *a, **kw: 'resource_list/'
)
@patch(
    'ids.connector.http.HttpConnector.get_query_params',
    new=lambda *a, **kw: {'token': 'planner'}
)
def test_build_url_with_resource_additional_params():
    connector = init_connector()

    url_prefix = DUMMY_PROTOCOL + '://' + DUMMY_HOST

    expected = url_prefix + 'resource_list/?token=planner'
    assert connector.build_url(resource='WHATEVER') == expected


@patch(
    'ids.connector.HttpConnector.get_resource_pattern',
    new=lambda *a, **kw: 'resource_list/?with=spoon'
)
@patch(
    'ids.connector.HttpConnector.get_query_params',
    new=lambda *a, **kw: {'token': 'planner'}
)
def test_build_url_with_resource_with_merge_of_params():
    connector = init_connector()

    url_prefix = DUMMY_PROTOCOL + '://' + DUMMY_HOST

    expected = url_prefix + 'resource_list/?with=spoon&token=planner'
    assert connector.build_url(resource='WHATEVER') == expected


def test_execute_request_get_normal():
    connector = init_connector()
    expected_response = stub(status_code=200, text='some response')
    connector.session = stub(request=lambda *a, **kw: expected_response)

    response = connector.execute_request(method='get')

    assert response == expected_response


def test_execute_request_post_normal():
    connector = init_connector()
    expected_response = stub(status_code=200, text='some response')
    connector.session = stub(request=lambda *a, **kw: expected_response)

    response = connector.execute_request(method='post')

    assert response == expected_response


def test_execute_request_get_403():
    connector = init_connector()
    connector.session = stub(request=lambda *a, **kw: stub(
        status_code=403,
        text="Auth Error or smth, bro"
    ))

    with pytest.raises(AuthError):
        connector.execute_request()


def test_execute_request_get_non_200():
    connector = init_connector()
    connector.session = stub(request=lambda *a, **kw: stub(
        status_code=418,
        text="I'm nginx based teapot",
    ))

    with pytest.raises(BackendError):
        connector.execute_request()


def test_get():
    connector = init_connector()
    expected_response = stub(status_code=200, text='some response')
    connector.session = stub(request=lambda *a, **kw: expected_response)

    response = connector.get()

    assert response == expected_response


def test_post():
    connector = init_connector()
    expected_response = stub(status_code=200, text='some response')
    connector.session = stub(request=lambda *a, **kw: expected_response)

    response = connector.post()

    assert response == expected_response


def test_check_collect_required_params_no_plugins():
    class JellyConnector(Connector):
        required_params = ['jelly', 'spoon']

    connector = init_connector(cls=JellyConnector, jelly=1, spoon=2)
    expected = set(['jelly', 'spoon'])
    assert connector.collect_required_params() == expected


def test_check_collect_required_params_with_plugins():
    class Plugin(PluginBase):
        required_params = ['jar']

    class JellyConnector(Connector):
        required_params = ['jelly', 'spoon']
        plugins = [Plugin]

    expected = set(['jelly', 'spoon', 'jar'])
    connector = init_connector(cls=JellyConnector, jelly=1, spoon=2, jar=3)
    assert connector.collect_required_params() == expected


def test_check_required_params_all_set():
    class JellyConnector(Connector):
        required_params = ['jelly', 'spoon']

    connector = init_connector(cls=JellyConnector, jelly=True, spoon=False)

    assert connector.jelly
    assert not connector.spoon


def test_check_required_params_not_all():
    class JellyConnector(Connector):
        required_params = ['jelly', 'spoon']

    with pytest.raises(ConfigurationError):
        init_connector(cls=JellyConnector, jelly=True)


def test_check_required_params_in_plugins_success():
    class Plugin(PluginBase):
        def check_required_params(self, params):
            return

    class JellyConnector(Connector):
        plugins = [Plugin]

    init_connector(cls=JellyConnector)


def test_check_required_params_in_plugins_fail():
    class Plugin(PluginBase):
        def check_required_params(self, params):
            raise ConfigurationError()

    class JellyConnector(Connector):
        required_params = ['jelly']
        plugins = [Plugin]

    with pytest.raises(ConfigurationError):
        init_connector(cls=JellyConnector, jelly=True)


def test_prepare_params_in_plugins():
    class Plugin(PluginBase):
        def prepare_params(self, params):
            params['hello'] = 'world'

    class JellyConnector(Connector):
        plugins = [Plugin]

    connector = init_connector(cls=JellyConnector)
    params = {}
    connector._prepare_params(params)

    assert params == {'hello': 'world'}


def test_handle_response_in_plugins():
    SOMETHING_ELSE = object()

    class Plugin(PluginBase):
        def handle_response(self, response):
            return SOMETHING_ELSE

    class JellyConnector(Connector):
        plugins = [Plugin]

    connector = init_connector(cls=JellyConnector)
    response = object()
    handled_response = connector._handle_response(response)

    assert handled_response is not response
    assert handled_response is SOMETHING_ELSE


def test_update_headers_user_agent():
    connector = Connector('host', 'proto', user_agent='Batman')

    params = {}

    connector._add_user_agent(params)

    assert 'User-Agent' in params['headers']
    assert params['headers']['User-Agent'] == 'Batman'
