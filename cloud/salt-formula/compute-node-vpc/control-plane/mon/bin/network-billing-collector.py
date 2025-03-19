#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Checks that network billing collector is functional

import subprocess
import os.path
import glob
import time
import json

import yaml


CHECK_NAME = 'network-billing-collector'
CONFIG_FILE = '/home/monitor/agents/etc/network-billing-collector.conf'
WIKI_HELP_LINK = 'See https://nda.ya.ru/3UXFWw for more info.'

SERVICE_NAME = 'yc-network-billing-collector'

STATUS_OK = 0
STATUS_WARN = 1
STATUS_CRIT = 2

PUSH_CLIENT_CONFIG_BILLING = '/etc/yandex/statbox-push-client/conf.d/push-client-billing.yaml'
PUSH_CLIENT_CONFIG_ANTIFRAUD = '/etc/yandex/statbox-push-client/conf.d/push-client-sdn_antifraud.yaml'


class CheckCrit(Exception):
    pass


class CheckWarn(Exception):
    pass


def is_service_active(service_name):
    proc = subprocess.run(['systemctl', 'is-active', service_name],
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return proc.returncode == 0


def count_running_processes(pattern):
    proc = subprocess.run(['pgrep', '--full', '--count', pattern], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    return int(proc.stdout)


def get_service_start_time(service_name):
    proc = subprocess.run(['systemctl', 'show', '--property=ExecMainStartTimestamp', service_name],
                          stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, env={'LC_TIME': 'C'}, check=True)
    # outputs: 'ExecMainStartTimestamp=Fri 2018-08-31 15:18:45 UTC'
    # outputs on fail: 'ExecMainStartTimestamp='
    # exit code is expected to be always 0, even on non-existing service
    _, value = proc.stdout.decode('utf-8').strip().split('=', 1)
    return value or None


def get_first_error_since(service_name, since):
    args = ['journalctl', '--lines=1', '--priority=0..3', '--quiet',
            '--since', since, '--unit', service_name]
    proc = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=True)
    return proc.stdout.decode('utf-8').strip() or None


def test_is_service_running():
    assert is_service_active("systemd-journald") is True
    assert is_service_active("non-existent") is False


def test_count_running_processes():
    assert count_running_processes('nonexistent_process') == 0
    # trying to find process running this test
    assert count_running_processes('network-billing-collector.py') == 1


def format_time(seconds):
    seconds = int(seconds)
    result = []
    for period_name, period_seconds in [('hour', 3600), ('minute', 60), ('second', 1)]:
        if seconds >= period_seconds:
            cnt = seconds // period_seconds
            seconds -= cnt * period_seconds
            s = 's' if cnt > 1 else ''
            result.append('{} {}{}'.format(cnt, period_name, s))
    return ' '.join(result)


def test_format_time():
    assert format_time(75) == '1 minute 15 seconds'
    assert format_time(2*3600) == '2 hours'


def getstat_timeout(path, timeout_sec=0.5):
    """Workaround possible race when getting mtime for a rotating log.

    File rotation usually consist of renaming old file and then creating new one in place.
    We can possibly fall in-between these steps and fail. Retries are needed to avoid this case
    and make monitoring more stable.
    """

    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        try:
            return os.stat(path)  # let's spin!
        except FileNotFoundError:
            pass
    else:
        raise RuntimeError('Unable to get mtime for {} within {} seconds'.format(path, timeout_sec))


def glob_with_attr(pattern, attr_func):
    for path in glob.glob(pattern):
        try:
            attr = attr_func(path)
        except FileNotFoundError:
            # ignore files that were moved or deleted
            # between getting file list and attributes
            continue

        yield path, attr


def get_push_client_status(push_client_cmd, log_file, push_client_debug_help):
    # CLOUD-15767 network-billing-collector check flaps with KeyError and CalledProcessError(2)
    # Try to ignore transition processes, probably during log file rotation.
    max_retries = 3
    retry_delay_sec = 2

    error_msg = None
    for retry in range(max_retries):
        try:
            proc = subprocess.run(push_client_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
            status_list = json.loads(proc.stdout.decode('utf-8'))
        except Exception as e:
            error_msg = "Can't get status of push-client (using {cmd!r}): {e!r}".format(
                cmd=' '.join(push_client_cmd),
                e=e
            )
            time.sleep(retry_delay_sec)
            continue

        network_files = [file for file in status_list if file['name'] == log_file]
        if not network_files:
            msg = "Push client doesn't know about our file ({!r}). {}"
            raise CheckCrit(msg.format(log_file, push_client_debug_help))

        try:
            last_commit_time = max(file['last commit time'] for file in network_files)
            lag_sum = sum(file['lag'] for file in network_files)
        except KeyError as e:
            error_msg = "Couldn't parse push-client status: {!r}. {}".format(e, push_client_debug_help)
            time.sleep(retry_delay_sec)
            continue

        return last_commit_time, lag_sum

    raise RuntimeError(error_msg)


def check_service_status(config):
    check_service_active()
    check_running_processes(expected=config['expected_proc_count'])
    check_errors_in_journald()

    check_logs_are_processed_in_time(glob_pattern=config['unprocessed_flows_glob'],
                                     max_delay=config['max_processing_delay'])
    check_no_logs_stuck_in_processing(glob_pattern=config['processing_flows_glob'],
                                      max_time=config['max_processing_time'])

    check_push_client_and_output_log(log_file=config['accounting_log'],
                                     push_client_config=PUSH_CLIENT_CONFIG_BILLING,
                                     max_push_client_lag=config['max_push_client_lag_accounting'],
                                     max_send_delay=config['max_send_delay'],
                                     max_idle_time=config['max_time_without_output'])

    check_push_client_and_output_log(log_file=config['antifraud_log'],
                                     push_client_config=PUSH_CLIENT_CONFIG_ANTIFRAUD,
                                     max_push_client_lag=config['max_push_client_lag_antifraud'],
                                     max_send_delay=config['max_send_delay'],
                                     max_idle_time=config['max_time_without_output'])


def check_errors_in_journald():
    start_time = get_service_start_time(SERVICE_NAME)
    if start_time is None:
        raise CheckCrit('Seems that service {!r} was never started'.format(SERVICE_NAME))

    first_error = get_first_error_since(SERVICE_NAME, start_time)
    if first_error:
        msg = 'Errors in journald for {!r} since service start. Please fix and restart service. ' \
                'First error: {!r}. {}'
        raise CheckCrit(msg.format(SERVICE_NAME, first_error, WIKI_HELP_LINK))


def check_service_active():
    if not is_service_active(SERVICE_NAME):
        msg = 'Service {!r} is not in active state. See systemctl status for details.'
        raise CheckCrit(msg.format(SERVICE_NAME))


def check_running_processes(expected):
    count = count_running_processes('/usr/bin/yc-network-billing-collector')
    if count != expected:
        msg = 'Too few running processes: found={}, expected={}. Some workers have died?'
        msg = msg.format(count, expected)
        raise CheckCrit(msg)


def check_logs_are_processed_in_time(glob_pattern, max_delay):
    now = time.time()
    for path, mtime in glob_with_attr(glob_pattern, os.path.getmtime):
        processing_delay = now - mtime
        if processing_delay > max_delay:
            msg = '{!r} was not processed for more than {} (must be < {}). {}'
            msg = msg.format(path, format_time(processing_delay), format_time(max_delay), WIKI_HELP_LINK)
            raise CheckCrit(msg)


def check_no_logs_stuck_in_processing(glob_pattern, max_time):
    now = time.time()
    for path, ctime in glob_with_attr(glob_pattern, os.path.getctime):
        processing_time = now - ctime
        if processing_time > max_time:
            msg = '{!r} is in processing state for more than {} (must be < {}).' \
                   + ' Worker hung? Some files left after restart? {}'
            msg = msg.format(path, format_time(processing_time), format_time(max_time), WIKI_HELP_LINK)
            raise CheckCrit(msg)


def check_push_client_and_output_log(log_file, push_client_config,
                                     max_push_client_lag, max_send_delay,
                                     max_idle_time):
    push_client_cmd = ['push-client', '-c', push_client_config, '--status', '--json']
    push_client_debug_help = 'Try running {!r} to see what went wrong.'.format(' '.join(push_client_cmd))

    log_file_stat = getstat_timeout(log_file, timeout_sec=0.5)

    # push-client doesn't report usual status for empty files
    # so we don't check it if we didn't write anything for it anyway
    if log_file_stat.st_size == 0:
        # FIXME: CLOUD-10993 process ipv6 traffic in network-billing-collector to enhance monitoring
        # last_write_delay = time.time() - accounting_stat.st_mtime
        # if last_write_delay > max_idle_time:
        #     msg = "Haven't written to accounting log for more than {}. No traffic to account?"
        #     raise CheckWarn(msg.format(format_time(last_write_delay)))
        return

    last_commit_time, lag_sum = get_push_client_status(push_client_cmd, log_file, push_client_debug_help)

    check_push_client_state(
        last_commit_time=last_commit_time,
        lag_sum=lag_sum,
        last_write_time=log_file_stat.st_mtime,
        max_send_delay=max_send_delay,
        max_lag=max_push_client_lag,
        push_client_debug_help=push_client_debug_help
    )


def check_push_client_state(last_commit_time, lag_sum, last_write_time, max_send_delay, max_lag, push_client_debug_help):

    commit_delay = last_write_time - last_commit_time
    if commit_delay > max_send_delay:
        msg = 'push-client stopped sending data: last_write_to_log - last_commit_time > {}. {}'
        msg = msg.format(format_time(commit_delay), push_client_debug_help)
        raise CheckCrit(msg)

    # check push-client log reading lag in bytes (= file_size - file_offset)
    if lag_sum > max_lag:
        msg = 'push-client seems broken: lag sum for reading log files = {} bytes' \
              ' (should be 0 most of the time, definitely a problem when > {}). {}'
        msg = msg.format(lag_sum, max_lag, push_client_debug_help)
        raise CheckCrit(msg)


def load_config(path):
    with open(path) as stream:
        return yaml.load(stream)


def print_for_monitoring(status, desc):
    print('PASSIVE-CHECK:{};{};{}'.format(CHECK_NAME, status, desc or 'OK'))


def main():
    try:
        config = load_config(CONFIG_FILE)
    except IOError:
        print_for_monitoring(STATUS_CRIT,
                             "Can't load config file {!r} for monitoring".format(CONFIG_FILE))
        return 0

    try:
        check_service_status(config)
    except CheckWarn as e:
        print_for_monitoring(STATUS_WARN, str(e))
    except CheckCrit as e:
        print_for_monitoring(STATUS_CRIT, str(e))
    except Exception as e:
        print_for_monitoring(STATUS_CRIT, 'Unexpected error while running check: {!r}'.format(e))
    else:
        print_for_monitoring(STATUS_OK, None)

    return 0


if __name__ == '__main__':
    main()
