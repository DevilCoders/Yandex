"""Aux functions for configs"""

import argparse
from typing import List
import yaml

from .exceptions import BootstrapError
from .utils import merge_dicts


class BootstrapConfigMixin:
    _CONFIG_FILE = None
    _CONFIG_ARGPARSE_NAME = "config"
    _SECRET_CONFIG_FILE = None
    _SECRET_CONFIG_ARGPARSE_NAME = "secret-config"

    @classmethod
    def from_configs(cls, config_files: List[str]) -> "BootstrapConfigMixin":
        """Create db configs from list for config files (futher configs of highers priority)"""
        config_as_dict = {}

        for config_file in config_files:
            with open(config_file) as f:
                try:
                    config_as_dict = merge_dicts(config_as_dict, yaml.load(f, Loader=yaml.SafeLoader))
                except (yaml.parser.ParserError, yaml.scanner.ScannerError) as e:
                    fmtmsg = "Failed to parse config <{}>. Yaml Error:\n{}"
                    raise BootstrapError(fmtmsg.format(config_file, str(e))) from None

        return cls(config_as_dict)

    @classmethod
    def from_argparse(cls, options: argparse.Namespace) -> "AppConfig":
        options_config = getattr(options, cls._CONFIG_ARGPARSE_NAME.replace("-", "_"))
        options_secret_config = getattr(options, cls._SECRET_CONFIG_ARGPARSE_NAME.replace("-", "_"))

        if (options_config != cls._CONFIG_FILE) and (options_secret_config == cls._SECRET_CONFIG_FILE):
            # do not use default secret config if using non-default normal config
            config_files = [options_config]
        else:
            config_files = [options_config, options_secret_config]

        return cls.from_configs(config_files)

    @classmethod
    def update_parser(cls, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("--{}".format(cls._CONFIG_ARGPARSE_NAME), metavar="FILE", type=str,
                            default=cls._CONFIG_FILE, help="Config file")
        parser.add_argument("--{}".format(cls._SECRET_CONFIG_ARGPARSE_NAME), metavar="FILE", type=str,
                            default=cls._SECRET_CONFIG_FILE, help="Secret config file")
