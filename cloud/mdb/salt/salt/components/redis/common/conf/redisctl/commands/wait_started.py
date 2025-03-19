#!/usr/bin/env python
# -*- coding: utf-8 -*-
from utils.command import CommandResult, ResultCode, WaitCommand
from utils.ctl_logging import log
from utils.state import get_state, RedisStatus


class WaitStarted(WaitCommand):
    cmd_name = "wait_started"

    def call_func(self):
        conn, state = get_state(self.config)
        if state == RedisStatus.READY_TO_ACCEPT_COMMANDS:
            return CommandResult(result_print=state.value)
        log.debug("in progress...")
        return CommandResult(result_print="wait started", result_code=ResultCode.ERROR)
