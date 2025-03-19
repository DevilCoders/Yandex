#!/usr/bin/env python3
"""
 Wait for input, parse csv and emit a tskv line.
 statements() iter -> lines() iter -> sys.stdin
"""
import argparse
import socket
import sys
import time
from datetime import datetime as dt
from os import environ

LOG_TYPE = environ.get('LOG_ENVIRONMENT', 'dbaas_int_log')
DATE_FORMAT = '%Y-%m-%d %H:%M:%S.%f'  # 2017-04-12 17:32:45.727
PC_KEY = '__pk_data'


def get_timestamp(string):
    """
    Convert pgbouncer timestamp to unix
    """
    try:
        tstamp = dt.strptime(string, DATE_FORMAT).timestamp()
        millis = tstamp % 1
    except (AttributeError, ValueError) as err:
        raise ValueError('error parsing timestamp %s: %s' % (string, err))
    return [int(tstamp), millis]


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
    # `for line in file: pass` construct hogs cpu on read() syscalls
    while True:
        chunk = source.readline()
        if not chunk:  # empty line generates '\n'
            raise EOFError()
        line_id[PC_KEY], line_data = detect_magic(chunk)
        # Save in case an exception is thrown later in the processing
        yield line_data.strip()


def statements():
    """
    Yields a single parsed statement
    """
    def service_msg(line):
        """
        tls_sbufio_recv: read failed: Connection reset by peer
        """
        return {'text': line}

    def db_msg(line):
        """
        C-<pointer>: <db>/<user>@<ip>:<port> Pooler Error:
        """
        session_key, db_info, message = line.split(' ', 2)
        db, user_source = db_info.split('/', 1)
        user, source = user_source.split('@', 1)
        return {
            'session_id': session_key[:-1],  # strip semicolon
            'db': db,
            'user': user,
            'source': source,
            'text': message,
        }

    def skip_pgsync_checks(msg):
        """
        Skip checks in pooler
        """
        source = '[::1]'
        text = 'closing because: client unexpected eof (age=0)'
        if msg['source'].startswith(source) and msg['text'] == text:
            return True

    line_id = {}  # push-client`s magic numbers in each string
    for line in readlines(line_id=line_id):
        try:
            log_date, log_time, pid, level, msg = line.split(' ', 4)
            data = {
                'log_time': '%s %s' % (log_date, log_time),
                'pid': pid,
                'level': level,
            }
            # session messages have session keys.
            if msg.startswith('C-0x'):
                db_message = db_msg(msg)
                if skip_pgsync_checks(db_message):
                    continue
                data.update(db_message)
            else:
                data.update(service_msg(msg))
            data.update(line_id)
            yield data
        except ValueError:
            continue


def add_tracking_data(stm, cluster, hostname, origin='pgbouncer'):
    """
    Add timestamps, milliseconds and logtype.
    """
    assert isinstance(stm, dict), 'stm must be a dict!'
    tstamp, millis = get_timestamp(stm['log_time'])
    addendum = {
        'timestamp': tstamp,
        'ms': '%d' % int(millis * 1000),  # Clickhouse does not like floats.
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
    pos = stm[PC_KEY]
    # Remove from resulting string.
    del stm[PC_KEY]
    # Form string.
    tskv = ['%s=%s' % (k, prepare(v)) for k, v in stm.items()]
    print('{coordinates};tskv\t{msg}'.format(
        coordinates=';'.join(pos),
        msg='\t'.join(tskv),
    ))


def main():
    """
    Wait for input, parse csv and emit a tskv line.
    statements() iter -> csv.DictReader() iter -> lines() iter -> sys.stdin
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--cluster', type=str, help='Cluster name')
    parser.add_argument(
        '-o',
        '--origin',
        type=str,
        help='Originator identifier',
        default='pgbouncer'
    )
    args = parser.parse_args()

    hostname = socket.getfqdn()

    for stm in statements():
        try:
            add_tracking_data(stm, args.cluster, hostname, args.origin)
            print_tskv(stm)
        except EOFError:
            break
        except Exception as exc:
            print(exc, file=sys.stderr)


if __name__ == '__main__':
    main()
