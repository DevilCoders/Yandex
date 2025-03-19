#!/usr/bin/env python
# -*- coding: utf-8 -*-
import enum

from redis.exceptions import BusyLoadingError, ConnectionError, ResponseError

from .connection import get_redis_conn
from .ctl_logging import log
from .exception import get_msg


class RedisStatus(enum.Enum):
    LOADING = "Redis is loading DB in memory from RDB or AOF"
    READY_TO_ACCEPT_COMMANDS = "Redis is ready to accept connections (PING -> PONG)"
    CONNECTION_REFUSED = "Connection to Redis refused (mostly expected down)"
    UNDEFINED = "Something unexpected happened, see log"
    AOF_WRITING_FAILED_ON_FULL_DISK = "Writing to AOF failed as disk is full"


def get_state(config, host='localhost'):
    conn, exc = get_redis_conn(config, host=host)
    if exc is None:
        return conn, RedisStatus.READY_TO_ACCEPT_COMMANDS
    # ping gives error, but connection could be useful anyway - we can execute SHUTDOWN, e.g.
    if isinstance(exc, BusyLoadingError):
        log.debug(RedisStatus.LOADING.value)
        return conn, RedisStatus.LOADING
    if isinstance(exc, ResponseError):
        if "MISCONF Errors writing to the AOF file: No space left on device" in get_msg(exc):
            return conn, RedisStatus.AOF_WRITING_FAILED_ON_FULL_DISK
        log.debug("%s: %s", exc.__class__, get_msg(exc))
        return conn, RedisStatus.UNDEFINED
    if isinstance(exc, ConnectionError):
        log.debug(RedisStatus.CONNECTION_REFUSED.value)
        return None, RedisStatus.CONNECTION_REFUSED

    log.debug("%s: %s", exc.__class__, get_msg(exc))
    return conn, RedisStatus.UNDEFINED
