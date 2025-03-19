import pytest
from cloud.mdb.internal.python.pg_create_users.internal.base import parse_role_config


@pytest.mark.parametrize("empty", [None, []])
def test_parse_role_config_for_empty(empty):
    assert parse_role_config(empty) == {}


def test_parse_role_config_for_real_example():
    assert parse_role_config(
        [
            "default_transaction_isolation=read committed",
            "log_statement=mod",
            "search_path=public",
        ]
    ) == {
        "default_transaction_isolation": "read committed",
        "log_statement": "mod",
        "search_path": "public",
    }


def test_parse_role_config_for_duplicate_settings():
    with pytest.raises(RuntimeError):
        parse_role_config(["foo=bar", "foo=baz"])
