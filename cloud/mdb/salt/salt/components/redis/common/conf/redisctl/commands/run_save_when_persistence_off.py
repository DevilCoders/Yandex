#!/usr/bin/env python
# -*- coding: utf-8 -*-
from utils.bgsave import bgsave
from utils.command import Command, CommandResult, ResultCode
from utils.persistence import is_persistence_enabled
from utils.state import get_state, RedisStatus


class RunSaveWhenPersistenceOff(Command):
    cmd_name = "run_save_when_persistence_off"

    def __call__(self):
        bgsave_wait, bgsave_cmd, lastsave_cmd = self.config.get_redisctl_options(
            'bgsave_wait', 'bgsave_rename', 'lastsave_rename'
        )
        conn, state = get_state(self.config)
        if state != RedisStatus.READY_TO_ACCEPT_COMMANDS:
            message = 'Redis is not ready for accept commands, current state: {}'.format(state)
            return CommandResult(result_print=message, result_code=ResultCode.ERROR)

        is_pers_on = is_persistence_enabled(self.config)
        if is_pers_on:
            return CommandResult(result_print="AOF or RDB is on, skipping bgsave")

        command_result = bgsave(conn, bgsave_cmd, lastsave_cmd, bgsave_wait)
        if command_result.is_failed():
            return command_result

        return CommandResult()
