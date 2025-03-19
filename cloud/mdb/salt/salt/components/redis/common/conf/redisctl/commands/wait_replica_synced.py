#!/usr/bin/env python
# -*- coding: utf-8 -*-
from utils.ctl_logging import log
from utils.command import CommandResult, ResultCode, WaitCommand
from utils.state import get_state, RedisStatus


class WaitReplicaSynced(WaitCommand):
    cmd_name = "wait_replica_synced"

    def call_func(self):
        repl_data = self.config.get_redisctl_options('repl_data')
        conn, state = get_state(self.config)
        if state != RedisStatus.READY_TO_ACCEPT_COMMANDS:
            return CommandResult(result_print=state.value, result_code=ResultCode.ERROR)

        info = conn.info()
        repl_data = ",".join(
            ["{}={}".format(name, info[name]) for name in repl_data if info.get(name, None) is not None]
        )
        log.debug("replica sync related data: {}".format(repl_data))
        if info.get('master_failover_state') and info['master_failover_state'] != 'no-failover':
            return CommandResult(result_print="failover in progress", result_code=ResultCode.ERROR)
        if info.get('master_sync_in_progress') == 1:
            return CommandResult(result_print="replica sync in progress", result_code=ResultCode.ERROR)
        if info['role'] == 'master':
            return CommandResult(result_print="no need to wait for replica sync cause it's master")
        if info.get('master_link_status') == 'up' and info.get('master_sync_in_progress') == 0:
            return CommandResult(result_print="replica is not syncing now")
        return CommandResult(result_print="replica sync state unexpected (see logs)", result_code=ResultCode.ERROR)
