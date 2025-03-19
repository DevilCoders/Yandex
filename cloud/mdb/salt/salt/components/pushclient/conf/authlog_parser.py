#!/usr/bin/env python3
"""
 Wait for input, parse csv and emit a tskv line.
 statements() iter -> lines() iter -> sys.stdin

"""
import argparse
import socket
import logging
import sys
import time
from datetime import datetime as dt
from os import environ

LOG_TYPE = environ.get('LOG_ENVIRONMENT', 'dbaas_int_log')
DATE_FORMAT = '%Y %b %d %H:%M:%S'  # 2017 June 1 17:32:45.727
CURRENT_YEAR = dt.strftime(dt.today(), '%Y')
PK_KEY = '__pk_data'


def get_timestamp(string):
    """
    Convert time identifier to unix
    """
    try:
        tstamp = dt.strptime(string, DATE_FORMAT).timestamp()
    except (AttributeError, ValueError) as err:
        raise ValueError('error parsing timestamp %s: %s' % (string, err))
    return int(tstamp)


def prepare(string):
    """
    Cast to string, sanitize according to
    https://wiki.yandex-team.ru/statbox/LogRequirements/
    """
    patterns = [
        # pattern, replacement
        ['\t', ' '],
        ['=', r'\='],
        ['\n', r'\n']
    ]
    string = str(string)
    for pattern, repl in patterns:
        string = string.replace(pattern, repl)
    return string


def detect_magic(string):
    """
    Detect magic number at beginning of the line
    """
    try:
        *nums, data = string.split(';', 3)
        for num in nums:
            int(num)
        return [nums, data]
    except (ValueError, TypeError) as err:
        raise ValueError('unable to read line coordinates: %s' % err)


def readlines(source=sys.stdin, line_id=None):
    """
    Input entry point.

    Yields a single sanitized string from stdin
    with magic
    """
    while True:
        chunk = source.readline()
        if not chunk:
            raise EOFError()
        line_id[PK_KEY], line_data = detect_magic(chunk)
        yield line_data.strip()


def statements():
    """
    Yields a single parsed statement
    """
    line_id = {}  # push-client`s magic numbers in each string
    for line in readlines(line_id=line_id):
        # Jun 21 09:40:09 paysys-trust01f sshd[1035564]: <msg>
        month, day, timestamp, host, process_tag, msg = \
            line.split(sep=None, maxsplit=5)
        # CRON[1034747]
        daemon, _, pid = process_tag.replace(':', '').partition('[')
        data = {
            'log_time': '%s %s %s %s' % (
                CURRENT_YEAR,
                month,
                day,
                timestamp
            ),
            'daemon': daemon.lower(),
            'pid': pid.replace(']', ''),
            'text': msg,
        }
        # session messages have session keys.
        data.update(line_id)
        yield data


def add_tracking_data(stm, cluster, hostname, origin='system_auth'):
    """
    Add timestamps, milliseconds and logtype.
    """
    assert isinstance(stm, dict), 'stm must be a dict!'
    tstamp = get_timestamp(stm['log_time'])
    addendum = {
        'timestamp': tstamp,
        'log_format': LOG_TYPE,
        'origin': origin,
        'cluster': cluster,
        'hostname': hostname
    }
    stm.update(addendum)


def print_tskv(stm):
    """
    Print TSKV-line from statement dictionary.
    """
    # Find out PK line coords.
    pos = stm[PK_KEY]
    # Remove from resulting string.
    del stm[PK_KEY]
    # Form string.
    tskv = ['%s=%s' % (k, prepare(v)) for k, v in stm.items()]
    print('{coordinates};tskv\t{msg}'.format(
        coordinates=';'.join(pos),
        msg='\t'.join(tskv),
    ))


def _do_processing(args, hostname):
    for stm in statements():
        add_tracking_data(stm, args.cluster, hostname, args.origin)
        print_tskv(stm)


def main():
    """
    Wait for input, parse csv and emit a tskv line.
    statements() iter -> csv.DictReader() iter -> lines() iter -> sys.stdin
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-c', '--cluster', type=str, help='Cluster name'
    )
    parser.add_argument(
        '-o',
        '--origin',
        type=str,
        help='Originator identifier',
        default='syslog_auth'
    )
    parser.add_argument(
        '-l', '--log-file',
        type=str, help='Debug log file path',
        default='/var/log/statbox/authlog_parser.log')
    args = parser.parse_args()

    logging.basicConfig(
        filename=args.log_file,
        format='%(asctime)s:\t%(message)s')
    hostname = socket.getfqdn()

    while True:
        try:
            _do_processing(args, hostname)
        except EOFError:
            break
        except ValueError as exc:
            logging.error('parser error: %s', exc)
        except Exception as exc:
            logging.exception('general error: %s', exc)
            time.sleep(1)


if __name__ == '__main__':
    main()
