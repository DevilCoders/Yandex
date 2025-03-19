import pytest
from yc_common import exceptions, ids


def test_ids_prefix_correct():
    resource_id = ids.generate_id("abc")
    assert len(resource_id) == 20


def test_ids_prefix_invalid():
    with pytest.raises(exceptions.Error):
        ids.generate_id("!", validate_prefix=True)


def test_ids_conversions():
    resource_id = ids.generate_id("abc")

    uuid = ids.convert_to_uuid(resource_id)
    assert len(uuid) == 36

    resource_id_restored = ids.convert_from_uuid(uuid)
    assert resource_id_restored == resource_id
