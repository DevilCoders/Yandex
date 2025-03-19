#!/usr/bin/env python3
"""
Pushclient lag/commit delay monitoring
"""
import argparse
import glob
import json
import os
import shlex
import subprocess
import sys
import time
from secrets import randbelow

BASEPATH = '/etc/pushclient'


def parse_args():
    """
    Parse known cmd args
    """
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-w',
        '--lag-warn',
        type=int,
        default=67108864,
        help='Warning lag limit')

    parser.add_argument(
        '-c',
        '--lag-crit',
        type=int,
        default=134217728,
        help='Critical lag limit')

    parser.add_argument(
        '--billing-lag-warn',
        type=int,
        default=131072,
        help='Warning lag limit for billing logs')

    parser.add_argument(
        '--billing-lag-crit',
        type=int,
        default=1048576,
        help='Critical lag limit for billing logs')

    parser.add_argument(
        '-b',
        '--base',
        type=str,
        default='/usr/bin/push-client -f -w -c {config} --status --json',
        help='Push-client base status command')

    parser.add_argument(
        '-l',
        '--configs',
        type=str,
        help='Push-client config path')

    parser.add_argument(
        '-n',
        '--delay-warn',
        type=int,
        default=43200,
        help='Commit delay warning limit')

    parser.add_argument(
        '-m',
        '--delay-crit',
        type=int,
        default=86400,
        help='Commit delay critical limit')

    parser.add_argument(
        '--billing-delay-warn',
        type=int,
        default=300,
        help='Commit delay warning limit for billing logs')

    parser.add_argument(
        '--billing-delay-crit',
        type=int,
        default=3600,
        help='Commit delay critical limit for billing logs')

    return parser.parse_args()


def config_discover(basepath):
    return glob.glob('{path}/*.conf'.format(path=basepath))


def die(code, comment):
    """
    Passive-check message formatter
    """
    print('{code};{message}'.format(
        code=code, message=comment if comment else 'OK'))
    sys.exit(0)


def format_lag(log_name, lag):
    """
    Format lag message
    """
    return '{log_name} has lag {lag}'.format(log_name=log_name, lag=lag)


def format_delay(log_name, delay):
    """
    Format delay message
    """
    return '{log_name} has commit delay {delay}'.format(
        log_name=log_name, delay=delay)


def get_status(base_cmd, config):
    """
    Get status of pushclient for config
    """
    cmd = base_cmd.format(config=config)
    tries = 10
    try_num = 0
    while try_num < tries:
        proc = subprocess.Popen(
            ['/usr/bin/timeout', '-k', '7', '5'] + shlex.split(cmd), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = proc.communicate()
        stdout = stdout.decode()
        stderr = stderr.decode()
        if not stderr and proc.poll() == 0:
            break
        if proc.poll() == 124:
            stderr = 'Status command timed out'
        try_num += 1
        time.sleep(7 + randbelow(int(1000 * min(1.5 ** try_num, 5))) / 1000)
    return stdout, stderr


def check_status(name, lag, commit_delay, args, message):
    """
    Check status of log file
    """
    max_status = 0
    if name.endswith('billing.log'):
        lag_crit = args.billing_lag_crit
        lag_warn = args.billing_lag_warn
        delay_crit = args.billing_delay_crit
        delay_warn = args.billing_delay_warn
    else:
        lag_crit = args.lag_crit
        lag_warn = args.lag_warn
        delay_crit = args.delay_crit
        delay_warn = args.delay_warn
    if lag > lag_crit:
        max_status = 2
        message.append(format_lag(name, lag))
    elif lag > lag_warn:
        max_status = max(max_status, 1)
        message.append(format_lag(name, lag))

    delta = int(time.time() - os.path.getmtime(name))

    if delta < commit_delay:
        if commit_delay > delay_crit:
            max_status = 2
            message.append(format_delay(name, commit_delay))
        elif commit_delay > delay_warn:
            max_status = max(max_status, 1)
            message.append(format_delay(name, commit_delay))

    return max_status


def _main():
    args = parse_args()
    message = []
    max_status = 0
    configs = args.configs.split() if args.configs else None
    if not configs:
        configs = config_discover(BASEPATH)
    for config in configs:
        out, err = get_status(args.base, config)
        if err:
            max_status = 2
            message.append(' '.join(err.split()))
            continue
        try:
            states = json.loads(out)
            for missing_state in [x for x in states if 'lag' not in x]:
                if os.path.exists(missing_state['name']) and os.path.getsize(
                        missing_state['name']) == 0:
                    continue
                max_status = 2
                message.append('{name} {stype}'.format(
                    name=missing_state['name'], stype=missing_state['type']))
            for state in [x for x in states if 'lag' in x]:
                max_status = max(
                    max_status,
                    check_status(state['name'], state['lag'],
                                 state['commit delay'], args, message))
        except Exception as exc:
            max_status = 2
            message.append('{config} error: {exc}'.format(
                config=config, exc=repr(exc)))

    die(max_status, '; '.join(message))


if __name__ == '__main__':
    _main()
