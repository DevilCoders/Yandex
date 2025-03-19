# -*- coding: utf-8 -*-
"""
DBaaS config parsing functions
"""
from typing import List
import collections.abc
import json
import jsonschema
from types import SimpleNamespace

from .dict import combine_dict


Validator = jsonschema.Draft4Validator


class ConfigValidationError(RuntimeError):
    """
    Common validation error
    """

    pass


class ConfigSyntaxError(RuntimeError):
    """
    Config syntax error
    """


def parse_config_set(config_paths: List[str], schema: dict, default: dict = None) -> SimpleNamespace:
    """
    Load configs, validate and return namespaced dict
    """
    if default is None:
        default = dict()

    config = default.copy()
    for config_path in config_paths:
        with open(config_path) as config_file:
            try:
                config = combine_dict(config, json.load(config_file))
            except ValueError as exc:
                raise ConfigSyntaxError(f"Invalid config '{config_file}' syntax: {exc}")

    validate_config(config, schema)

    namespaces = {}
    for section, options in config.items():
        if isinstance(options, collections.abc.Mapping):
            namespaces[section] = SimpleNamespace(**options)
        else:
            namespaces[section] = options
    return SimpleNamespace(**namespaces)


def parse_config(config_path: str, schema: dict, default: dict = None) -> SimpleNamespace:
    """
    Load config, validate and return namespaced dict
    """
    return parse_config_set([config_path], schema, default)


def validate_config(config: dict, schema: dict):
    validator = Validator(schema)
    config_errors = validator.iter_errors(config)
    report_error = jsonschema.exceptions.best_match(config_errors)
    if report_error is not None:
        msg = 'Malformed config at {path}: {error}'.format(
            path='.'.join(map(str, report_error.absolute_path)),
            error=report_error.message,
        )
        raise ConfigValidationError(msg)
