#!/usr/bin/env python
# -*- coding: utf-8 -*-
from utils.command import CommandResult, ResultCode, WaitCommand
from utils.redis_server import get_redis_pids
from utils.state import get_state, RedisStatus
from utils.stop import stop


class StopAndWait(WaitCommand):
    cmd_name = "stop_and_wait"

    def call_func(self):
        if not get_redis_pids(self.config):
            return CommandResult()

        shutdown_rename = self.config.get_redisctl_options('shutdown_rename')
        conn, state = get_state(self.config)
        if state not in (
            RedisStatus.READY_TO_ACCEPT_COMMANDS,
            RedisStatus.LOADING,
            RedisStatus.AOF_WRITING_FAILED_ON_FULL_DISK,
        ):
            return CommandResult(result_print=state.value, result_code=ResultCode.ERROR)

        stop_result = stop(conn, shutdown_rename)
        if stop_result.is_failed():
            return stop_result

        conn, state = get_state(self.config)
        if state == RedisStatus.READY_TO_ACCEPT_COMMANDS:
            return CommandResult(result_print="Redis is unexpectedly up after shutdown", result_code=ResultCode.ERROR)
        return CommandResult()
