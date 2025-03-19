import abc
import argparse
import os
from typing import List
from typing import Union

from yc_common import config
from yc_common import logging


class BaseCommand(abc.ABC):
    """
    The base class for all commands.

    1. `add_arguments` - used for add custom arguments
    2. `handle` - main method for command

    """
    description = ""
    add_base_arguments = True
    default_verbosity = 1

    VERBOSITY_LEVEL_MAP = {
        0: logging.WARNING,
        1: logging.INFO,
        2: logging.DEBUG,
    }
    CONFIG_PATH = os.getenv("YC_CONFIG_PATH", "/etc/yc-marketplace/populate-kikimr-config.yaml")

    def _create_parser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(description=self.description or None)

        if self.add_base_arguments:
            parser.add_argument(
                "--devel",
                help="Develop mode.",
                action="store_true", default=False,
            )

            parser.add_argument(
                "-v", "--verbosity",
                help="Verbosity level (0 = warning level, 1 = info level, 2 = debug level).",
                action="store", dest="verbosity", default=self.default_verbosity,
                type=int, choices=[0, 1, 2],
            )

            parser.add_argument(
                "--config",
                help="Custom path to the configuration file.",
            )

        self.add_arguments(parser)

        return parser

    def add_arguments(self, parser: argparse.ArgumentParser):
        """
        Add custom arguments.
        """
        pass

    @abc.abstractmethod
    def handle(self, **kwargs):
        """
        Command body.
        """
        pass

    @classmethod
    def execute(cls, argv: List[str]):
        command = cls()

        parser = command._create_parser()
        options = parser.parse_args(argv)

        command.set_environment(options.devel, options.verbosity, options.config)

        command.handle(**vars(options))

    @classmethod
    def set_environment(cls, devel: bool, verbosity: int, config_path: Union[str, None]):
        if config_path is None:
            config_path = cls.CONFIG_PATH
        config.load([config_path], ignore_unknown=True)

        logging.setup(
            devel_mode=devel,
            level=cls.VERBOSITY_LEVEL_MAP.get(verbosity, cls.default_verbosity),
        )
