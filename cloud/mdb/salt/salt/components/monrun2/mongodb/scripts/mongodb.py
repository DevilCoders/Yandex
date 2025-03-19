#!/usr/bin/env python3

import argparse
import os
import sys
from collections import namedtuple
from datetime import datetime, timedelta
import warnings

import pymongo

# Ignore ssl-related warnings if any
warnings.simplefilter("ignore")

Result = namedtuple('Result', 'code, message')
RS_STATE = '/tmp/mongodb_monitoring_rs'
PROFILER_STATE = '/tmp/mongodb_monitoring_slowlog'
USERFAULT_BROKEN_FLAG_FILE = '/tmp/load-monitor-userfault.flag'
ACTIONLOG_ERRORS_MIN_AGO = os.getenv('MONGO_ACTIONLOG_ERRORS_MIN_AGO', 20)

DEFAULT_TIMEOUT_MS = 5000


class Status(object):
    """ Class for holding Juggler status """
    code = 0
    text = []

    def set_code(self, new_code):
        """ Set the code if it is greater than the current. """
        if new_code > self.code:
            self.code = new_code

    def append(self, new_text):
        """Accumulate the status text"""
        self.text.append(new_text)

    def report(self, code=0, message=None):
        """ Output formatted status message"""
        # concatenate all received statuses
        if message is None:
            message = '. '.join(self.text)
        if not message and self.code == 0:
            message = 'ok'
        # Check if code is above current setting
        self.set_code(code)
        # strip underscores and newlines.
        print('%d;%s' % (self.code, message.replace('_', ' ').replace('\n', '').replace('mail.yandex.net', 'm')))
        sys.exit(0)


class CheckMongoDB(object):
    def __init__(self, uri=None, db=None):
        self.con = self.__connect(uri)
        self.db = db
        self.uri = uri

    def __connect(self, uri):
        return pymongo.MongoClient(
            uri,
            socketTimeoutMS=DEFAULT_TIMEOUT_MS,
            connectTimeoutMS=DEFAULT_TIMEOUT_MS,
            serverSelectionTimeoutMS=DEFAULT_TIMEOUT_MS,
            waitQueueTimeoutMS=DEFAULT_TIMEOUT_MS,
        )

    def locked_queue(self, crit=None, warn=None):
        code = 0
        if crit is None:
            crit = 20
        if warn is None:
            warn = 10

        cmd = self.con['admin'].command('serverStatus', metrics=0, recordStats=0, locks=1)
        active = cmd['globalLock']['currentQueue']['total']

        if active >= crit:
            code = 2
        elif active >= warn:
            code = 1
        else:
            code = 0
        return Result(code=code, message='%d reqs' % active)

    def lag(self, crit=None, warn=None):
        code = 0
        if crit is None:
            crit = 100
        if warn is None:
            warn = 20
        cmd = self.con['admin'].command('replSetGetStatus')
        current_instance = {}
        primary = None
        # Get shortcuts.
        for state in cmd['members']:
            if 'self' in state.keys():
                current_instance = state
            if state['stateStr'] == 'PRIMARY':
                primary = state

        if current_instance['stateStr'] == 'PRIMARY':
            return Result(code=0, message='OK')

        if current_instance.get('errmsg') is not None:
            return Result(code=2, message=current_instance.get('errmsg'))

        if primary is None:
            # Do not raise CRIT in case of no master
            return Result(code=1, message="Primary not found")

        diff = primary['optimeDate'] - current_instance['optimeDate']
        diff_secs = (diff.days * 86400 + diff.seconds)
        if diff_secs > 1500000000:
            return Result(code=1, message='initial sync in progress')
        if diff_secs >= crit:
            code = 2
        elif diff_secs >= warn:
            code = 1
        return Result(code=code, message='%d sec' % diff_secs)

    def up(self, **_):
        self.con['admin'].command('ping')
        return Result(code=0, message='OK')

    def master(self, **_):
        cmd = self.con['admin'].command('replSetGetStatus')
        primaries_cnt = 0
        for state in cmd['members']:
            if state['stateStr'] == 'PRIMARY':
                primaries_cnt += 1

        if primaries_cnt > 1:
            return Result(code=2, message="%d primaries in rs" % primaries_cnt)
        elif primaries_cnt < 1:
            retcode = 2
            if len(cmd['members']) < 3:
                # If less than 3 hosts in RS, then warning instead of CRIT
                retcode = 1
            return Result(code=retcode, message="no primaries in rs")
        return Result(code=0, message='OK')

    def role(self, **_):
        role = 'UNKNOWN'
        my_state = {}
        try:
            role = open(RS_STATE).read().strip()
        except Exception:
            pass

        cmd = self.con['admin'].command('replSetGetStatus')
        for state in cmd['members']:
            if 'self' in state.keys():
                my_state = state
        # No changes.
        if my_state['stateStr'] == role:
            return Result(code=0, message='OK')
        # Master -> Secondary. Critical
        if role == 'PRIMARY' and my_state['stateStr'] != 'PRIMARY':
            with open(RS_STATE, 'w') as fobj:
                fobj.write(my_state['stateStr'])
            return Result(code=2, message='%s->%s' % (role, my_state['stateStr']))
        # Secondary -> anything else. A warning.
        if role != 'PRIMARY':
            with open(RS_STATE, 'w') as fobj:
                fobj.write(my_state['stateStr'])
            return Result(code=1, message='%s->%s' % (role, my_state['stateStr']))

    def slowlog(self, crit=None, warn=None):
        if crit is None:
            crit = 100000
        if warn is None:
            warn = 100

        curr_slow_ops = self.con['admin'][self.db]['system.profile'].count()
        with open(PROFILER_STATE, 'w') as fobj:
            if str(curr_slow_ops).isdigit():
                fobj.write(str(curr_slow_ops))
        if curr_slow_ops >= crit:
            code = 2
        elif curr_slow_ops >= warn:
            code = 1
        else:
            code = 0
        return Result(code=code, message='%d slow ops at "%s"' % (int(curr_slow_ops), self.db))

    def actionlog_errors(self, crit=None, warn=None):
        if crit is None:
            crit = 5
        if warn is None:
            warn = 0

        min_time = datetime.now() - timedelta(minutes=ACTIONLOG_ERRORS_MIN_AGO)
        errors_cnt = self.con['config']['actionlog'].find({
            'details.errorOccured': True,
            'time': {
                '$gt': min_time,
            },
        }).count()

        code = 0
        if errors_cnt > crit:
            code = 2
        elif errors_cnt > warn:
            code = 1
        return Result(
            code=code,
            message='%d errors in config.actionlogs during last %d minutes' % (errors_cnt, ACTIONLOG_ERRORS_MIN_AGO))


if __name__ == '__main__':
    arg = argparse.ArgumentParser(description="""
            MongoDB checker.
            """)
    arg.add_argument('-c', '--critical', type=int, required=False, metavar='<integer>', help='critical threshold')
    arg.add_argument('-w', '--warning', type=int, required=False, metavar='<integer>', help='warning threshold')
    arg.add_argument('-a', '--action', type=str, required=True, help='perform these check')
    arg.add_argument(
        '-d',
        '--db',
        required=False,
        type=str,
        default='admin',
        metavar='<str>',
        help='use this database when applicable')
    arg.add_argument('-u', '--uri', required=True, type=str, metavar='<str>', help='URI to use when connecting')
    arg.add_argument('-s', '--silent', action='store_true', help='Silent check, warn on check failure')
    settings = vars(arg.parse_args())

    action = settings.get('action')
    status = Status()
    try:
        if os.path.exists(USERFAULT_BROKEN_FLAG_FILE):
            status.set_code(0)
            status.append('Muted by userfault flag')
        else:
            check = CheckMongoDB(uri=settings.get('uri'), db=settings.get('db'))
            func = getattr(check, action)
            result = func(crit=settings.get('critical'), warn=settings.get('warning'))
            status.append('%s: %s' % (action, result.message))
            status.set_code(result.code)
    except AttributeError as exc:
        status.append('%s: %s' % (action, exc))
        status.set_code(2)
    except Exception as exc:
        # raise
        status.append('%s: %s' % (action, exc))
        status.set_code(1 if settings.get('silent') else 2)

    status.report()
