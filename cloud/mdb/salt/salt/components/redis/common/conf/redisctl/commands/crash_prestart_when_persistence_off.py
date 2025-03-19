#!/usr/bin/env python
# -*- coding: utf-8 -*-
import time

from utils.command import Command, CommandResult, ErrorResult
from utils.ctl_logging import log
from utils.persistence import is_persistence_enabled
from utils.redis_server import check_master, is_master, is_master_in_config, is_single_noded
from utils.state import get_state, RedisStatus


def ping_shard(config, dbaas_conf, check_func, check_result):
    current_host = dbaas_conf['fqdn']
    shard_hosts = dbaas_conf['shard_hosts']
    for shard_host in [host for host in shard_hosts if host != current_host]:
        conn, state = get_state(config, host=shard_host)
        if state != RedisStatus.READY_TO_ACCEPT_COMMANDS:
            log.debug('failed to get connect to %s: %s', shard_host, state)
            continue
        result = check_func(conn)
        if check_result(result):
            return CommandResult()

    return ErrorResult(result_print="shard hosts ping result differs from expected")


class CrashPrestartWhenPersistenceOff(Command):
    cmd_name = "crash_prestart_when_persistence_off"

    def __call__(self):
        initial = True
        infinite_wait_prestart_flag = False
        while infinite_wait_prestart_flag or initial:
            is_pers_on = is_persistence_enabled(self.config)
            if is_pers_on:
                return CommandResult(result_print="AOF or RDB is on, skipping")

            dbaas_conf, wait_prestart_on_crash, infinite_wait_prestart_flag = self.config.get_redisctl_options(
                'dbaas_conf',
                'wait_prestart_on_crash',
                'infinite_wait_prestart_flag',
            )

            if is_single_noded(dbaas_conf):
                return CommandResult(result_print="single node in shard, skipping")

            if not is_master_in_config(dbaas_conf, self.config):
                return CommandResult(result_print="not a master according to config, skipping")

            if initial:
                log.debug("sleeping %s secs as crash stop detected", wait_prestart_on_crash)
                time.sleep(wait_prestart_on_crash)
                initial = False
            else:
                cmd = ping_shard(self.config, dbaas_conf, is_master, check_master)
                if not cmd.is_failed():
                    return cmd

            # infinite circle could not be stopped by pillar change without reloading of static data file
            self.config.load_static()

        return CommandResult()
