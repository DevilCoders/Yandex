from cloud.mdb.dbaas_worker.internal.providers.dns.client import (
    ensure_dot,
    normalized_address,
    records_stable_hash,
    Record,
)

A_RECORD = Record('10.10.10.10', 'A')
AAAA_RECORD = Record('2001:0db8::0001:0000', 'AAAA')


def test_get_fqdn():
    assert ensure_dot('') == '.'
    assert ensure_dot('.') == '.'
    assert ensure_dot('a') == 'a.'


def test_normalized_address():
    assert normalized_address('2001:0db8::0001:0000', 'AAAA') == normalized_address('2001:db8::1:0', 'AAAA')


def test_records_stable_hash_for_same_records():
    assert records_stable_hash([A_RECORD, AAAA_RECORD]) == records_stable_hash([AAAA_RECORD, A_RECORD])


def test_records_stable_hash_for_different():
    assert records_stable_hash([A_RECORD, AAAA_RECORD]) != records_stable_hash([A_RECORD])
