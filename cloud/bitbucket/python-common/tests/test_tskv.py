from yc_common.tskv import dict_to_tskv, tskv_to_dict

test_dict_data = {'key': 'value'}
test_tskv_data = "tskv\tkey=value"

test_dict_data_with_eq = {'key=shmey': 'value=shmalue'}
test_tskv_data_with_eq = "tskv\tkey\\=shmey=value=shmalue"


def test_dict_to_tskv():
    assert dict_to_tskv(test_dict_data) == test_tskv_data


def test_dict_to_tskv_escaped_key_value():
    assert dict_to_tskv(test_dict_data_with_eq) == test_tskv_data_with_eq


def test_tskv_to_dict():
    assert dict(tskv_to_dict(test_tskv_data)) == test_dict_data


def test_tskv_to_dict_escaped_key_value():
    assert dict(tskv_to_dict(test_tskv_data_with_eq)) == test_dict_data_with_eq
