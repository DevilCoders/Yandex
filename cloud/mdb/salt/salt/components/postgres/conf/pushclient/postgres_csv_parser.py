{% from "components/postgres/pg.jinja" import pg with context %}
#!/usr/bin/env python3
"""
1. Sanitize string (replace double quote escape ("") with '''
2. Yield a complete statement string
3. Add service fields (timestampt, log_format)
4. Emit a line of tskv
"""
import argparse
import csv
import errno
import socket
import sys
import time
import logging
from datetime import datetime as dt
from os import environ

RS = ','
LOG_TYPE = environ.get('LOG_ENVIRONMENT', 'dbaas_int_log')
DATE_FORMAT = '%Y-%m-%d %H:%M:%S.%f %Z'
MAX_CELL_LENGTH = 10 * 1024 * 1024 # 10MB
COLUMNS = [
    'log_time',
    'user_name',
    'database_name',
    'process_id',
    'connection_from',
    'session_id',
    'session_line_num',
    'command_tag',
    'session_start_time',
    'virtual_transaction_id',
    'transaction_id',
    'error_severity',
    'sql_state_code',
    'message',
    'detail',
    'hint',
    'internal_query',
    'internal_query_pos',
    'context',
    'query',
    'query_pos',
    'location',
    'application_name',
{% if pg.version.major_num == 1300 %}
    'backend_type',
{% endif %}
]
PC_KEY = '__pk_data'
RAW_LINE_KEY = '__raw_line'


class ParseError(ValueError):
    """ Subclass of general error that contains data in question """
    def __init__(self, *args, data):
        super().__init__(*args)
        self.data = data


def SpecialCSVReader(chunks, delimiter=",", quotechar="\"", escapechar="\""):
    """ special street csv parser """
    in_quote = False
    in_escape = False
    row = []
    cell = ""
    for chunk in chunks:
        start = 0
        for i, c in enumerate(chunk):
            if in_quote:
                if c == escapechar and i < len(chunk) - 1 and chunk[i+1] == quotechar and not in_escape:
                    cell += chunk[start:i]
                    start = i + 1
                    in_escape = True
                elif c == quotechar:
                    if in_escape:
                        in_escape = False
                    else:
                        cell += chunk[start:i]
                        start = i + 1
                        in_quote = False
            else:
                if c == delimiter:
                    cell += chunk[start:i]
                    row.append(cell[:MAX_CELL_LENGTH])
                    cell = ""
                    start = i + 1
                elif c == quotechar and (i == 0 or chunk[i-1] == delimiter):
                    # strange, but quotation may start only just after delimiter
                    cell += chunk[start:i]
                    start = i + 1
                    in_quote = True
        cell += chunk[start:]
        if not in_quote:
            cell = cell.rstrip("\n")
            if cell or row:
                row.append(cell[:MAX_CELL_LENGTH])
            yield row
            row = []
            cell = ""
    if cell or row:
        row.append(cell[:MAX_CELL_LENGTH])
    if row:
        yield row



def SpecialCSVDictReader(chunks, fieldnames, **kwargs):
    for row in SpecialCSVReader(chunks, **kwargs):
        if len(fieldnames) != len(row):
            """In case of invalid input send it as row message MDB-4916"""
            result = {}
            for fieldname in fieldnames:
                result[fieldname] = ''
            result['log_time'] = row[0]
            result['message'] = ' '.join(row[1:])
            yield result
        else:
            yield dict(zip(fieldnames, row))


def get_timestamp(string):
    """
    Convert postgres timestamp to unix
    """
    ts_candidate, *_ = string.split(RS)
    try:
        tstamp = dt.strptime(ts_candidate, DATE_FORMAT).timestamp()
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


def readlines(source=sys.stdin, misc_data=None):
    """
    Input entry point.

    Yields a single sanitized string from stdin
    with magic
    """
    # `for line in file: pass` construct hogs cpu on read() syscalls
    while True:
        chunk = source.readline()
        if not chunk:
            raise EOFError()
        misc_data[PC_KEY], line_data = detect_magic(chunk)
        # Save in case an exception is thrown later in the processing
        misc_data[RAW_LINE_KEY] = line_data
        suffix = ' ' if line_data.endswith('\n') or line_data.endswith(' ') else ''
        yield line_data.strip() + suffix


def statements():
    """
    Yields a single parsed statement
    """
    line_data = {}  # e.g., push-client`s magic numbers in each string
    try:
        for row in SpecialCSVDictReader(readlines(misc_data=line_data),
                                  fieldnames=COLUMNS, delimiter=RS):
            row.update(line_data)
            yield dict(row)
    except EOFError:
        raise
    except Exception as exc:
        raise ParseError(
            exc,
            data=line_data.get(RAW_LINE_KEY)) from exc


def add_tracking_data(stm, cluster, hostname):
    """
    Add timestamps, milliseconds and logtype.
    """
    assert isinstance(stm, dict), 'stm must be a dict!'

    tstamp, millis = get_timestamp(stm['log_time'])
    addendum = {
        'timestamp': tstamp,
        'ms': '%d' % int(millis * 1000),  # Clickhouse does not like floats.
        'log_format': LOG_TYPE,
        'origin': 'postgresql',
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
    # Remove misc fields from resulting string.
    for key in (PC_KEY, RAW_LINE_KEY):
        del stm[key]
    # Form string.
    tskv = ['%s=%s' % (k, prepare(v)) for k, v in stm.items()]
    print('{coordinates};tskv\t{msg}'.format(
        coordinates=';'.join(pos),
        msg='\t'.join(tskv),
    ))

def _do_processing(args, hostname):
    for stm in statements():
        add_tracking_data(stm, args.cluster, hostname)
        print_tskv(stm)


def main():
    """
    Wait for input, parse csv and emit a tskv line.
    statements() iter -> SpecialCSVDictReader() iter -> lines() iter -> sys.stdin
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-c', '--cluster',
        type=str, help='Cluster name',
        default='')
    parser.add_argument(
        '-l', '--log-file',
        type=str, help='Debug log file path',
        default='/var/log/statbox/postgres_csv_parser.log')
    args = parser.parse_args()
    logging.basicConfig(
        filename=args.log_file,
        format='%(asctime)s:\t%(message)s')

    hostname = socket.getfqdn()

    while True:
        try:
            _do_processing(args, hostname=hostname)
        except ParseError as err:
            logging.exception('parser error: %s, data: %s', err, err.data)
        except EOFError:
            break
        except Exception as exc:
            logging.exception('general error: %s', exc)
            time.sleep(0.2)


if __name__ == '__main__':
    main()
