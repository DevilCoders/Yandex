from yc_common import config
from yc_common import logging
from yc_common.exceptions import Error
from cloud.marketplace.cli.yc_marketplace_cli import migrations
from cloud.marketplace.common.yc_marketplace_common.db.models import DB_LIST_BY_NAME
from cloud.marketplace.common.yc_marketplace_common.db.models import categories_table
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.lib import TaskUtils
from cloud.marketplace.cli.yc_marketplace_cli.command import BaseCommand
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from yc_marketplace_migrations.migration import MigrationsPropagate

log = logging.get_logger(__name__)


class Command(BaseCommand):
    description = "Populates marketplace database."

    def add_arguments(self, parser):
        parser.add_argument(
            "--erase", action="store_true", default=False,
            help="Delete all tables before creation.",
        )

        parser.add_argument(
            "--fill-all", action="store_true", default=False,
            help="Add all basic data.",
        )

        parser.add_argument(
            "--fill-tasks", action="store_true", default=False,
            help="Add periodic tasks.",
        )

    def handle(self, erase: bool, fill_all: bool, fill_tasks: bool, devel: bool, **kwargs):
        if erase and not devel:
            raise Error("Database erasing is available only in develop mode.")

        self.populate_kikimr(erase)

        if fill_all:
            self.fill_kikimr()
        elif fill_tasks:
            self.fill_queue_tasks()

        log.debug("%s", config.get_value("endpoints.kikimr.marketplace.root"))

    @classmethod
    def populate_kikimr(cls, erase: bool):
        tables = DB_LIST_BY_NAME.values()

        if erase:
            for table in tables:
                table.drop(only_if_exists=True)

        for table in tables:
            table.create()

    @classmethod
    def fill_kikimr(cls):
        # add default category
        category = Category.new("all products", Category.Type.PUBLIC, 0, None)
        category.id = config.get_value("marketplace.default_os_product_category")

        with marketplace_db().with_table_scope(categories_table).transaction() as tx:
            tx.insert_object("INSERT INTO $table", category)
            categories = {
                "publisher": Category.Type.PUBLISHER,
                "var": Category.Type.VAR,
                "isv": Category.Type.ISV,
            }
            for partner_type in categories:
                category = Category.new("All {}s".format(partner_type), categories[partner_type], 0, None)
                category.id = config.get_value("marketplace.default_{}_category".format(partner_type))
                tx.insert_object("UPSERT INTO $table", category)

        cls.fill_queue_tasks()

        # add previous migrations
        MigrationsPropagate(log, migrations=migrations, query_get=marketplace_db().select,
                            query_set=marketplace_db().query).migrate(fake=True)

    @classmethod
    def fill_queue_tasks(cls):
        with marketplace_db().transaction() as tx:
            TaskUtils.create(
                operation_type="resolve_dependencies",
                group_id=None,
                params={
                    "timeout": 60,
                },
                is_infinite=True,
                tx=tx,
            )

            TaskUtils.create(
                operation_type="export_tables",
                group_id=None,
                params={
                    "timeout": 60 * 60 * 4,
                    "export_destination": "logbroker",
                },
                is_infinite=True,
                tx=tx,
            )
