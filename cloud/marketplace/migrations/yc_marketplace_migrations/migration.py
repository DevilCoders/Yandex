import importlib
import json
import re
from contextlib import contextmanager
from logging import Logger
from types import ModuleType
from typing import Any
from typing import Callable
from typing import Dict
from typing import List
from typing import Optional
from typing import Union

from yc_marketplace_migrations.base import BaseMigration  # Using for import
from yc_marketplace_migrations.table import MigrationTable
from yc_marketplace_migrations.utils.errors import MigrationBadConfiguredError
from yc_marketplace_migrations.utils.errors import MigrationLockError
from yc_marketplace_migrations.utils.errors import MigrationPathNotCoincides
from yc_marketplace_migrations.utils.errors import MigrationPathNotFound
from yc_marketplace_migrations.utils.helpers import list_migrations_files
from yc_marketplace_migrations.utils.helpers import timestamp


class MigrationsPropagate:
    NUMBER_REGEXP = re.compile(r"^[0-9]{4}")
    LOCK = "lock"

    def __init__(self,
                 log: Logger,
                 migrations: ModuleType,
                 query_get: Callable[[str], List[Dict]],
                 query_set: Callable[[str], Any],
                 table_name: str = "migration",
                 metadata: Dict = None):
        self.migrations = migrations
        self.table_name = table_name
        self.query_get = query_get
        self.query_set = query_set
        self.log = log
        self.metadata = metadata
        self._db_migrations = self._disk_migrations = None

    @classmethod
    def extract_number(cls, name: str) -> Union[str, None]:
        found = cls.NUMBER_REGEXP.search(name)
        if found:
            return found.group(0)

        return None

    @property
    def current_applied_version(self) -> Optional[str]:
        if not self.db_migrations:
            return

        last_applied_version = sorted(self.db_migrations.keys())[-1]

        return last_applied_version

    @property
    def db_migrations(self) -> dict:
        if self._db_migrations is None:
            db_migrations = {}

            self.create_table()

            migrations_all = self.query_get("SELECT * FROM {} ORDER BY id DESC".format(self.table_name))
            for migration in migrations_all:
                migration_name = migration["id"]
                number = self.extract_number(migration_name)

                if number is None:
                    self.log.warning("Migration `%s` without number in database", migration_name)
                    continue

                if number not in self.disk_migrations or self.disk_migrations[number].name != migration_name:
                    self.log.warning("Migration `%s` in database, but not on disk.", migration_name)
                    continue

                db_migrations[number] = migration_name

            self._db_migrations = db_migrations

        return self._db_migrations

    @property
    def disk_migrations(self) -> dict:
        if self._disk_migrations is None:
            disk_migrations = {}

            for migration_name in list_migrations_files(self.migrations):
                number = self.extract_number(migration_name)

                if number is None:
                    continue

                if number in disk_migrations:
                    raise MigrationBadConfiguredError("Multiple migrations found with the `%s` number.", number)

                migration_module = importlib.import_module(
                    "{}.{}".format(self.migrations.__package__, migration_name))
                if not hasattr(migration_module, "Migration"):
                    raise MigrationBadConfiguredError("Migration's file `%s` has no Migration class.",
                                                      migration_name)

                try:
                    disk_migrations[number] = migration_module.Migration()
                except Exception as e:
                    raise MigrationBadConfiguredError("Migration `%s` is poorly configured", migration_name) from e

            self._disk_migrations = disk_migrations

        return self._disk_migrations

    @contextmanager
    def with_lock(self):
        try:
            yield self.lock()
        finally:
            self.unlock()

    def migrate(self, target: str = None, fake: bool = False):
        plan = self.get_migration_plan(target)

        if not plan:
            return

        self.execute_plan(plan=plan, rollback=False, fake=fake)

    def rollback(self, target: str = None, fake: bool = False):
        plan = self.get_rollback_plan(target)

        if not plan:
            return

        self.execute_plan(plan=plan, rollback=True, fake=fake)

    def execute_plan(self, plan: List[BaseMigration], rollback: bool, fake: bool):
        executer = self._rollback if rollback else self._execute

        with self.with_lock():
            for migration in plan:
                executer(migration, fake)

    def is_migrated(self):
        return self.db_migrations.keys() == self.disk_migrations.keys()

    def create_table(self):
        self.query_set(MigrationTable(self.table_name).create(), transactionless=True)

    def list(self) -> List[dict]:
        migrations_list = []
        prev_is_applied = True

        for number in sorted(self.disk_migrations):
            cur_is_applied = number in self.db_migrations

            if not prev_is_applied and cur_is_applied:
                self.log.error("Migrations before `%s` are not in database.", number)

            prev_is_applied = cur_is_applied

            migrations_list.append({
                "number": number,
                "migration": self.disk_migrations[number],
                "is_applied": cur_is_applied,
            })

        return migrations_list

    def lock(self):
        query = "INSERT INTO {} (id, created_at) VALUES (?, ?)".format(self.table_name)
        try:
            self.query_set(query, self.LOCK, timestamp())
        except Exception as e:
            raise MigrationLockError(e)

    def unlock(self):
        query = "DELETE FROM {} WHERE id = ?".format(self.table_name)
        try:
            self.query_set(query, self.LOCK)
        except Exception as e:
            raise MigrationLockError(e)

    def get_missing_db_migrations(self):
        """
            We can apply migrations iff *every* migration in DB is present on disk.
            Every other case we should fail fast and not try to rollback.

            Examples when we should fail fast:

            1) DB has bigger version:
            Disk: 0 1 2 3
            DB: 0 1 2 3 *4*

            2) Disk has "holes":
            Disk: 0 1 3 4
            DB: 0 1 *2* 3
        """
        return set(self.db_migrations) - set(self.disk_migrations)

    def get_missing_disk_migrations(self):
        return set(self.disk_migrations) - set(self.db_migrations)

    def is_target_on_disk(self, target: str):
        if target is None:
            return True

        return target in self.disk_migrations

    def get_migration_plan(self, target: str = None) -> List[BaseMigration]:
        missing_from_db = self.get_missing_db_migrations()
        if missing_from_db:
            raise MigrationBadConfiguredError("Migrations from DB: %s are missing on disk", missing_from_db)

        missing_in_db = self.get_missing_disk_migrations()
        if not missing_in_db:
            return []

        if not self.is_target_on_disk(target):
            raise MigrationPathNotFound("Target migration `%s` is not found on disk.", target)

        return self._build_migration_plan(target)

    def get_rollback_plan(self, target: str) -> List[BaseMigration]:
        """
            We can only rollback if all migrations were applied and target migration is on disk.
        """
        if not self.is_migrated():
            raise MigrationBadConfiguredError("Not all migration were applied can't rollback")

        if not self.is_target_on_disk(target):
            raise MigrationPathNotFound("Target migration `%s` is not found on disk.", target)

        return self._build_rollback_plan(target)

    def _build_rollback_plan(self, target: str = None):
        plan = []

        for number in sorted(self.disk_migrations):
            if target is not None and number < target:
                continue

            plan.append(self.disk_migrations[number])

        return list(reversed(plan))

    def _build_migration_plan(self, target: str = None) -> List[BaseMigration]:
        plan = []
        prev_is_applied = True

        for number in sorted(self.disk_migrations):
            if target is not None and number > target:
                break

            cur_is_applied = number in self.db_migrations

            if not prev_is_applied and cur_is_applied:
                raise MigrationPathNotCoincides("Migration `%s` is already in database.", number)

            prev_is_applied = cur_is_applied

            if not cur_is_applied:
                plan.append(self.disk_migrations[number])

        return plan

    def _execute(self, migration: BaseMigration, fake: bool):
        if not fake:
            self.log.debug("Execute migration %s...", migration.name)
            migration.execute()

        self.log.debug("Insert %s to migration table...", migration.name)
        self.query_set("UPSERT INTO {} (id, created_at, metadata) VALUES (?, ?, ?)".format(
            self.table_name), migration.name, timestamp(), json.dumps(self.metadata))

    def _rollback(self, migration: BaseMigration, fake: bool):
        if not fake:
            self.log.debug("Rollback migration %s...", migration.name)
            migration.rollback()

        self.log.debug("Delete %s from migration table...", migration.name)
        self.query_set("DELETE FROM {} WHERE id=?".format(self.table_name), migration.name)
