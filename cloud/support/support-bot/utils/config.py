#!/usr/bin/env python3
"""This module contains Config class."""

import os
import yaml
import logging

from core.error import ConfigError
from core.constants import (DEFAULT_LOG_BACKUP_COUNT, DEFAULT_MAX_LOGFILE_SIZE,
                            RESPS_BASE_URL, STAFF_BASE_URL, STARTREK_BASE_URL, SK_BASE_URL,
                            ABC2_BASE_URL, BLACKBOX_BASE_URL, DEFAULT_SCHEDULE_ID)

homedir = os.path.expanduser('~')
config_file = os.path.join(homedir, '.support-bot', 'config.yaml')  # change to /etc/support_bot.yaml later...


class Config:
    """This object represents a settings for the library."""
    try:
        with open(config_file, 'r') as cfgfile:
            config = yaml.load(cfgfile, Loader=yaml.Loader)
    except FileNotFoundError:
        raise ConfigError('Config file not found')
    except TypeError:
        raise ConfigError('Corrupted config file or bad format')

    # Auth
    TELEGRAM_TOKEN = config.get('telegram', {}).get('token')
    STAFF_TOKEN = config.get('staff', {}).get('token')
    STARTREK_TOKEN = config.get('startrek', {}).get('token')
    RESPS_TOKEN = config.get('resps', {}).get('token')
    TVM_LOCAL_TOKEN = config.get('tvm', {}).get('local_token')

    for token in (TELEGRAM_TOKEN, STAFF_TOKEN, STARTREK_TOKEN, TVM_LOCAL_TOKEN):
        if token is None:
            raise ConfigError(f'Check your config. Not all tokens are specified.')
    
    # Bot / abc2.0
    SCHEDULE_ID = config.get('staff', {}).get('schedule_id') or DEFAULT_SCHEDULE_ID
            
    # Endpoints
    RESPS_ENDPOINT = config.get('resps', {}).get('endpoint') or RESPS_BASE_URL
    STAFF_ENDPOINT = config.get('staff', {}).get('endpoint') or STAFF_BASE_URL
    STARTREK_ENDPOINT = config.get('startrek', {}).get('endpoint') or STARTREK_BASE_URL
    SK_ENDPOINT = config.get('supkeeper', {}).get('endpoint') or SK_BASE_URL
    ABC2_ENDPOINT = config.get('tvm', {}).get('abc2_endpoint') or ABC2_BASE_URL
    BLACKBOX_ENDPOINT = config.get('tvm', {}).get('bb_endpoint') or BLACKBOX_BASE_URL

    # Database
    HOST = config.get('database', {}).get('host')
    PORT = config.get('database', {}).get('port')
    USER = config.get('database', {}).get('user')
    PASSWD = config.get('database', {}).get('passwd')
    DB_NAME = config.get('database', {}).get('db_name')
    CA_PATH = config.get('database', {}).get('ca_path')

    # Proxy
    PROXY_URL = config.get('proxy', {}).get('url')
    PROXY_USER = config.get('proxy', {}).get('user')
    PROXY_PASSWD = config.get('proxy', {}).get('passwd')
    READ_TIMEOUT = config.get('proxy', {}).get('read_timeout')
    CONNECT_TIMEOUT = config.get('proxy', {}).get('connect_timeout')

    # Logs
    LOGPATH = config.get('logs', {}).get('path') or os.path.join('/', 'var', 'log')
    if LOGPATH is not None and not LOGPATH.endswith('.log'):
        LOGPATH = os.path.join(LOGPATH, 'support_bot.log')
    LOGLEVEL = config.get('logs', {}).get('level') or 'info'
    LOGLEVEL = getattr(logging, LOGLEVEL.upper())
    if not isinstance(LOGLEVEL, int):
        raise ValueError('loglevel must be: debug/info/warning/error')
    if LOGLEVEL < 20:  # disable spam logs from startrek if loglevel=info
        logging.getLogger('yandex_tracker_client').setLevel(logging.WARNING)
    MAX_LOGFILE_SIZE = config.get('logs', {}).get('max_size') or DEFAULT_MAX_LOGFILE_SIZE
    LOG_BACKUP_COUNT = config.get('logs', {}).get('backup_count') or DEFAULT_LOG_BACKUP_COUNT
    # Disable log for external modules
    if config.get('logs', {}).get('disable_telegram_debug'):
        logging.getLogger('telegram').setLevel(logging.WARNING)
    if config.get('logs', {}).get('disable_startrek_debug'):
        logging.getLogger('yandex_tracker_client').setLevel(logging.WARNING)
