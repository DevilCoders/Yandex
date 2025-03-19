from unittest.mock import MagicMock
import unittest.mock as m

import pytest

from cloud.marketplace.cli.yc_marketplace_cli.commands.migrate import Command


@pytest.mark.parametrize(
    ["func_kwargs", "migrate_calls", "list_migrations_calls_count"],
    [
        ({"target": "0001", "fake": False, "list_migrations": True}, [], 1),
        ({"target": "0001", "fake": False, "list_migrations": False}, [m.call("0001", False)], 0),
        ({"target": "0001", "fake": True, "list_migrations": False}, [m.call("0001", True)], 0),
    ],
)
def test_handle(mocker, func_kwargs, migrate_calls, list_migrations_calls_count):
    manager_mock = mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.migrate.MigrationsPropagate")
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.migrate.marketplace_db")

    mocker.patch.object(Command, "_list_migrations")

    result = Command().handle(**func_kwargs)

    assert result is None
    assert Command._list_migrations.call_count == list_migrations_calls_count
    assert manager_mock.return_value.migrate.mock_calls == migrate_calls


def test__list_migrations(mocker):
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.migrate.sys")
    manager_mock = mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.migrate.MigrationsPropagate")
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.migrate.marketplace_db")

    mocker.patch.object(manager_mock, "list",
                        return_value=[{"migration": MagicMock(), "is_applied": False, "number": "0001"}])

    result = Command()._list_migrations(manager_mock)

    assert result is None
