import sys

from cloud.marketplace.cli.yc_marketplace_cli import migrations
from cloud.marketplace.cli.yc_marketplace_cli.command import BaseCommand
from cloud.marketplace.cli.yc_marketplace_cli.types import ArgumentType
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from yc_common import logging
from yc_marketplace_migrations.migration import MigrationsPropagate

log = logging.get_logger(__name__)


class Command(BaseCommand):
    def add_arguments(self, parser):
        parser.add_argument(
            "target", nargs="?", type=ArgumentType.regex(r"^([0-9]{4})|(zero)$"),
            help="Number of target migration.",
        )

        parser.add_argument(
            "--fake", action="store_true", default=False,
            help="Mark migrations as run without actually running them.",
        )

        parser.add_argument(
            "--list", dest="list_migrations", action="store_true", default=False,
            help="Print migrations status.",
        )

    def handle(self, target: str, fake: bool, list_migrations: bool, **kwargs):
        # manager = MktPropagate()
        manager = MigrationsPropagate(log, migrations, marketplace_db().select, marketplace_db().query, "migration")

        if list_migrations:
            self._list_migrations(manager)
            return

        err = manager.migrate(target, fake)
        if isinstance(err, BaseException):
            raise err

    def _list_migrations(self, manager: MigrationsPropagate):
        output = ""

        for migration_info in manager.list():
            output += "({})  ".format("*" if migration_info["is_applied"] else " ")
            output += migration_info["migration"].name
            output += "\n"

        sys.stdout.write(output)
