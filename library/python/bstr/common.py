from __future__ import print_function

import hashlib
import io
import json
import logging
import os
import random
import sys
import time
import ujson

from pygments import highlight, lexers, formatters
from six import ensure_text, ensure_binary

import library.python.oauth as lpo
import library.python.init_log as lpi


logger = logging.getLogger(__name__)


def ml(func, data):
    for l in data.split('\n'):
        func(l)


def shut_up(log):
    log = logging.getLogger(log)

    def ignore_stupid_decisions(*args, **kwargs):
        pass

    log.setLevel(logging.WARNING)
    log.setLevel = ignore_stupid_decisions


def init_log(args):
    if args.quiet:
        level = 'INFO'
    else:
        level = 'DEBUG'

    lpi.init_log(level=level, append_ts=args.ts)

    if hasattr(args, 'log_file') and args.log_file:
        rotate = False
        if hasattr(args, 'rotate_log_file') and args.rotate_log_file:
            rotate = True
        lpi.init_handler(level=(args.log_file_level or 'INFO'), append_ts=args.ts, log_file=args.log_file, rotate=rotate)

    shut_up('yt.packages.requests.packages.urllib3.connectionpool')
    shut_up('requests.packages.urllib3.connectionpool')
    shut_up('kikimr.public.sdk')


def init(args, init_logger=True):
    if init_logger:
        init_log(args)

    if args.spec:
        with open(args.spec, 'r') as f:
            spec = json.loads(f.read())
    else:
        spec = {}

    return spec


def run_verbose(func, verbose):
    if verbose:
        func()
    else:
        try:
            func()
        except Exception:
            logger.exception('Exception occured, STOP bstr')
            sys.exit(1)
        except KeyboardInterrupt:
            logger.error('KeyboardInterrupt')
            sys.exit(2)


def dump_json(data, pretty=True):
    if not pretty:
        return ujson.dumps(data, indent=4, ensure_ascii=False)
    return highlight(ensure_text(json.dumps(data, indent=4, sort_keys=True), 'UTF-8'), lexers.JsonLexer(), formatters.TerminalFormatter())


def iter_file_fast(f):
    with io.FileIO(f.fileno(), closefd=False) as stream:
        while True:
            part = stream.read(100000)

            if not part:
                break

            yield ensure_text(part)


def iter_lines(f):
    line = ''

    for part in iter_file_fast(f):
        for ch in part:
            if ch == '\n':
                yield line
                line = ''
            else:
                line += ch

    if line:
        yield line


def gen_entropy(n):
    m = random.randint(0, n)
    res = bytearray('a' * m + 'b' * (n - m))

    assert len(res) == n

    for i in range(0, 100):
        k = random.randint(0, len(res) - 1)
        l = random.randint(0, len(res) - 1)

        v = res[k]
        res[k] = res[l]
        res[l] = v

    return res


def get_common_token():
    return str(lpo.get_token('b0919fe7f38c42b1927f046a5d165b52', '3ce599ccf3ec4f8d846cdf0f28a4fbf6'))


def safe_get_token(get_token):
    def impl(args):
        tok = get_token(args)
        logger.info('will use %s token', hashlib.md5(ensure_binary(tok)).hexdigest())

        class Flt(logging.Filter):
            def filter(self, record):
                try:
                    record.msg = record.msg.replace(tok, '** CENSORED **')
                except Exception:
                    pass

                return True

        flt = Flt()

        for hndl in logging.root.handlers:
            hndl.addFilter(flt)

        return tok

    return impl


@safe_get_token
def get_yt_token(args):
    if args.token:
        return args.token

    if os.environ.get('YT_TOKEN'):
        return os.environ.get('YT_TOKEN')

    if os.environ.get('YT_SECURE_VAULT_BSTR_YT_TOKEN'):
        return os.environ.get('YT_SECURE_VAULT_BSTR_YT_TOKEN')

    return get_common_token()


@safe_get_token
def get_lb_token(args):
    if args.lb_token:
        return args.lb_token

    if os.environ.get('LB_TOKEN'):
        return os.environ.get('LB_TOKEN')

    if os.environ.get('YT_SECURE_VAULT_BSTR_LB_TOKEN'):
        return os.environ.get('YT_SECURE_VAULT_BSTR_LB_TOKEN')

    return get_common_token()


class BaseWeightEstimator:
    def get_final_weight(self, info):
        raise NotImplementedError

    def no_limit_condition(self, info_list, spec):
        return False

    def get_custom_labels(self, info):
        return {}


class UidStat(object):
    def __init__(self, count=0, ts=0):
        self.count = count
        self.ts = ts

    def inc(self):
        self.count += 1
        self.ts = time.time()
