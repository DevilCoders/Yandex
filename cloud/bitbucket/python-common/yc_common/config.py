"""Configuration file reading."""

import copy

import yaml
import toml

from typing import List, Dict, Tuple, Callable

from yc_common.exceptions import Error
from yc_common.models import ModelValidationError

_CONFIG = None
_UNDEFINED = object()


class MissingConfigOptionError(Error):
    def __init__(self, name):
        super().__init__("Configuration option {!r} is missing.", name)


class ConfigNotLoadedError(Error):
    def __init__(self):
        super().__init__("Configuration file is not loaded yet.")


class ConfigFormat:
    YAML = "yaml"
    TOML = "toml"


def load(config_paths, model=None, ignore_unknown=False, config_format=ConfigFormat.YAML):
    config_path, *defaults_paths = config_paths

    config = load_raw_config(config_path, config_format=config_format)
    for defaults_path in defaults_paths:
        merge_raw_config(config, load_raw_config(defaults_path, config_format=config_format))

    if model is not None:
        try:
            model.from_api(config, ignore_unknown=ignore_unknown)
        except ModelValidationError as e:
            raise Error("Configuration file validation error: {}", e)

    global _CONFIG
    _CONFIG = config


def get_value(name, default=_UNDEFINED, converter=None, model=None):
    value = _get()

    for key in name.split("."):
        if not isinstance(value, dict):
            raise MissingConfigOptionError(name)

        try:
            value = value[key]
        except KeyError:
            if default is _UNDEFINED:
                raise MissingConfigOptionError(name)

            return default

    if converter is not None:
        try:
            value = converter(value)
        except ValueError as e:
            raise Error("{!r} configuration file option validation error: {}", name, e)

    if model is not None:
        try:
            value = model.from_api(value)
        except ModelValidationError as e:
            raise Error("{!r} configuration file option validation error: {}", name, e)

    return value


def devel_mode() -> bool:
    return get_value("devel_mode")


def test_mode() -> bool:
    return get_value("test_mode")


def prod_mode() -> bool:
    return not devel_mode() and not test_mode()


def set_value(name, value):
    config = _get()
    keys = name.split(".")

    for key_id, key in enumerate(keys):
        if not isinstance(config, dict):
            raise Error("Unable to set configuration value {!r}: {!r} is not a dictionary.",
                        name, ".".join(keys[:key_id]) or "[root]")

        if key_id == len(keys) - 1:
            config[key] = value
        else:
            config = config.setdefault(key, {})


def is_loaded():
    return _CONFIG is not None


def _get():
    if not is_loaded():
        raise ConfigNotLoadedError()

    return _CONFIG


def load_raw_config(path, config_format=ConfigFormat.YAML):
    try:
        with open(path) as config_file:
            if config_format == ConfigFormat.YAML:
                config = yaml.safe_load(config_file)
            elif config_format == ConfigFormat.TOML:
                config = toml.load(config_file)
            else:
                raise Error("Invalid config format: {}.", config_format)
    except OSError as e:
        raise Error("Configuration file {!r} reading error: {}.", path, e.strerror)
    except yaml.YAMLError as e:
        raise Error("Configuration file {!r} parsing error: {}.", path, e)

    if config is None:
        config = {}
    elif type(config) is not dict:
        raise Error("Configuration file {!r} parsing error: the root element must be a dictionary.")

    return config


def _try_custom_merger(lvalue, rvalue, path, custom_mergers):
    """Apply custom merger"""
    if path in custom_mergers:
        result = custom_mergers[path](lvalue, rvalue)
        return True, result
    return False, None


def merge_raw_config(config, other, update=False, path: List[str] = None,
                     custom_mergers: Dict[Tuple[str], Callable] = None):
    """
        Merges two configs before building the final model. If update is set to False, uses other config as defaults
        and doesn't overwrite existing values.

        path: path from root
        custom_mergers: special mergers for merging paths
    """
    if not path:
        path = []
    if not custom_mergers:
        custom_mergers = {}

    for key, value in other.items():
        if key in config:
            if isinstance(config[key], dict) and isinstance(value, dict):
                apply, new_value = _try_custom_merger(config[key], value, tuple(path + [key]), custom_mergers)

                if apply:
                    config[copy.deepcopy(key)] = new_value
                else:
                    merge_raw_config(config[key], value, update, path + [key], custom_mergers)

                continue

            if not update:
                continue

        config[copy.deepcopy(key)] = copy.deepcopy(value)
