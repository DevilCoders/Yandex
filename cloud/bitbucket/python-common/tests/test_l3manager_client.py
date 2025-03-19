""" L3ManagerClient test """

import copy
import time
import json
import requests
from typing import List
from urllib.parse import parse_qs

import responses
import pytest
from unittest.mock import patch

import yc_common.config
from .utils import mock_api
from yc_common.clients import l3manager
from yc_common.clients.l3manager import LoadBalancer, Service, Configuration, VirtualServer, VirtualServerState, RealServer, RealServerState
from yc_common.test.core import monkeypatch_function

BASE_URL = "https://l3mgr.localhost/api/v1"


def _wrap_responses(url: str, replies: List):
    def callback(_):
        if len(callback.replies) == 1:
            return callback.replies[0]
        return callback.replies.pop(0)
    callback.replies = copy.copy(replies)

    responses.add_callback(responses.GET, url, callback=callback, content_type="text")


@pytest.fixture(autouse=True)
def monkeypatch_config(monkeypatch):
    def get_value(name, model=None, default=None, converter=None):
        if name == "endpoints.l3manager":
            return model({"token": "TEST_TOKEN", "base_url": BASE_URL})
        return default

    monkeypatch_function(monkeypatch, yc_common.config.get_value, side_effect=get_value)


@responses.activate
@mock_api(
    "%s/balancer" % BASE_URL,
    responses.GET,
    mock_file="l3manager/list_balancers.json"
)
def test_list_balancers():
    l3 = l3manager.get_l3manager_client()
    items = l3.list_balancers()
    assert len(items) == 4
    item = items[0]
    assert isinstance(item, LoadBalancer)
    assert item.fqdn == "sas1-lb26.yndx.net"
    assert item.name == "sas1-lb26"
    assert not item.test_env
    assert item.location == ["SAS"]
    assert item.url == "/api/v1/balancer/67"
    assert item.state == "ACTIVE"
    assert not item.full
    assert item.id == 67


@responses.activate
@mock_api(
    "%s/service" % BASE_URL,
    responses.GET,
    mock_file="l3manager/list_services.json"
)
def test_list_services():
    l3 = l3manager.get_l3manager_client()
    items = l3.list_services()
    assert len(items) == 3
    item = items[0]
    assert isinstance(item, Service)
    assert not item.archive
    assert item.vs is None
    assert item.fqdn == "api-sas-tuman.cloud.yandex.net"
    assert item.abc == "cloud"
    assert item.config is None
    assert item.id == 269
    assert item.action is None
    assert item.state == "ACTIVE"
    assert item.url == "/api/v1/service/269"


@responses.activate
@mock_api(
    "%s/service/798/config" % BASE_URL,
    responses.GET,
    mock_file="l3manager/list_service_configurations.json"
)
def test_list_service_configurations():
    l3 = l3manager.get_l3manager_client()
    items = l3.list_service_configurations(798)
    assert len(items) == 3
    item = items[0]
    assert isinstance(item, Configuration)
    assert item.description == "Updated state \"DEPLOYING\" -> \"ACTIVE\""
    # assert isinstance(item.service, Service)
    assert item.vs_id == [4114]
    assert str(item.timestamp) == "2018-02-05 09:19:29.372000+00:00"
    assert item.id == 2490
    assert item.url == "/api/v1/service/798/config/2490"
    assert item.state == "ACTIVE"
    assert item.comment == "zed-0xff: Autoconfig on RS updating"
    assert item.history == [1867, 1868, 1869, 1870, 1873, 2416, 2417, 2419, 2420, 2429, 2430, 2435, 2445, 2455, 2456,
                            2467, 2476, 2483]


@responses.activate
@mock_api(
    "%s/service/798/vs" % BASE_URL,
    responses.GET,
    mock_file="l3manager/list_service_virtual_servers.json"
)
def test_list_service_virtual_servers():
    l3 = l3manager.get_l3manager_client()
    items = l3.list_service_virtual_servers(798)
    assert len(items) == 3
    item = items[0]
    assert isinstance(item, VirtualServer)
    assert item.total_count is None
    assert item.protocol == "TCP"
    assert len(item.config) == 19
    assert item.config["WEIGHT_DC_MAN"] == 100
    assert not item.config["DYNAMICACCESS"]
    assert item.config["CHECK_TYPE"] == "TCP_CHECK"
    assert item.config["CHECK_RETRY_TIMEOUT"] is None
    assert item.lb == []
    assert item.ip == "2a02:6b8:0:3400:0:587:0:1b"
    assert len(item.status) == 1
    assert isinstance(item.status[0], VirtualServerState)
    assert item.url == "/api/v1/service/798/vs/4114"
    assert item.ext_id == "52be149afc550bd3fd1332a87a9dba7c1cf7a89f812e3666af4afeaa14f173ed"
    assert item.id == 4114
    assert item.group == ["%ycprod_region_man1_head"]
    assert item.editable is None
    assert item.active_count is None
    assert item.port == 80


@responses.activate
@mock_api(
    "%s/service/798" % BASE_URL,
    responses.GET,
    mock_file="l3manager/get_service.json"
)
def test_get_service():
    l3 = l3manager.get_l3manager_client()
    item = l3.get_service(798)
    assert isinstance(item, Service)
    assert not item.archive
    assert len(item.vs) == 1
    assert isinstance(item.vs[0], VirtualServer)
    assert item.fqdn == "marketplace-test.cloud.yandex.net"
    assert item.abc == "cloud"
    assert isinstance(item.config, Configuration)
    assert item.id == 798
    assert item.action == ["edit"]
    assert item.state == "ACTIVE"
    assert item.url == "/api/v1/service/798"


@responses.activate
@mock_api(
    "%s/service/798/config/2490" % BASE_URL,
    responses.GET,
    mock_file="l3manager/get_service_configuration.json"
)
def test_get_service_configuration():
    l3 = l3manager.get_l3manager_client()
    item = l3.get_service_configuration(798, 2490)
    assert isinstance(item, Configuration)
    assert item.description == "Updated state \"DEPLOYING\" -> \"ACTIVE\""
    # assert isinstance(item.service, Service)
    assert item.vs_id == [4114]
    assert str(item.timestamp) == "2018-02-05 09:19:29.372000+00:00"
    assert item.id == 2490
    assert item.url == "/api/v1/service/798/config/2490"
    assert item.state == "ACTIVE"
    assert item.comment == "zed-0xff: Autoconfig on RS updating"
    assert item.history == [1867, 1868, 1869, 1870, 1873, 2416, 2417, 2419, 2420, 2429, 2430, 2435, 2445, 2455, 2456,
                            2467, 2476, 2483]


@responses.activate
@mock_api(
    "%s/vs/4114/rsstate" % BASE_URL,
    responses.GET,
    mock_file="l3manager/get_real_server_states.json"
)
def test_get_real_server_states():
    l3 = l3manager.get_l3manager_client()
    items = l3.get_real_server_states(4114)
    assert len(items) == 3
    item = items[0]
    assert isinstance(item, RealServerState)
    assert item.fwmark == 1748
    assert item.description == "Updated at 2018-02-05 13:17:09.481643+00:00"
    assert isinstance(item.lb, LoadBalancer)
    assert isinstance(item.rs, RealServer)
    assert item.state == "ACTIVE"
    assert str(item.timestamp) == "2018-02-05 13:17:09.481000+00:00"


@responses.activate
def test_create_service_configuration():
    def callback(request):
        params = parse_qs(request.body)
        assert params == {"comment": ["test"], "vs": ["2849"]}
        assert request.headers["Content-Type"] == "application/x-www-form-urlencoded"
        headers = {}
        body = json.dumps({"object": {"id": 123}})
        return (200, headers, body)

    responses.add_callback(responses.POST,
                           BASE_URL + "/service/798/config",
                           callback=callback,
                           content_type="application/json")

    l3 = l3manager.get_l3manager_client()
    id = l3.create_service_configuration(798, {"comment": "test", "vs": 2849})
    assert id == 123


@responses.activate
def test_deploy_service_configuration():
    responses.add(responses.POST,
                  BASE_URL + "/service/798/config/2849/process",
                  json={"object": {"id": 123}},
                  content_type="application/json")

    l3 = l3manager.get_l3manager_client()
    id = l3.deploy_service_configuration(798, 2849)
    assert id == 123


@responses.activate
def test_replace_service_reals():
    def callback(request):
        params = parse_qs(request.body)
        assert params == {"group": ["real1", "real2", "real3"]}
        assert request.headers["Content-Type"] == "application/x-www-form-urlencoded"
        headers = {}
        body = json.dumps({"object": {"id": 123}})
        return (200, headers, body)

    responses.add_callback(responses.POST,
                           BASE_URL + "/service/798/editrs",
                           callback=callback,
                           content_type="application/json")

    l3 = l3manager.get_l3manager_client()
    id = l3.replace_service_reals(798, ["real1", "real2", "real3"])
    assert id == 123


@responses.activate
@pytest.mark.parametrize("replies,expected_exception", [
    (
        [
            (requests.codes.ok, {"Content-Type": "application/json"}, '{ "objects": [] }'),
        ],
        None,
    ),
    (
        [
            (requests.codes.internal_server_error, {"Content-Type": "application/json"},
             '{ "result": "No result", "message": "Got Error" }'),
            (requests.codes.bad_gateway, {"Content-Type": "text/html"},
             '{ "result": "No result", "message": "Got Error" }'),
            (requests.codes.ok, {"Content-Type": "application/json"}, '{ "objects": [] }'),
        ],
        None,
    ),
    (
        [
            (requests.codes.bad_gateway, {"Content-Type": "text/html"},
             '{ "result": "No result", "message": "Got Error" }'),
        ],
        "The server returned '502 Bad Gateway' status code",
    ),
])
def test_l3_manager_strange_5xx_retries(replies, expected_exception):
    """Here we test only for errors, which are not handled by yc_common.clients.api.ApiClient"""
    _wrap_responses("{}/service?limit=10".format(BASE_URL), replies)

    client = l3manager.get_l3manager_client()
    client.retry_temporary_errors = False

    with patch.object(time, "sleep", return_value=None):
        if expected_exception:
            with pytest.raises(Exception, match=expected_exception):
                client.list_services(params=dict(limit=10))
        else:
            client.list_services(params=dict(limit=10))
