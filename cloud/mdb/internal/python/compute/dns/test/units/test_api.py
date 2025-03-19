from cloud.mdb.internal.python.compute.dns.api import list_records_filter


def test_list_records_filter_with_name_only():
    assert list_records_filter("foo.db.", []) == 'name="foo.db."'


def test_list_records_filter_with_name_and_types():
    assert list_records_filter("foo.db.", ["A", "AAAA"]) == 'name="foo.db." AND type IN ("A", "AAAA")'
