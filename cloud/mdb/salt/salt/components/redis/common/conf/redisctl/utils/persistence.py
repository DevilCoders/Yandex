#!/usr/bin/env python
# -*- coding: utf-8 -*-
from common.functions import get_config_option_getter


def is_aof_enabled(config, options_getter=None):
    options_getter = options_getter or get_config_option_getter(config)
    appendonly = options_getter('appendonly')
    return appendonly == 'yes'


def is_rdb_enabled(config, options_getter=None):
    options_getter = options_getter or get_config_option_getter(config)
    save = options_getter('save')
    return save


def is_persistence_enabled(config):
    options_getter = get_config_option_getter(config)
    return is_aof_enabled(config, options_getter) or is_rdb_enabled(config, options_getter)
