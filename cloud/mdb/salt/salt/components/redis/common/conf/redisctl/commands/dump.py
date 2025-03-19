#!/usr/bin/env python
# -*- coding: utf-8 -*-

from os import path

from utils.command import Command, CommandResult, ResultCode
from utils.state import get_state, RedisStatus
from utils.bgsave import bgsave
from utils.file_ops import copy_file, STDOUT_PATH


LASTSAVE_WAIT_SLEEP_TIME = 2  # sec


class Dump(Command):
    cmd_name = "dump"

    def __call__(self):
        file, bgsave_wait, bgsave_cmd, lastsave_cmd, rdb_full_path = self.config.get_redisctl_options(
            'file', 'bgsave_wait', 'bgsave_rename', 'lastsave_rename', 'rdb_full_path'
        )
        if not file:
            return CommandResult(result_print="Option --file is not provided", result_code=ResultCode.ERROR)

        write_to_stdout = file in ['-', STDOUT_PATH]
        if not write_to_stdout and path.exists(file):
            return CommandResult(result_print="File {} does already exists".format(file), result_code=ResultCode.ERROR)

        conn, state = get_state(self.config)
        if state != RedisStatus.READY_TO_ACCEPT_COMMANDS:
            message = 'Redis is not ready for accept commands, current state: {}'.format(state)
            return CommandResult(result_print=message, result_code=ResultCode.ERROR)

        command_result = bgsave(conn, bgsave_cmd, lastsave_cmd, bgsave_wait)
        if command_result.is_failed():
            return command_result

        command_result = copy_file(rdb_full_path, file)
        if command_result.is_failed():
            return command_result

        if write_to_stdout:
            return CommandResult(result_print='')

        return CommandResult()
