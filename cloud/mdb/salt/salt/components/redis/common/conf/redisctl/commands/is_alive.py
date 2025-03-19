#!/usr/bin/env python
# -*- coding: utf-8 -*-
from utils.command import Command, CommandResult, ResultCode
from utils.state import get_state, RedisStatus


class IsAlive(Command):
    cmd_name = "is_alive"

    def __call__(self):
        conn, state = get_state(self.config)
        result_code = ResultCode.SUCCESS if state == RedisStatus.READY_TO_ACCEPT_COMMANDS else ResultCode.ERROR
        return CommandResult(result_print=state.value, result_code=result_code)
