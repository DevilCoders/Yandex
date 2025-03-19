#!/usr/bin/env python
# -*- coding: utf-8 -*-
from enum import IntEnum
from time import time, sleep

from .process import detect_restart
from .redis_server import get_redis_pids
from .timing import time_left as ttime_left
from .ctl_logging import log


class ResultCode(IntEnum):
    SUCCESS = 0
    ERROR = 1


class CommandResult(object):
    def __init__(self, result_print="OK", result_code=ResultCode.SUCCESS):
        self.result_print = result_print
        self.result_code = result_code

    def __str__(self):
        return self.result_print

    def is_failed(self):
        return self.result_code != ResultCode.SUCCESS


class ErrorResult(CommandResult):
    def __init__(self, result_print="FAILED"):
        super(ErrorResult, self).__init__(result_print=result_print, result_code=ResultCode.ERROR)


class Command(object):
    cmd_name = None

    def __init__(self, config):
        self.config = config


class WaitCommand(Command):
    def call_func(self):  # -> CommandResult
        raise NotImplementedError

    def __call__(self):  # -> CommandResult
        pids = None
        restart_count, timeout, sleep_secs = self.config.get_redisctl_options('restarts', 'timeout', 'sleep')
        start_ts = time()
        command_result = None
        while restart_count > 0 and ttime_left(start_ts, timeout) > 0:
            new_pids = get_redis_pids(self.config)
            restart_count = detect_restart(restart_count, pids, new_pids)
            pids = new_pids

            command_result = self.call_func()
            if not command_result.is_failed():
                break

            sleep(sleep_secs)
            log.debug("restarts left: {}".format(restart_count))
        else:
            time_left = ttime_left(start_ts, timeout)
            result_print = "FAILED: wait command: time left = {}; restarts left = {}; last pids seen = {}".format(
                0 if time_left < 0 else time_left, restart_count, pids
            )
            if command_result:
                result_print += "; last result seen: {}".format(command_result.result_print)
            command_result = CommandResult(result_print=result_print, result_code=ResultCode.ERROR)
        log.debug(command_result.result_print)
        return command_result
