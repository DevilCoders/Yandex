import logging
import re
from unittest.mock import MagicMock

import pytest

from yc_common.clients.kikimr import get_custom_kikimr_client
from yc_marketplace_migrations.migration import BaseMigration
from yc_marketplace_migrations.migration import MigrationsPropagate
from yc_marketplace_migrations.utils.errors import MigrationBadConfiguredError
from yc_marketplace_migrations.utils.errors import MigrationPathNotCoincides
from yc_marketplace_migrations.utils.errors import MigrationPathNotFound

log = logging.getLogger()
table_name = "migrations"
client = get_custom_kikimr_client("marketplace", "yc-testing-2.yandex.net:2135", "/Root/MarketplaceMigrationsTest")
query_get = client.query
query_set = client.select


class Migration(BaseMigration):
    def execute(self):
        pass

    def rollback(self):
        pass


class Module(MagicMock):
    @staticmethod
    def Migration():  # noqa
        return "test"


class MigrationMock:
    def __init__(self, name):
        self.name = name


@pytest.mark.parametrize(
    ["module", "expected_result"],
    [
        ("app", "app"),
        ("yc_marketplace_common.migrations.0000_initial", "0000_initial"),
    ],
)
def test_migration_name(mocker, module, expected_result):
    mocker.patch.object(Migration, "__module__", module)

    result = Migration().name

    assert result == expected_result


@pytest.mark.parametrize(
    ["filename", "expected_result"],
    [
        ("", None),
        ("0000_test", "0000"),
        ("9999_test", "9999"),
    ],
)
def test_extract_number(mocker, filename, expected_result):
    mocker.patch.object(MigrationsPropagate, "NUMBER_REGEXP", re.compile(r"^[0-9]{4}"))

    result = MigrationsPropagate(log, None, query_get, query_set, table_name).extract_number(filename)

    assert result == expected_result


def test_get_disk_migrations_equals_numbers(mocker):
    numbers = ["0000", "0001", "0000"]
    disk_migrations = MagicMock()
    disk_migrations.__file__ = ""
    disk_migrations.__package__ = "yc_marketplace_migrations.example"

    mocker.patch("yc_marketplace_migrations.migration.list_migrations_files", return_value=numbers)
    mocker.patch("yc_marketplace_migrations.migration.MigrationsPropagate.extract_number", side_effect=numbers)
    importlib_mock = mocker.patch("yc_marketplace_migrations.migration.importlib")
    importlib_mock.import_module.return_value = MagicMock()

    with pytest.raises(MigrationBadConfiguredError):
        assert MigrationsPropagate(log, disk_migrations, query_get, query_set, table_name).disk_migrations


def test_get_disk_migrations_no_migration_class(mocker):
    numbers = ["0000"]
    disk_migrations = MagicMock()
    disk_migrations.__file__ = ""
    disk_migrations.__package__ = "yc_marketplace_migrations.example"

    mocker.patch("yc_marketplace_migrations.migration.list_migrations_files", return_value=numbers)
    mocker.patch("yc_marketplace_migrations.migration.MigrationsPropagate.extract_number", side_effect=numbers)
    importlib_mock = mocker.patch("yc_marketplace_migrations.migration.importlib")
    importlib_mock.import_module.return_value = MagicMock(spec=[])

    with pytest.raises(MigrationBadConfiguredError):
        assert MigrationsPropagate(log, disk_migrations, query_get, query_set, table_name).disk_migrations


def test_existing_migration_numbers():
    from cloud.marketplace.cli.yc_marketplace_cli import migrations
    migrations_count = 55  # migrations start with "0000" number

    manager = MigrationsPropagate(log=log, migrations=migrations, query_get=None, query_set=None, table_name="",
                                  metadata={})

    assert sorted(map(int, manager.disk_migrations)) == list(range(0, migrations_count))


@pytest.mark.parametrize(
    ["migrations_list", "disk_migrations", "expected_result"],
    [
        ([{"id": "0001_test"}], {"0001": MigrationMock("0001_test")}, {"0001": "0001_test"}),
        ([{"id": "0001_test_2"}], {"0001": MigrationMock("0001_test")}, {}),
        ([{"id": "test_2"}], {"0001": MigrationMock("0001_test")}, {}),
    ],
)
def test_get_db_migrations(mocker, migrations_list, disk_migrations, expected_result):
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)
    mocker.patch.object(MigrationsPropagate, "create_table")

    def fake_query_get(*args):
        return migrations_list

    result = MigrationsPropagate(log, None, fake_query_get, query_set, table_name).db_migrations
    assert result == expected_result


@pytest.mark.parametrize(
    ["db_migrations", "disk_migrations", "expected_result"],
    [
        (
            {"0001": "0001_test"},
            {},
            [],
        ),
        (
            {},
            {"0001": MagicMock},
            [{"migration": MagicMock, "is_applied": False, "number": "0001"}],
        ),
        (
            {"0001": "0001_test", "0003": "0003_test"},
            {"0001": MagicMock, "0002": MagicMock, "0003": MagicMock},
            [
                {"migration": MagicMock, "is_applied": True, "number": "0001"},
                {"migration": MagicMock, "is_applied": False, "number": "0002"},
                {"migration": MagicMock, "is_applied": True, "number": "0003"},
            ],
        ),
        (
            {"0001": "0001_test"},
            {"0001": MagicMock, "0002": MagicMock, "0003": MagicMock},
            [
                {"migration": MagicMock, "is_applied": True, "number": "0001"},
                {"migration": MagicMock, "is_applied": False, "number": "0002"},
                {"migration": MagicMock, "is_applied": False, "number": "0003"},
            ],
        ),
    ],
)
def test_list(mocker, db_migrations, disk_migrations, expected_result):
    mocker.patch.object(MigrationsPropagate, "db_migrations", db_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)

    result = MigrationsPropagate(log, None, query_get, query_set, table_name).list()

    assert result == expected_result


@pytest.mark.parametrize(
    ["get_migration_plan", "execute_calls"],
    [
        ([], 0),
        (["test", "test2"], 2),
    ],
)
def test_migrate(mocker, get_migration_plan, execute_calls):
    mocker.patch.object(MigrationsPropagate, "get_migration_plan", return_value=get_migration_plan)
    mocker.patch.object(MigrationsPropagate, "_execute")
    mocker.patch.object(MigrationsPropagate, "lock")
    mocker.patch.object(MigrationsPropagate, "unlock")

    result = MigrationsPropagate(log, None, query_get, query_set, table_name).migrate()

    assert result is None
    assert MigrationsPropagate._execute.call_count == execute_calls


@pytest.mark.parametrize(
    ["db_migrations", "disk_migrations", "execute_calls"],
    [
        # No migrations in DB, one migration on disk
        (
            {},
            {"0001": MagicMock},
            1,
        ),
        # Same migration in DB and on disk
        (
            {"0001": "0001_test"},
            {"0001": MagicMock},
            0,
        ),
        # One migration in DB and two on disk
        (
            {"0001": "0001_test"},
            {"0001": MagicMock, "0002": MagicMock},
            1,
        ),
    ],
)
def test_get_migration_plan(mocker, db_migrations, disk_migrations, execute_calls):
    mocker.patch.object(MigrationsPropagate, "db_migrations", db_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)

    execute_mock = MagicMock()
    mocker.patch.object(MigrationsPropagate, "_build_migration_plan", return_value=execute_mock)

    result = MigrationsPropagate(log, None, query_get, query_set, table_name).get_migration_plan()
    expected_result = (execute_mock if execute_calls else [])

    assert MigrationsPropagate._build_migration_plan.call_count == execute_calls
    assert result == expected_result


@pytest.mark.parametrize(
    ["target", "db_migrations", "disk_migrations"],
    [
        ("0003", {"0001": "0001_test"}, {"0001": MagicMock, "0002": MagicMock}),
    ],
)
def test_get_migration_plan_not_found(mocker, target, db_migrations, disk_migrations):
    mocker.patch.object(MigrationsPropagate, "db_migrations", db_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)
    with pytest.raises(MigrationPathNotFound):
        MigrationsPropagate(log, None, query_get, query_set, table_name).get_migration_plan(target)


@pytest.mark.parametrize(
    ["db_migrations", "disk_migrations"],
    [
        # No migrations on disk, one migration in DB
        ({"0001": "0001_test"}, {}),
        # One migration on disk, two migrations in DB
        ({"0001": "0001_test", "0002": "00002_test"}, {"0001": MagicMock}),
        # Two migrations on disk, three migrations in DB
        ({"0001": "0001_test", "0002": "00002_test", "0003": "0003_test"}, {"0001": MagicMock, "0003": MagicMock}),
    ],
)
def test_get_migration_plan_not_on_disk(mocker, db_migrations, disk_migrations):
    mocker.patch.object(MigrationsPropagate, "db_migrations", db_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)

    with pytest.raises(MigrationBadConfiguredError):
        MigrationsPropagate(log, None, query_get, query_set, table_name).get_migration_plan()


@pytest.mark.parametrize(
    ["target", "disk_migrations", "call_count"],
    [
        (None, {"0001": MagicMock}, 1),
        ("0001", {"0001": MagicMock}, 1),
        ("0001", {"0001": MagicMock, "0002": MagicMock}, 1),
        ("0001", {"0001": MagicMock, "0002": MagicMock, "0003": MagicMock}, 1),
    ],
)
def test_get_rollback_plan(mocker, target, disk_migrations, call_count):
    mocker.patch.object(MigrationsPropagate, "db_migrations", disk_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)
    execute_mock = MagicMock()
    mocker.patch.object(MigrationsPropagate, "_build_rollback_plan", return_value=execute_mock)

    MigrationsPropagate(log, None, query_get, query_set, table_name).get_rollback_plan(target)

    assert MigrationsPropagate._build_rollback_plan.call_count == call_count


@pytest.mark.parametrize(
    ["target", "disk_migrations"],
    [
        ("0000", {"0001": MagicMock}),
        ("0002", {"0001": MagicMock}),
        ("0000", {"0001": MagicMock, "0002": MagicMock}),
        ("0004", {"0001": MagicMock, "0002": MagicMock, "0003": MagicMock}),
    ],
)
def test_get_rollback_plan_not_on_disk(mocker, target, disk_migrations):
    mocker.patch.object(MigrationsPropagate, "db_migrations", disk_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)
    execute_mock = MagicMock()
    mocker.patch.object(MigrationsPropagate, "_build_rollback_plan", return_value=execute_mock)

    with pytest.raises(MigrationPathNotFound):
        MigrationsPropagate(log, None, query_get, query_set, table_name).get_rollback_plan(target)


@pytest.mark.parametrize(
    ["disk_migrations", "db_migrations"],
    [
        ({"0001": MagicMock}, {"0001": "0001_test", "0002": "0002_test"}),
        ({"0001": MagicMock}, {"0000": "0000_test"}),
        ({"0000": MagicMock, "0001": MagicMock}, {"0000": "0000_test"}),
    ],
)
def test_get_rollback_plan_not_migrated(mocker, disk_migrations, db_migrations):
    mocker.patch.object(MigrationsPropagate, "db_migrations", db_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)
    execute_mock = MagicMock()
    mocker.patch.object(MigrationsPropagate, "_build_rollback_plan", return_value=execute_mock)

    with pytest.raises(MigrationBadConfiguredError):
        MigrationsPropagate(log, None, query_get, query_set, table_name).get_rollback_plan(None)


@pytest.mark.parametrize(
    ["target", "db_migrations", "disk_migrations", "expected_result"],
    [
        (
            "0001",
            {},
            {},
            [],
        ),
        (
            "0001",
            {"0001": "0001_test", "0002": "0002_test"},
            {"0001": "module_1", "0002": "module_2"},
            [],
        ),
        (
            "0002",
            {"0001": "0001_test"},
            {"0001": "module_1", "0002": "module_2"},
            ["module_2"],
        ),
        (
            "0004",
            {"0001": "0001_test"},
            {"0001": "module_1", "0002": "module_2"},
            ["module_2"],
        ),
    ],
)
def test_build_migration_plan(mocker, target, db_migrations, disk_migrations, expected_result):
    mocker.patch.object(MigrationsPropagate, "db_migrations", db_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)

    result = MigrationsPropagate(log, None, query_get, query_set, table_name)._build_migration_plan(target)

    assert result == expected_result


@pytest.mark.parametrize(
    ["target", "db_migrations", "disk_migrations"],
    [
        (
            "0004",
            {"0001": "0001_test", "0003": "module_3"},
            {"0001": "module_1", "0002": "module_2", "0003": "module_3"},
        ),
    ],
)
def test_build_migration_plan_error(mocker, target, db_migrations, disk_migrations):
    mocker.patch.object(MigrationsPropagate, "db_migrations", db_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)

    with pytest.raises(MigrationPathNotCoincides):
        assert MigrationsPropagate(log, None, query_get, query_set, table_name)._build_migration_plan(target)


@pytest.mark.parametrize(
    ["target", "db_migrations", "disk_migrations", "expected_result"],
    [
        (
            "0001",
            {"0001": "0001_test", "0002": "0002_test"},
            {"0001": "module_1", "0002": "module_2"},
            ["module_2", "module_1"],
        ),
        (
            "0002",
            {"0001": "0001_test", "0002": "0002_test"},
            {"0001": "module_1", "0002": "module_2"},
            ["module_2"],
        ),
        (
            "0002",
            {"0001": "0001_test", "0002": "0002_test", "0003": "0003_test"},
            {"0001": "module_1", "0002": "module_2", "0003": "module_3"},
            ["module_3", "module_2"],
        ),
    ],
)
def test_build_rollback_plan(mocker, target, db_migrations, disk_migrations, expected_result):
    mocker.patch.object(MigrationsPropagate, "db_migrations", db_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)

    result = MigrationsPropagate(log, None, query_get, query_set, table_name)._build_rollback_plan(target)

    assert result == expected_result


@pytest.mark.parametrize(
    ["fake", "execute_calls"],
    [
        (False, 1),
        (True, 0),
    ],
)
def test_execute(mocker, fake, execute_calls):
    def fake_set(*args):
        pass

    migration = MagicMock()
    result = MigrationsPropagate(log, None, query_get, fake_set, table_name)._execute(migration, fake)

    assert result is None
    assert migration.execute.call_count == execute_calls


@pytest.mark.parametrize(
    ["fake", "rollback_calls"],
    [
        (False, 1),
        (True, 0),
    ],
)
def test_rollback(mocker, fake, rollback_calls):
    def fake_set(*args):
        pass

    migration = MagicMock()
    result = MigrationsPropagate(log, None, query_get, fake_set, table_name)._rollback(migration, fake)

    assert result is None
    assert migration.rollback.call_count == rollback_calls


@pytest.mark.parametrize(
    ["db_migrations", "disk_migrations", "is_migrated"],
    [
        (
            {},
            {},
            True,
        ),
        (
            {"0001": "0001_test"},
            {"0001": MagicMock},
            True,
        ),
        (
            {"0001": "0001_test", "0002": "0002_test"},
            {"0001": MagicMock, "0002": MagicMock},
            True,
        ),
        (
            {},
            {"0001": MagicMock},
            False,
        ),
        (
            {"0001": "0001_test"},
            {"0001": MagicMock, "0002": MagicMock},
            False,
        ),
        (
            {"0001": "0001_test", "0002": "0002_test", "0003": "0003_test"},
            {"0001": MagicMock, "0003": MagicMock},
            False,
        ),
    ],
)
def test_is_migrated(mocker, disk_migrations, db_migrations, is_migrated):
    mocker.patch.object(MigrationsPropagate, "db_migrations", db_migrations)
    mocker.patch.object(MigrationsPropagate, "disk_migrations", disk_migrations)

    assert MigrationsPropagate(log, None, query_get, query_set, table_name).is_migrated() == is_migrated


@pytest.mark.parametrize(
    ["db_migrations", "current_applied_version"],
    [
        ({}, None),
        ({"0001": "0001_test"}, "0001"),
        ({"0001": "0001_test", "0002": "0002_test"}, "0002"),
        ({"0003": "0003_test", "0001": "0001_test", "0002": "0002_test"}, "0003"),
    ],
)
def test_current_applied_version(mocker, db_migrations, current_applied_version):
    mocker.patch.object(MigrationsPropagate, "db_migrations", db_migrations)
    migrator = MigrationsPropagate(log, None, query_get, query_set, table_name)

    assert migrator.current_applied_version == current_applied_version
