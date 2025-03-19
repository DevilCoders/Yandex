#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os.path
import os
import re

from common.functions import get_config_option_getter, get_config_option_setter
from utils.bgsave import bgsave
from utils.file_ops import (
    is_disk_full,
    is_file_large,
    remove_file,
    store_to_file,
    get_from_file,
    get_total_size,
    get_file_size,
)
from utils.command import Command, CommandResult, ResultCode
from utils.persistence import is_aof_enabled
from utils.redis_server import is_master
from utils.state import get_state, RedisStatus
from utils.ctl_logging import log


SAVE_STATE_FILE = '/var/tmp/redisctl.save.state'
REDIS_VERSION_WITH_SPLITTED_AOF = 7
AOF_FILENAME_BASE_PATTERN = re.compile(r'appendonly.aof.\d+.base.rdb')
AOF_FILENAME_AOF_PATTERN = re.compile(r'appendonly.aof.\d+.incr.aof')

# This file name still can be seen for a while after upgrading Redis from older versions
AOF_FILENAME_PRE_7 = re.compile(r'appendonly.aof')


def restore_save(options_setter):
    save_before = get_from_file(SAVE_STATE_FILE)
    if save_before is None:
        return CommandResult(
            result_print="Failed to restore save as state file doesn't exists", result_code=ResultCode.ERROR
        )
    set_result, is_ok = options_setter('save', save_before)
    if not is_ok:
        return CommandResult(
            result_print="Failed to set save config option after: {}".format(set_result), result_code=ResultCode.ERROR
        )
    log.debug("set save to '{}', deleting {}".format(save_before, SAVE_STATE_FILE))
    result = remove_file(SAVE_STATE_FILE)
    return result if result.is_failed() else CommandResult()


def is_aof_large(aof_full_path_pre_7, aof_dir_path, redis_version_major, aof_limit):
    if redis_version_major < REDIS_VERSION_WITH_SPLITTED_AOF:
        return is_file_large(aof_full_path_pre_7, aof_limit)
    else:
        total_size = get_total_size(aof_dir_path)
        files_size = sum(
            [
                get_file_size(os.path.join(aof_dir_path, filename))
                for filename in os.listdir(aof_dir_path)
                if (
                    AOF_FILENAME_BASE_PATTERN.match(filename)
                    or AOF_FILENAME_AOF_PATTERN.match(filename)
                    or AOF_FILENAME_PRE_7.match(filename)
                )
            ]
        )
        aof_usage = 100 * (files_size / total_size)
        return aof_usage >= aof_limit, aof_usage


def remove_aof(aof_full_path_pre_7, aof_dir_path, redis_version_major):
    if redis_version_major < REDIS_VERSION_WITH_SPLITTED_AOF:
        result = remove_file(aof_full_path_pre_7)
        if result.is_failed():
            return result
    else:
        for filename in os.listdir(aof_dir_path):
            if (
                AOF_FILENAME_BASE_PATTERN.match(filename)
                or AOF_FILENAME_AOF_PATTERN.match(filename)
                or AOF_FILENAME_PRE_7.match(filename)
            ):
                result = remove_file(os.path.join(aof_dir_path, filename))
                if result.is_failed():
                    return result
    return CommandResult()


class RemoveAof(Command):
    cmd_name = "remove_aof"

    def __call__(self, *args, **kwargs):
        (
            redis_version_major,
            aof_dir_path,
            aof_full_path_pre_7,
            disk_limit,
            aof_limit,
            force,
            dry_run,
            bgsave_wait,
            bgsave_rename,
            lastsave_rename,
            bgrewriteaof_rename,
        ) = self.config.get_redisctl_options(
            'redis_version_major',
            'aof_dir_path',
            'aof_full_path',
            'disk_limit',
            'aof_limit',
            'force',
            'dry_run',
            'bgsave_wait',
            'bgsave_rename',
            'lastsave_rename',
            'bgrewriteaof_rename',
        )

        result = self.prechecks(aof_full_path_pre_7, aof_dir_path, redis_version_major, disk_limit, aof_limit, force)
        if result:
            return result

        options_getter = get_config_option_getter(self.config)
        _is_aof_enabled = is_aof_enabled(self.config, options_getter)
        if _is_aof_enabled:
            result = self.remove_if_aof_enabled(
                aof_full_path_pre_7, aof_dir_path, redis_version_major, dry_run, force, options_getter
            )
        else:
            result = self.remove_if_aof_disabled(aof_full_path_pre_7, aof_dir_path, redis_version_major, dry_run)
        if result:
            return result

        return self.cleanup_aof_remove(bgsave_wait, bgsave_rename, lastsave_rename, bgrewriteaof_rename)

    def prechecks(self, aof_full_path_pre_7, aof_dir_path, redis_version_major, disk_limit, aof_limit, force):
        # 0. check file existence
        if redis_version_major < REDIS_VERSION_WITH_SPLITTED_AOF:
            if not os.path.exists(aof_full_path_pre_7):
                return CommandResult(
                    result_print="No {} - skip other steps".format(aof_full_path_pre_7), result_code=ResultCode.ERROR
                )
        else:
            if not os.path.exists(aof_dir_path) or not any(
                [
                    True
                    for filename in os.listdir(aof_dir_path)
                    if AOF_FILENAME_BASE_PATTERN.match(filename) or AOF_FILENAME_PRE_7.match(filename)
                ]
            ):
                return CommandResult(result_print="No base AOF file - skip other steps", result_code=ResultCode.ERROR)
        # 0. check clean finish last time
        if os.path.exists(SAVE_STATE_FILE):
            return CommandResult(
                result_print="{} exists indicating dirty exit last time".format(SAVE_STATE_FILE),
                result_code=ResultCode.ERROR,
            )
        # 0. check force flag
        if not force:
            conn, state = get_state(self.config)
            if state == RedisStatus.AOF_WRITING_FAILED_ON_FULL_DISK:
                return CommandResult(
                    result_print="Disk is full but we can't proceed as we failed to connect to Redis"
                    + " to check if it's master (to skip that use --force flag)",
                    result_code=ResultCode.ERROR,
                )
            if state != RedisStatus.READY_TO_ACCEPT_COMMANDS:
                return CommandResult(result_print=state.value, result_code=ResultCode.ERROR)
            role, _is_master = is_master(conn)
            log.info('Current role is {}'.format(role))
            if _is_master:
                return CommandResult(
                    result_print="Can't proceed on master without --force flag", result_code=ResultCode.ERROR
                )
        # 1. disk check
        if redis_version_major < REDIS_VERSION_WITH_SPLITTED_AOF:
            path_to_some_file = aof_full_path_pre_7
        else:
            path_to_some_file = aof_dir_path
        is_full, disk_usage_before = is_disk_full(path_to_some_file, disk_limit)
        if not is_full:
            return CommandResult(
                result_print="Disk usage {}% is less than limit {}% provided, nothing done".format(
                    disk_usage_before, disk_limit
                )
            )
        # 2. aof check
        is_large, aof_usage_before = is_aof_large(aof_full_path_pre_7, aof_dir_path, redis_version_major, aof_limit)
        if not is_large:
            return CommandResult(
                result_print="AOF disk usage {}% is less than limit {}% provided, nothing done".format(
                    aof_usage_before, aof_limit
                )
            )

    def remove_if_aof_enabled(
        self, aof_full_path_pre_7, aof_dir_path, redis_version_major, dry_run, force, options_getter
    ):
        if dry_run:
            return CommandResult(result_print="AOF is enabled, file could be deleted after turning AOF off")
        if not force:
            options_setter, state = get_config_option_setter(self.config)
            result = self.turn_aof_off(options_getter, options_setter, state)
            if result.is_failed():
                return result

        # 7. remove aof
        result = remove_aof(aof_full_path_pre_7, aof_dir_path, redis_version_major)
        if result.is_failed():
            return result

        if force:
            options_setter, state = get_config_option_setter(self.config)
            result = self.turn_aof_off(options_getter, options_setter, state)
            if result.is_failed():
                return result

    @staticmethod
    def remove_if_aof_disabled(aof_full_path_pre_7, aof_dir_path, redis_version_major, dry_run):
        # 3. aof disabled
        if dry_run:
            return CommandResult(result_print="AOF is not enabled, file could be deleted right now")
        return remove_aof(aof_full_path_pre_7, aof_dir_path, redis_version_major)

    def cleanup_aof_remove(self, bgsave_wait, bgsave_rename, lastsave_rename, bgrewriteaof_rename):
        conn, state = get_state(self.config)
        if state != RedisStatus.READY_TO_ACCEPT_COMMANDS:
            return CommandResult(result_print=state.value, result_code=ResultCode.ERROR)

        # 8. run bgsave
        command_result = bgsave(conn, bgsave_rename, lastsave_rename, bgsave_wait)
        if command_result.is_failed():
            return command_result

        # 9. set appendonly
        options_setter, state = get_config_option_setter(self.config)
        set_result, is_ok = options_setter('appendonly', 'yes')
        if not is_ok:
            return CommandResult(
                result_print="Failed to set appendonly config option after: {}".format(set_result),
                result_code=ResultCode.ERROR,
            )
        # 10. run bgrewriteaof - we don't really care much about it's result, as it's:
        # - scheduled if unavailable now because of bgsave;
        # - skipped if already runs (on small bases it's possible).
        try:
            result = conn.execute_command(bgrewriteaof_rename)
        except Exception as exc:
            result = repr(exc)
        log.debug("bgrewriteaof_cmd result: %s", result)

        # 11. set save
        return restore_save(options_setter)

    @staticmethod
    def turn_aof_off(options_getter, options_setter, state):
        # 4. get save params and store it in file (otherwise we gonna lose it on restart after setting new save)
        save_before = options_getter('save')
        store_to_file(save_before, SAVE_STATE_FILE)
        # 5. set save 900 1
        if state == RedisStatus.AOF_WRITING_FAILED_ON_FULL_DISK:
            return CommandResult(
                result_print="Failed to set save config option as we failed to connect to Redis"
                + " (to skip that use --force flag)",
                result_code=ResultCode.ERROR,
            )
        if state != RedisStatus.READY_TO_ACCEPT_COMMANDS:
            return CommandResult(
                result_print="Failed to connect to Redis (see logs) - to skip that use --force flag",
                result_code=ResultCode.ERROR,
            )
        set_result, is_ok = options_setter('save', '900 1')
        if not is_ok:
            return CommandResult(
                result_print="Failed to set save config option before: {}".format(set_result),
                result_code=ResultCode.ERROR,
            )
        # 6. set appendonly
        set_result, is_ok = options_setter('appendonly', 'no')
        if not is_ok:
            return CommandResult(
                result_print="Failed to set appendonly config option before: {}".format(set_result),
                result_code=ResultCode.ERROR,
            )
        return CommandResult()
