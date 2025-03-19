#!/usr/bin/env python

import argparse
import fnmatch
import os
import sys
import time


SENDER_TEMPLATE = '{{ salt.mdb_metrics.get_sender_template() }}'
WARN = {{ salt.mdb_metrics.get_sender_warn_limit() }}
CRIT = {{ salt.mdb_metrics.get_sender_crit_limit() }}


class Template:
    def __init__(self, template):
        self.template = template
        self.dir = os.path.dirname(self.template)
        self.glob = self.template.format('*')
        self.start_len, self.end_len = map(len, self.template.split('{}'))

    def get_sender(self, path):
        return path[self.start_len : -self.end_len]


def ok():
    die(0)


def warn(comment):
    die(1, comment)


def crit(comment):
    die(2, comment)


def die(code, comment="OK"):
    print('%d;%s' % (code, comment))
    sys.exit(0)


def mtime_diff(path):
    mtime = os.stat(path).st_mtime if os.path.exists(path) else 0
    return time.time() - mtime


def _get_state_files(sender_state_template):
    files = [os.path.join(sender_state_template.dir, file) for file in os.listdir(sender_state_template.dir)]
    state_files = fnmatch.filter(files, sender_state_template.glob)
    if len(state_files) == 0:
        ok()
    return state_files


def _get_senders(state_files, sender_state_template, warn_timeout, crit_timeout):
    warns = []
    crits = []
    for state_file in state_files:
        diff = mtime_diff(state_file)
        if diff < warn_timeout:
            continue
        sender = sender_state_template.get_sender(state_file)
        if diff >= crit_timeout:
            crits.append(sender)
            continue
        if diff >= warn_timeout:
            warns.append(sender)
    return crits, warns


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-s', '--sender-template', default=SENDER_TEMPLATE, help='Sender template ({}-style)')
    parser.add_argument(
        '-w', '--warn', type=int, default=WARN, help='Warning limit (seconds)')
    parser.add_argument(
        '-c', '--crit', type=int, default=CRIT, help='Critical limit (seconds)')
    return parser.parse_args()


def get_msg(timeout, llist):
    return "{}+ sec old={}".format(timeout, llist)


def _main():
    args = get_args()
    try:
        template = Template(args.sender_template)
        state_files = _get_state_files(template)
        crits, warns = _get_senders(state_files, template, args.warn, args.crit)
        if crits:
            msg = get_msg(args.crit, crits)
            if warns:
                msg = "; ".join([msg, get_msg(args.warn, warns)])
            crit(msg)
        if warns:
            msg = get_msg(args.warn, warns)
            warn(msg)
        ok()
    except Exception as exc:
        warn(exc)


if __name__ == '__main__':
    _main()
