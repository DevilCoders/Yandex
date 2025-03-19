#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Entry point for interacting with Redis:
 - commands folder: contains .py files with certain commands, granular as possible; SHOULDN'T BE USED FROM OTHER FILES
 - utils folder: contains different utils function, used in commands and in other utils
 - current file: just entry-point, no working code
"""
import argparse
import sys

from commands.crash_prestart_when_persistence_off import CrashPrestartWhenPersistenceOff
from commands.is_alive import IsAlive
from commands.get_config_option import GetConfigOption
from commands.stop_and_wait import StopAndWait
from commands.wait_replica_synced import WaitReplicaSynced
from commands.wait_started import WaitStarted
from commands.dump import Dump
from commands.remove_aof import RemoveAof
from commands.run_save_when_persistence_off import RunSaveWhenPersistenceOff
from utils.config import Config
from utils.ctl_logging import log

IS_ALIVE = 'is_alive'
WAIT_STARTED = 'wait_started'
GET_CONFIG_OPTION = 'get_config_options'
DUMP = 'dump'
WAIT_REPLICA_SYNCED = 'wait_replica_synced'
STOP_AND_WAIT = 'stop_and_wait'
REMOVE_AOF = 'remove_aof'
RUN_SAVE_WHEN_PERSISTENCE_OFF = 'run_save_when_persistence_off'
CRASH_PRESTART_WHEN_PERSISTENCE_OFF = 'crash_prestart_when_persistence_off'

COMMANDS = [
    IS_ALIVE,
    WAIT_STARTED,
    GET_CONFIG_OPTION,
    DUMP,
    WAIT_REPLICA_SYNCED,
    STOP_AND_WAIT,
    REMOVE_AOF,
    RUN_SAVE_WHEN_PERSISTENCE_OFF,
    CRASH_PRESTART_WHEN_PERSISTENCE_OFF,
]
COMMANDS_LIST = ",".join(COMMANDS)


def get_args(argv=None):
    parser = argparse.ArgumentParser(description='Redis management script')
    parser.add_argument('--action', help="Action to perform {}".format(COMMANDS_LIST), required=True, choices=COMMANDS)
    parser.add_argument('--options', help="Options for --action get_config_options", default=None)
    parser.add_argument('--restarts', help="Restarts count for --action wait_started", default=0, type=int)
    parser.add_argument('--timeout', help="Timeout in secs for --action wait_started", default=0, type=int)
    parser.add_argument('--sleep', help="Sleep in secs for --action wait_started", default=0, type=int)
    parser.add_argument('--file', help="Option for dump command: filename for dump", default=None)
    parser.add_argument(
        '--bgsave-wait',
        help="Option for dump command: how many seconds to wait for bgsave complete",
        dest='bgsave_wait',
        default=30 * 60,
        type=int,
    )
    parser.add_argument(
        '--disk-limit',
        help="Option for remove_aof command: data disk usage percentage to proceed",
        dest='disk_limit',
        default=95,
        type=int,
    )
    parser.add_argument(
        '--aof-limit',
        help="Option for remove_aof command: aof disk usage percentage to proceed",
        dest='aof_limit',
        default=25,
        type=int,
    )
    parser.add_argument(
        '--dry-run',
        help="Option for remove_aof command: do not change anything, just show plan",
        action='store_true',
        default=False,
        dest='dry_run',
    )
    parser.add_argument(
        '--force',
        help="Option for remove_aof command: skip on master",
        action='store_true',
        default=False,
        dest='force',
    )
    parser.add_argument(
        '--redis-data-path',
        help="Path to redis data path used to initialize config",
        default='/home/redis/info.json',
        dest='redis_data_path',
    )
    parser.add_argument(
        '--redis-conf-paths',
        help="Path to redis configs",
        default=('/etc/redis/redis.conf', '/etc/redis/redis-main.conf'),
        dest='redis_conf_path',
    )
    parser.add_argument(
        '--dbaas-conf-path',
        help="Path to dbaas config",
        default='/etc/dbaas.conf',
        dest='dbaas_conf_path',
    )
    parser.add_argument(
        '--sentinel-conf-path',
        help="Path to sentinel config",
        default='/etc/redis/sentinel.conf',
        dest='sentinel_conf_path',
    )
    parser.add_argument(
        '--cluster-conf-path',
        help="Path to cluster config",
        default='/etc/redis/cluster.conf',
        dest='cluster_conf_path',
    )
    args = parser.parse_args(argv)
    return args


def run(args, config):
    log.info("Started redisctl with args %s", args)

    action = args.action
    if action not in COMMANDS:
        raise ValueError("--action should be one of {}".format(COMMANDS_LIST))

    command = None

    if action == IS_ALIVE:
        command = IsAlive(config)

    if action == WAIT_STARTED:
        command = WaitStarted(config)

    if action == GET_CONFIG_OPTION:
        command = GetConfigOption(config)

    if action == GET_CONFIG_OPTION:
        command = GetConfigOption(config)

    if action == DUMP:
        command = Dump(config)

    if action == WAIT_REPLICA_SYNCED:
        command = WaitReplicaSynced(config)

    if action == STOP_AND_WAIT:
        command = StopAndWait(config)

    if action == REMOVE_AOF:
        command = RemoveAof(config)

    if action == RUN_SAVE_WHEN_PERSISTENCE_OFF:
        command = RunSaveWhenPersistenceOff(config)

    if action == CRASH_PRESTART_WHEN_PERSISTENCE_OFF:
        command = CrashPrestartWhenPersistenceOff(config)

    if not command:
        raise ValueError("Internal error, go to developers")

    command_result = command()
    if command_result.is_failed():
        log.debug("Command '{}' failed: {}".format(action, command_result))
        sys.stderr.write(str(command_result) + "\n")
        sys.exit(int(command_result.result_code))

    message = str(command_result)
    if message:
        print(message)
    log.debug("Command '{}' success: {}".format(action, command_result))
    sys.exit(0)


if __name__ == "__main__":
    args = get_args()
    config = Config(args)
    run(args, config)
