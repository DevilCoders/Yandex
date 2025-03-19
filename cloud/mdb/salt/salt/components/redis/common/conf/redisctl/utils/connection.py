#!/usr/bin/env python
# -*- coding: utf-8 -*-
from redis import Redis
from redis.exceptions import AuthenticationError, ResponseError

from .config import _get_config_option_from_files
from .ctl_logging import log
from .exception import get_msg


def _ping(*args, **kwargs):
    conn = None
    exc = None
    try:
        conn = Redis(*args, **kwargs)
        conn.ping()
    except Exception as caught:
        exc = caught
    return conn, exc


def _get_conn(port=None, password=None, host='localhost', tls=False, ssl_ca_certs=None, configs=None):
    """
    Get connection with redis.
    If auth fails with supplied password
    we fall back to requirepass value from config.
    This is dirty but allows password change via pillar.
    """
    conn, exc = _ping(host=host, port=port, password=password, ssl=tls, ssl_ca_certs=ssl_ca_certs)
    auth_failed = (
        isinstance(exc, AuthenticationError)
        or isinstance(exc, ResponseError)
        and 'WRONGPASS invalid username-password pair or user is disabled' in get_msg(exc)
    )
    if auth_failed:
        log.warning('Connection with initially supplied password failed: %s', exc)
        # Actual password resides in /etc/redis/redis.conf
        fallback_password = _get_config_option_from_files('requirepass', config_path=configs)
        conn, exc = _ping(host='localhost', port=port, password=fallback_password, ssl=tls, ssl_ca_certs=ssl_ca_certs)
    return conn, exc


def get_redis_conn(config, host='localhost'):
    port, password, redis_configs = config.get_redisctl_options('redis_port', 'password', 'redis_configs')
    return _get_conn(port=port, password=password, host=host, configs=redis_configs)
