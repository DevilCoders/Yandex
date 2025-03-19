from yc_common import config
from yc_common import logging
from yc_common.exceptions import Error
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.cli.yc_marketplace_cli.command import BaseCommand

log = logging.get_logger(__name__)


class Command(BaseCommand):
    description = "Drop development database."

    def handle(self, devel: bool, **kwargs):
        if not devel:
            raise Error("Database dropping is available only in develop mode.")

        self._drop_directory_recursively(config.get_value("endpoints.kikimr.marketplace.root"))

        log.debug("Database `%s` dropped!", config.get_value("endpoints.kikimr.marketplace.root"))

    @classmethod
    def _drop_directory_recursively(cls, path: str):
        kikimr_client = marketplace_db()

        for entry in kikimr_client.list_directory(path):
            path_to_drop = "{}/{}".format(path, entry["name"])

            if entry["directory"]:
                cls._drop_directory_recursively(path_to_drop)
            else:
                kikimr_client.query("DROP TABLE [{}]".format(path_to_drop))

        kikimr_client.delete_directory(path)
