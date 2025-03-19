#!/usr/bin/env python3

import argparse
import json
import logging
import re
import signal
import socket
import sys
import warnings
from collections import namedtuple
from datetime import timezone, datetime, timedelta
from os import environ
from typing import Optional

from dateutil.parser import parse as parse_datetime
from dateutil.tz import tzlocal, gettz

MAX_LENGTH = 10 * (2**13)
MAX_FIELD_LENGTH = 1024 * 1024
ERROR_COOLDOWN = timedelta(minutes=10)
ERROR_MAX_LOG_LEN = 64 * 1024
CHUNK_WAIT_TIMEOUT = 10
GOT_SIGUSR1 = False

LoggedError = namedtuple('LoggedError', ['ts', 'count'])


class TskvSerializer:

    ESCAPE_TRANSLATION = str.maketrans({'\t': r'\t', '\n': r'\n', '\r': r'\r', '\0': r'\0'})

    def __call__(self, *args, **kwds) -> str:
        return self.tskv_dumps(args[0])

    def tskv_dumps(self, record: dict) -> str:
        """
        Serialize record to tskv format with line id prefix.
        """
        kvs = ('{}={}'.format(k, self._tskv_escape(str(v))) for k, v in record.items() if v is not None)
        return 'tskv\t' + '\t'.join(kvs)

    def _tskv_escape(self, string: str) -> str:
        """
        Escape the string to use in tskv.
        """
        return string.translate(self.ESCAPE_TRANSLATION)


class ChunkReadTimeout(Exception):
    pass


def interrup_readline(*_):
    raise ChunkReadTimeout()


def handle_SIGUSR1(*_):
    logging.info("Got SIGUSR1")
    global GOT_SIGUSR1
    GOT_SIGUSR1 = True


class LogParser:

    PIPE_PREFIX = 'key'
    REQUIRED_DT_KEY = 'datetime'

    AVAILABLE_SERIALIZERS = {'tskv': TskvSerializer(), 'json': json.dumps}
    AVAILABLE_PARSERS = {'json': json.loads, 'unknown': None}

    def __init__(self):
        self.input_date_format = None
        self.out_date_format = None
        self.origin = None
        self.timezone = None
        self.secret_regex = None
        self.errors = {}
        self._key_pattern = re.compile(r'^(?P<key>\d+;\d+;\d+;)')
        self.tracking_data = None
        self.dt_key = None
        self.find_dt = None
        self.serializer = None
        self.parser = None

    def initialize(self, args=None):
        logging.basicConfig(level='DEBUG', format='%(asctime)s %(message)s')
        parser = argparse.ArgumentParser()
        parser.add_argument('-c', '--cluster', type=str, default='', help='Cluster name')
        parser.add_argument('-o', '--origin', type=str, help='Originator identifier', default=None)
        parser.add_argument('-t', '--timezone', type=str, default='', help='Timezone of the logs')
        parser.add_argument('-s', '--secrets', type=str, help='Secrets regexp file', default=None)
        parser.add_argument('-d', '--dt-key', type=str, help='Key with datetime', default='datetime')
        parser.add_argument(
            '-i',
            '--input-format',
            type=str,
            choices=list(self.AVAILABLE_PARSERS.keys()),
            help='Input format',
            default='unknown',
        )
        parser.add_argument(
            '-f',
            '--output-format',
            type=str,
            choices=list(self.AVAILABLE_SERIALIZERS.keys()),
            help='Output serializer',
            default='tskv',
        )
        parsed_args = parser.parse_args(args=args)

        self.origin = parsed_args.origin or self.origin
        self.serializer = self.AVAILABLE_SERIALIZERS[parsed_args.output_format]
        self.parser = self.AVAILABLE_PARSERS[parsed_args.input_format]

        if '.' in parsed_args.dt_key:
            self.dt_key = parsed_args.dt_key.split('.')
            self.find_dt = LogParser._find_value_by_complex_key
        else:
            self.dt_key = parsed_args.dt_key
            self.find_dt = LogParser._find_value_by_simple_key

        with warnings.catch_warnings(record=True):
            self.timezone = gettz(parsed_args.timezone) or tzlocal()
        self.secret_regex = self.build_secret_regex(parsed_args.secrets) if parsed_args.secrets else None
        self.tracking_data = {
            'cluster': parsed_args.cluster,
            'hostname': socket.getfqdn(),
            'origin': self.origin,
            'log_format': environ.get('LOG_ENVIRONMENT', 'dbaas_int_log'),
        }

    def run(self, args=None, stream=sys.stdin):
        self.initialize(args=args)
        signal.signal(signal.SIGALRM, interrup_readline)
        signal.signal(signal.SIGUSR1, handle_SIGUSR1)
        global GOT_SIGUSR1
        for chunk in self.read_chunks(stream):
            record = self.parse_chunk(chunk)
            if record is not None:
                self.print_record(record)
            elif GOT_SIGUSR1:
                logging.info('finish run with SIGUSR1')
                break

    def parse_chunk(self, chunk) -> Optional[dict]:
        try:
            if chunk is None:
                return None
            chunk = self.cut_secrets(chunk)
            record = self.parse_record(chunk)
            if record is None:
                return None
            self.add_time_data(record)
            self.truncate_long_fields(record)
            record.update(self.tracking_data)
            return record
        except Exception as e:
            self.log_exception(e, chunk)

    def read_chunks(self, stream):
        chunk = ''
        while True:
            try:
                signal.alarm(CHUNK_WAIT_TIMEOUT)
                line = stream.readline(MAX_LENGTH)
                if not line:
                    break
                if len(line) == MAX_LENGTH and not line.endswith('\n'):
                    # read the tail to prevent errors on next iteration
                    while True:
                        tail = stream.readline(MAX_LENGTH)
                        if not tail or tail.endswith('\n'):
                            break
                signal.alarm(0)
                line = str.rstrip(line)
            except UnicodeDecodeError as err:
                # Ignore martian characters
                line = err.object.decode('utf8', 'replace')[:MAX_LENGTH]
            except ChunkReadTimeout:
                if chunk:
                    yield chunk
                    chunk = ''
                continue
            finally:
                signal.alarm(0)
            try:
                if self.is_record_start(line):
                    if chunk:
                        yield chunk
                    chunk = line
                elif chunk:
                    chunk += '\n' + line
                else:
                    chunk = line
            except Exception as e:
                self.log_exception(e, chunk)
                chunk = line
        if chunk:
            yield chunk

    def is_record_start(self, s) -> bool:
        return True

    def cut_secrets(self, record: str) -> str:
        if self.secret_regex is not None:
            record = re.sub(self.secret_regex, '<secret>', record)
        return record

    def parse_record(self, line: str) -> Optional[dict]:
        if self.parser is None:
            raise ValueError('You should override parse_record method or set specific parser')

        parts = line.split(';', 3)  # https://wiki.yandex-team.ru/logbroker/docs/push-client/config/#pipe
        if len(parts) != 4:
            return None

        raw_log_line = parts[3]
        parsed_record = self.parser(raw_log_line)
        return {
            self.PIPE_PREFIX: ';'.join(parts[:3]) + ';',
            self.REQUIRED_DT_KEY: self.find_dt(parsed_record, self.dt_key),
            'message': raw_log_line,
        }

    def print_record(self, record):
        """
        Print record in output format with pipe prefix.
        """
        pipe_prefix = record.pop(self.PIPE_PREFIX)
        serialized_record = self.serializer(record)
        print(pipe_prefix + serialized_record, flush=True)

    def parse_timestamp(self, dt):
        if not dt:
            return None
        dt = parse_datetime(dt)
        if dt.tzinfo is None:
            dt = dt.replace(tzinfo=self.timezone)
        dt = dt.astimezone(tz=timezone.utc)
        return dt.timestamp()

    def add_time_data(self, record: dict):
        """
        Update the record with required time data.
        """
        timestamp = self.parse_timestamp(record[self.REQUIRED_DT_KEY])
        record.update(
            {
                'timestamp': int(timestamp),
                'ms': '%d' % (timestamp % 1 * 1000),
            }
        )

    @staticmethod
    def _find_value_by_complex_key(record, complex_key):
        data = record
        for key in complex_key:
            data = data.get(key)
            if data is None:
                return None
        return data

    @staticmethod
    def _find_value_by_simple_key(record, simple_key):
        return record.get(simple_key)

    def truncate_long_fields(self, record):
        for k, v in record.items():
            if isinstance(v, (str, bytes)) and len(v) > MAX_FIELD_LENGTH:
                record[k] = v[:MAX_FIELD_LENGTH]

    def log_exception(self, exc, log_record=''):
        """
        Log exception if not recently logged. Should be called inside except clause.
        """
        file, line = '', 0
        _, _, traceback = sys.exc_info()
        while traceback is not None:
            if "lib/python" not in traceback.tb_frame.f_code.co_filename:
                file = traceback.tb_frame.f_code.co_filename
                line = traceback.tb_lineno
            traceback = traceback.tb_next
        key = '{}:{}:{}'.format(exc.__class__.__name__, file, line)
        last_err = self.errors.get(key) or LoggedError(ts=datetime(1970, 1, 1), count=0)
        now = datetime.now()
        if now - last_err.ts < ERROR_COOLDOWN:
            self.errors[key] = LoggedError(ts=last_err.ts, count=last_err.count + 1)
        else:
            logging.error('Failed to handle log record: %s', log_record[:ERROR_MAX_LOG_LEN])
            logging.exception(exc)
            if last_err.count > 0:
                logging.error('There was %d such errors during last %s', last_err.count + 1, now - last_err.ts)
            self.errors[key] = LoggedError(now, count=0)

    def build_secret_regex(self, secrets_file):
        """ """
        with open(secrets_file) as fh:
            lines = [l.strip() for l in fh.readlines() if l.strip()]
            if lines:
                return re.compile('|'.join(lines))
        return None

    def cut_key(self, line: str) -> str:
        key_match = self._key_pattern.match(line)
        if not key_match:
            raise Exception("line without a key: >{}<".format(line))
        key_match.group('key')
        return line[key_match.end() :]

    def extract_raw(self, record: str):
        lines = record.split('\n')
        raw = ''
        for line in lines:
            if not line:
                continue
            if raw != '':
                raw += '\n'
            raw += self.cut_key(line)
        return raw


if __name__ == '__main__':
    parser = LogParser()
    parser.run()
