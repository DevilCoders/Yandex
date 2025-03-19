import http
import pytest
import re

from typing import Dict

from yc_common.clients.network.client import NativeNetworkClient
from yc_common.clients.contrail import exceptions
from yc_common.clients.contrail import client
from yc_common.clients.contrail import resource  # noqa
from yc_common.clients.network.loader import get_network_client_pool
from yc_common.clients.network.config import NetworkEndpointConfig
from yc_common.config import get_value
from yc_common.misc import generate_id
from yc_common.test import core
from yc_common.exceptions import Error


@pytest.fixture(autouse=True)
def network_config(monkeypatch):
    core.monkeypatch_function(
        monkeypatch, get_value,
        return_value=NetworkEndpointConfig.from_api({
            "protocol": "http",
            "schema_version": "3.1",
            "zones": [
                {"name": "dogfood",
                 "oct_clusters": [{
                     "name": "dogfood1",
                     "host": "localhost",
                     "port": 80,
                 }],
                 },
            ]}))
    core.monkeypatch_method(monkeypatch, client.ContrailAPISession.request)


@pytest.fixture(autouse=True)
def mock_fetch(monkeypatch):
    return core.monkeypatch_method(monkeypatch, resource.Resource.fetch,
                                   side_effect=lambda self, *args, **kwargs: self)


@pytest.fixture
def mock_create(monkeypatch):
    return core.monkeypatch_method(monkeypatch, resource.Resource.create,
                                   side_effect=lambda self, *args, **kwargs: self)


@pytest.fixture
def mock_create_conflict(monkeypatch):
    return core.monkeypatch_method(monkeypatch, resource.Resource.create,
                                   side_effect=exceptions.HttpError(http_status=http.HTTPStatus.CONFLICT))


@pytest.fixture
def mock_check(monkeypatch):
    return core.monkeypatch_method(monkeypatch, resource.Resource.check, return_value=True)


@pytest.fixture
def mock_delete(monkeypatch):
    return core.monkeypatch_method(monkeypatch, resource.Resource.delete)


@pytest.fixture
def network_client():
    return get_network_client_pool()["dogfood"]


def test_ensure_virtual_machine(network_client: NativeNetworkClient, mock_create):
    network_client.ensure_virtual_machine(instance_uuid=generate_id(), fqdn=None, compute_instance_id="abc000")

    assert mock_create.call_count == 1


def test_ensure_virtual_machine_exists(network_client: NativeNetworkClient, mock_create_conflict, mock_fetch):
    network_client.ensure_virtual_machine(instance_uuid=generate_id(), fqdn=None, compute_instance_id="abc000")

    assert mock_create_conflict.call_count == 1
    assert mock_fetch.call_count == 1


def test_ensure_virtual_machine_exists_hostname_differs(network_client: NativeNetworkClient, mock_create_conflict):
    with pytest.raises(Error) as excinfo:
        network_client.ensure_virtual_machine(instance_uuid=generate_id(), fqdn="other", compute_instance_id="abc000")

    assert re.match("Requested FQDN .* differs from instance's FQDN", str(excinfo.value))


@pytest.fixture
def mock_port_with_vdns(mock_network_with_ipam, network_client: NativeNetworkClient):
    port = network_client.resource("virtual-machine-interface", uuid=core.mock_id(True))
    port.data["virtual_network_refs"] = [mock_network_with_ipam]

    return port


@pytest.fixture
def mock_port_without_vdns(network_client: NativeNetworkClient):
    return network_client.resource("virtual-machine-interface", uuid=core.mock_id(True))


def monkeypatch_network_client_resource(monkeypatch, mocked_resources: Dict[str, object]):
    original_resource_fn = NativeNetworkClient.resource

    def resource_fn(self, resource_type, *args, **kwargs):
        if resource_type in mocked_resources:
            return mocked_resources[resource_type]
        else:
            return original_resource_fn(self, resource_type, *args, **kwargs)

    core.monkeypatch_method(monkeypatch, NativeNetworkClient.resource, side_effect=resource_fn)


@pytest.fixture
def mock_network_with_ipam(network_client: NativeNetworkClient):
    """Return network with associated ipam and 2 subnets, v6 and v4"""
    vdns = network_client.resource("virtual-DNS",
                                   virtual_DNS_data={"default_ttl_seconds": 60, "domain_name": "example.com"},
                                   uuid=core.mock_id(True))
    ipam = network_client.resource("network-ipam", uuid=core.mock_id(True))
    ipam.data["virtual_DNS_refs"] = [vdns]
    ipam.data["attr"] = {
        "ipam_subnets": [
            {
                "subnet": {"ip_prefix": "192.168.140.0", "ip_prefix_len": 24},
                "subnet_uuid": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
            },
            {
                "subnet": {"ip_prefix": "fc01::", "ip_prefix_len": 64},
                "subnet_uuid": "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb"
            },
        ]
    }
    network = network_client.resource("virtual-network", uuid=core.mock_id(True))
    network.data["network_ipam_refs"] = [ipam]

    return network


def test_port_creation(monkeypatch, network_client: NativeNetworkClient, mock_network_with_ipam, mock_create):
    """Creates a port and checks that all related objects were created

    Should create an instance_ip for exery subnet in network_ipam, a virtual_machine and a
    virtual-machine-interface itself.
    """
    instance_id = core.mock_id(True)
    compute_instance_id = "abc000"
    fqdn = "unit-test.cloud.yandex.net"
    mac_address = "aa:bb:cc:dd:ee:ff"

    core.monkeypatch_method(monkeypatch, NativeNetworkClient.create_fip)
    monkeypatch_network_client_resource(monkeypatch, {"virtual-network": mock_network_with_ipam})

    port = network_client.create_port(port_uuid=core.mock_id(True),
                                      network_uuid=mock_network_with_ipam.uuid,
                                      instance_uuid=instance_id,
                                      compute_instance_id=compute_instance_id,
                                      instance_fqdn=fqdn,
                                      ip_addresses=['192.168.140.5', 'fc01::fefe'],
                                      mac_address=mac_address,
                                      search_domains=["cloud.yandex.net", "auto.yandex.net"])

    assert port.contrail_network_id == mock_network_with_ipam.uuid
    assert port.mac_address == mac_address
    created_objects = [call[0][0] for call in mock_create.call_args_list]
    ips_created = [obj for obj in created_objects if obj.type == "instance-ip"]
    assert len(ips_created) == 2
    assert {"v4", "v6"} == {ip.instance_ip_family for ip in ips_created}
    ports_created = [obj for obj in created_objects if obj.type == "virtual-machine-interface"]
    assert len(ports_created) == 1
    dhcp_options = {o["dhcp_option_name"]: o["dhcp_option_value"] for o in
                    ports_created[0]["virtual_machine_interface_dhcp_option_list"]["dhcp_option"]}
    assert dhcp_options["host-name"] == "unit-test.cloud.yandex.net"
    assert dhcp_options["domain-name"] == "cloud.yandex.net"
    assert dhcp_options["domain-search"] == "cloud.yandex.net auto.yandex.net"
    assert dhcp_options["v6-domain-search"] == "cloud.yandex.net auto.yandex.net"
    vms_created = [obj for obj in created_objects if obj.type == "virtual-machine"]
    assert len(vms_created) == 1
    assert vms_created[0].uuid == instance_id


def test_build_dhcp_options(network_client: NativeNetworkClient):
    assert network_client._build_dhcp_options("unit-test.cloud.yandex.net",
                                              ["cloud.yandex.net", "auto.yandex.net"]) == {'dhcp_option': [
        {'dhcp_option_value': 'unit-test.cloud.yandex.net',
         'dhcp_option_value_bytes': None,
         'dhcp_option_name': 'host-name'},
        {'dhcp_option_value': 'cloud.yandex.net',
         'dhcp_option_value_bytes': None,
         'dhcp_option_name': 'domain-name'},
        {'dhcp_option_value': 'cloud.yandex.net auto.yandex.net',
         'dhcp_option_value_bytes': None,
         'dhcp_option_name': 'domain-search'},
        {'dhcp_option_value': 'cloud.yandex.net auto.yandex.net',
         'dhcp_option_value_bytes': None,
         'dhcp_option_name': 'v6-domain-search'}]}
    assert network_client._build_dhcp_options("unit-test", ["cloud.yandex.net", "auto.yandex.net"]) == {'dhcp_option': [
        {'dhcp_option_value': 'unit-test',
         'dhcp_option_value_bytes': None,
         'dhcp_option_name': 'host-name'},
        {'dhcp_option_value': 'cloud.yandex.net auto.yandex.net',
         'dhcp_option_value_bytes': None,
         'dhcp_option_name': 'domain-search'},
        {'dhcp_option_value': 'cloud.yandex.net auto.yandex.net',
         'dhcp_option_value_bytes': None,
         'dhcp_option_name': 'v6-domain-search'}]}
    assert network_client._build_dhcp_options("unit-test.cloud.yandex.net", None) == {'dhcp_option': [
        {'dhcp_option_value': 'unit-test.cloud.yandex.net',
         'dhcp_option_value_bytes': None,
         'dhcp_option_name': 'host-name'},
        {'dhcp_option_value': 'cloud.yandex.net',
         'dhcp_option_value_bytes': None,
         'dhcp_option_name': 'domain-name'}]}
    assert network_client._build_dhcp_options("", None) is None


def test_port_creation_nonexistent_cidr(network_client: NativeNetworkClient, mock_network_with_ipam,
                                        mock_create):
    instance_id = core.mock_id(True)
    instance_name = "unit-test.cloud.yandex.net"
    mac_address = "aa:bb:cc:dd:ee:ff"
    with pytest.raises(Error):
        network_client.create_port(port_uuid=core.mock_id(True),
                                   network_uuid=mock_network_with_ipam.uuid,
                                   instance_uuid=instance_id,
                                   compute_instance_id="abc000",
                                   instance_fqdn=instance_name,
                                   ip_addresses=['10.0.0.5'],
                                   mac_address=mac_address)


@pytest.fixture
def mock_port_with_ips(mock_port_with_vdns, network_client: NativeNetworkClient):
    """Returns a port with 2 ips, floating_ip and a vm"""
    port = mock_port_with_vdns
    port.data["floating_ip_back_refs"] = [network_client.resource("floating-ip", uuid=core.mock_id(True))]
    port.data["instance_ip_back_refs"] = [
        network_client.resource("instance-ip", uuid=core.mock_id(True),
                                instance_ip_address="10.0.0.10", instance_ip_family="v4"),
        network_client.resource("instance-ip", uuid=core.mock_id(True),
                                instance_ip_address="2a02:6b8::10", instance_ip_family="v6")
    ]
    port.data["virtual_machine_refs"] = [network_client.resource("virtual-machine", uuid=core.mock_id(True))]

    return port


def test_port_deletion(monkeypatch, network_client: NativeNetworkClient, mock_port_with_ips):
    """Deletes port and all related resources.

    Should delete virtual-machine-interface, virtual-machine, floating-ip, and 2 instance-ips"""

    mock_delete = core.monkeypatch_method(monkeypatch, resource.Resource.delete)
    monkeypatch_network_client_resource(monkeypatch, {"virtual-machine-interface": mock_port_with_ips})

    network_client.delete_port(mock_port_with_ips.uuid, "abc", "abc000")

    assert mock_delete.call_count == 5
    deleted_types = {call[1][0].type for call in mock_delete.mock_calls}
    assert deleted_types == {"virtual-machine-interface", "virtual-machine", "floating-ip", "instance-ip"}
