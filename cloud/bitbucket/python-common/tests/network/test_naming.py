import pytest
from yc_common.clients.network import naming


@pytest.mark.parametrize("ip_address", [
    "10.0.0.10",
    "2a02:6b8:001:002:0:fe01:ffaa:bbcc",
    "2a02:6b8::1"
])
def test_fip_name_parsing(ip_address):
    compute_instance_id = "dugc9ico45o3ravmspup"
    port_uuid = "8075ab96-fa0c-4c99-8217-03dabf6e67d9"

    fq_name = naming.fip(compute_instance_id, port_uuid, ip_address)

    parsed_compute_instance_id, parsed_port_uuid, parsed_ip_address = naming.parse_fip(fq_name)

    assert parsed_compute_instance_id == compute_instance_id
    assert parsed_port_uuid == port_uuid
    assert parsed_ip_address == ip_address


def test_fip_pool_name_parsing():
    fip_bucket_name = "public@ru-central1-a"

    fq_name = naming.fip_pool(fip_bucket_name)

    parsed_fip_bucket_name = naming.parse_fip_pool(fq_name)

    assert parsed_fip_bucket_name == fip_bucket_name


def test_fip_name():
    fq_name = naming.fip(compute_instance_id="aaa", port_uuid="bbb", ip_address="1.2.3.4")
    assert fq_name == "fip-aaa-bbb-1-2-3-4"


def test_fip_pool_name():
    fq_name = naming.fip_pool(fip_bucket_name="public@ru-central1-a")
    assert fq_name == "fip-pool-public@ru-central1-a"
