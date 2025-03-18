# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import yenv

from . import data


def get_config(config_name):
    if hasattr(data, config_name):
        config = getattr(data, config_name)

        try:
            return get_config_data(yenv.choose_key_by_type(config,
                                                           fallback=True))
        except ValueError:
            raise KeyError('There is no config for the current env.')

    else:
        raise KeyError('Unknown name of the configuration.')


def get_config_data(instance_config):
    for parameter, value in instance_config.items():
        if isinstance(value, dict):
            instance_config[parameter] = yenv.choose_key_by_name(value)
    return instance_config
