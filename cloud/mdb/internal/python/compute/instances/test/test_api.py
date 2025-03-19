from yandex.cloud.priv.compute.v1 import instance_service_pb2
from cloud.mdb.internal.python.compute.instances.api import dns_record_to_spec
from cloud.mdb.internal.python.compute.instances import DnsRecordSpec


def test_dns_record_to_spec_for_abs_fqdn():
    assert dns_record_to_spec(
        DnsRecordSpec(fqdn='foo.db.yt.', dns_zone_id='yc.mdb.zone', ttl=42, ptr=True)
    ) == instance_service_pb2.DnsRecordSpec(
        fqdn='foo.db.yt.',
        dns_zone_id='yc.mdb.zone',
        ptr=True,
        ttl=42,
    )


def test_dns_record_to_spec_for_not_abs_fqdn():
    assert dns_record_to_spec(
        DnsRecordSpec(fqdn='foo.db.yt', dns_zone_id='yc.mdb.zone', ttl=42, ptr=True)
    ) == instance_service_pb2.DnsRecordSpec(
        fqdn='foo.db.yt.',
        dns_zone_id='yc.mdb.zone',
        ptr=True,
        ttl=42,
    )
