#!/usr/bin/env python
# -*- coding: utf-8 -*-
from time import time, sleep
from redis.exceptions import ResponseError, ConnectionError
from .command import CommandResult, ResultCode

from .ctl_logging import log

LASTSAVE_WAIT_SLEEP_TIME_SEC = 2
BGSAVE_WAIT_SLEEP_TIME_SEC = 2


def bgsave(conn, bgsave_cmd, lastsave_cmd, bgsave_wait):
    lastsave_timestamp, error = lastsave(conn, lastsave_cmd)
    if error:
        return CommandResult(result_print=error, result_code=ResultCode.ERROR)

    start_time = time()
    wait_till = start_time + int(bgsave_wait)

    # bgsave can complete faster than 1 second
    # this may cause lastsave to return the same value as the previous
    # sleep can prevent this case
    sleep(1)

    result = call_bgsave(conn, wait_till, bgsave_cmd)
    if result.is_failed():
        return result

    result = wait_bgsave_finish(conn, wait_till, lastsave_cmd, lastsave_timestamp)
    if result.is_failed():
        return result

    elapsed_time = time() - start_time
    log.debug('elapsed_time time %s sec', elapsed_time)

    return CommandResult()


def lastsave(conn, lastsave_cmd):
    """
    returns timestamp of last bgsave or error
    :param conn:
    :param lastsave_cmd:
    :return: int,string - timestamp, error
    """
    try:
        tmp_stamp = conn.execute_command(lastsave_cmd)
        log.debug("lastsave_cmd result: %s", tmp_stamp)

    except (ConnectionError, ResponseError) as err:
        log.debug("lastsave_cmd failed: %s", err)
        return 0, str(err)
    return tmp_stamp, ''


def call_bgsave(conn, wait_till, bgsave_cmd, sleeptime=BGSAVE_WAIT_SLEEP_TIME_SEC):
    """
    tries to call bgsave in redis, return err or empty string on success
    :param conn:
    :param wait_till:
    :param bgsave_cmd:
    :param sleeptime: time in secs to sleep between attempts
    :return:
    """
    failed_counter = 0
    max_counter = 3

    while True:
        if time() >= wait_till:
            error_msg = 'Waiting for bgsave success call took too long'
            return CommandResult(result_print=error_msg, result_code=ResultCode.ERROR)

        try:
            res = conn.execute_command(bgsave_cmd)
            log.debug("bgsave result: %s", res)
            break
        except (ConnectionError, ResponseError) as err:
            if "already in progress" in str(err):
                log.debug("bgsave already in progress")
                break
            failed_counter += 1
            log.debug("bgsave failed [%d/%d]: %s", failed_counter, max_counter, err)
            if failed_counter >= 3:
                return CommandResult(result_print=str(err), result_code=ResultCode.ERROR)
            sleep(sleeptime)
    return CommandResult()


def wait_bgsave_finish(conn, deadline, lastsave_cmd, lastsave_timestamp, sleeptime=LASTSAVE_WAIT_SLEEP_TIME_SEC):
    """
    calls lastsave in while, waiting for bgsave complete - timestamp will change
    :param conn:
    :param deadline:
    :param lastsave_cmd:
    :param lastsave_timestamp: previous timestamp, and we wait for this timestamp change
    :param sleeptime: time in secs to sleep between attempts
    :return:
    """
    while True:
        if time() >= deadline:
            deadline_error = 'Waiting for bgsave to complete took too long'
            log.debug("lastsave_cmd: %s", deadline_error)
            return CommandResult(result_print=deadline_error, result_code=ResultCode.ERROR)

        tmp_stamp, error = lastsave(conn, lastsave_cmd)
        if error:
            return CommandResult(result_print=error, result_code=ResultCode.ERROR)

        if lastsave_timestamp != tmp_stamp:
            log.debug("lastsave_cmd result: %s | %s", lastsave_timestamp, tmp_stamp)
            break

        sleep(sleeptime)

    return CommandResult()
