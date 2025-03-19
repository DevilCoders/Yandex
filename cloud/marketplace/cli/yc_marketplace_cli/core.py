import argparse
from importlib import util
import sys
from typing import List
from typing import Type

from yc_common import logging
from cloud.marketplace.cli.yc_marketplace_cli.command import BaseCommand

log = logging.get_logger(__name__)


class ManagementCommand:
    """
    Encapsulate the logic of `yc-marketplace-manage`.

    """

    APPS = {
        "cloud.marketplace.cli.yc_marketplace_cli",
    }

    subcommands = {}

    def __init__(self):
        subcommands_by_app = {}

        for app_name in self.APPS:
            app_spec = util.find_spec(app_name)

            if app_spec is not None:
                subcommands = self.get_subcommands()

                if subcommands:
                    subcommands_by_app[app_name] = set(subcommands)

        self.subcommands = subcommands_by_app

    @staticmethod
    def get_subcommands() -> List[str]:
        subcommands = ["drop_db", "migrate", "populate_db"]

        return subcommands

    @staticmethod
    def execute(argv: List[str]):
        manager = ManagementCommand()

        if len(argv) <= 1:
            manager._print_subcommands()
            return

        subcommand = manager.validate_subcommand(argv[1])
        subcommand.execute(argv[2:])

    def _print_subcommands(self):
        help_text = "Available commands:\n"

        for app_name, subcommands in self.subcommands.items():
            help_text += "\n"
            help_text += app_name + "\n"
            help_text += "\n".join("    {}".format(c) for c in sorted(subcommands))
            help_text += "\n"

        sys.stdout.write(help_text + "\n")

    def validate_subcommand(self, subcommand: str) -> Type[BaseCommand]:
        for app_name, subcommands in self.subcommands.items():
            if subcommand in subcommands:
                command_spec = util.find_spec("{}.commands.{}".format(app_name, subcommand))

                if command_spec is not None:
                    command_module = util.module_from_spec(command_spec)
                    command_spec.loader.exec_module(command_module)

                    return command_module.Command

        raise argparse.ArgumentError(None, "Unexpected command `{}`.".format(subcommand))
