#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
File to include functions with cross-utils import to avoid circular dependencies
"""
from utils.state import get_state, RedisStatus
from utils.config import _get_config_option_from_files, _get_config_option_from_connection, _set_config_option


def get_config_option_getter(config):
    conn, state = get_state(config)
    config_rename, redis_configs = config.get_redisctl_options('config_rename', 'redis_configs')

    def from_file(*args, **kwargs):
        return _get_config_option_from_files(*args, config_path=redis_configs, **kwargs)

    def from_conn(opt):
        return _get_config_option_from_connection(conn, config_rename, pattern=opt)

    options_getter = from_file if state != RedisStatus.READY_TO_ACCEPT_COMMANDS else from_conn
    return options_getter


def get_config_option_setter(config):
    conn, state = get_state(config)
    if state != RedisStatus.READY_TO_ACCEPT_COMMANDS:
        return None, state
    config_rename = config.get_redisctl_options('config_rename')

    def options_setter(option, value):
        return _set_config_option(conn, config_rename, option, value)

    return options_setter, state
