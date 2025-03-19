#!/usr/bin/env python
# encoding: utf-8

import subprocess
from ConfigParser import RawConfigParser
import sys
import traceback
import logging
import re
import datetime
import os
import io

today = datetime.date.today()


def parsePattern(pattern):
    ret = {'files': []}
    sections = pattern.split()
    for section in sections:
        if section.startswith('regex:'):
            ret['regex'] = re.compile(section.split(':')[1])
        elif section.startswith('min_days:'):
            ret['days'] = float(section.split(':')[1])
        elif section.startswith('group'):
            ret['group'] = section.split(':')[1]

    for i in ['regex', 'days', 'group']:
        if i not in ret:
            raise Exception('Missing %s in pattern %s' % (i, pattern))

    return ret


def getDays(name):
    m = daysRe.match(name)
    if m:
        date = datetime.datetime.strptime(m.group(2), '%Y%m%d').date()
        days = (today - date).days
        return days
    else:
        return 0


def isDone(cmd):
    p = subprocess.Popen(cmd, bufsize=65535, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, shell=True)
    ret = p.wait()
    return ret == 0

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print 'Usage: ' + sys.argv[0] + ' <config> <baseDir>'
        sys.exit(1)
    config = RawConfigParser()
    config.read(sys.argv[1])
    baseDir = sys.argv[2]

    log = logging.getLogger('log rotator')
    log.setLevel(config.get('main', 'log_level').upper())
    _format = logging.Formatter("%(asctime)s [%(levelname)s] %(name)s:\t%(message)s")
    _handler = logging.FileHandler(
        '/var/log/logstore-rotator-%s.log' % baseDir)
    _handler.setFormatter(_format)
    _handler.setLevel(config.get('main', 'log_level').upper())
    log.addHandler(_handler)

    fileListCmd = config.get('main', 'file_list_cmd').replace('mount_p', baseDir)
    defaultDays = float(config.get('main', 'default_min_days'))
    batchSize = config.getint('main', 'batch_size')
    doneCheckCmd = config.get('main', 'done_check_cmd').replace('mount_p', baseDir)
    daysRe = re.compile(r'(.+?)([0-9]{8})(.+)')
    patterns = {'default': {'days': defaultDays,
                            'group': '',
                            'files': []}}
    try:
        for i in config.items('patterns'):
            if i != 'default':
                patterns[i[0]] = parsePattern(i[1])
            else:
                raise Exception('default pattern is reserved')
    except Exception:
        for line in traceback.format_exc().splitlines():
            log.error(line)
        sys.exit(1)

    if isDone(doneCheckCmd):
        log.info('Nothing to do')
        sys.exit(0)

    try:
        p = subprocess.Popen(fileListCmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, shell=True)
        out = io.open(p.stdout.fileno())
        while p.poll() is None:
            f = out.readline()
            if not f:
                continue
            fn = f.rstrip()
            log.debug('Processing ' + fn)
            flist = fn.split('/')
            if flist[-1] == 'mds-logbackup-tmp-file':
                continue
            elif len(flist) == 6:
                (_, baseDir, archLogs, group, host, name) = flist
            else:
                log.error('Unable to parse: ' + fn)
                continue
            days = getDays(name)
            for i in sorted(patterns.keys()):
                matched = False
                if patterns[i]['group'] in [group, 'all']:
                    if patterns[i]['regex'].match(name):
                        if days > patterns[i]['days']:
                            patterns[i]['files'].append({'name': fn,
                                                         'days': days})
                        matched = True
                        break
            if not matched:
                if days > defaultDays:
                    patterns['default']['files'].append({'name': fn,
                                                         'days': days})
        p.wait()
        del p
        procList = []
        for i in sorted(patterns.keys(), key=lambda x: patterns[x]['days']):
            log.info('Processing %d files (pattern %s)' %
                     (len(patterns[i]['files']), i))
            flist = sorted(patterns[i]['files'], key=lambda x: x['days'],
                           reverse=True)
            for f in flist:
                if len(procList) < batchSize:
                    procList.append(f['name'])
                else:
                    procList.append(f['name'])
                    for i in procList:
                        log.info('Removing ' + i)
                        try:
                            os.unlink(i)
                            # pass
                        except Exception:
                            log.error('Unable to remove ' + i)
                            for line in traceback.format_exc().splitlines():
                                log.error(line)
                    if isDone(doneCheckCmd):
                        log.info('Done')
                        sys.exit(0)
                    procList = []
    except Exception:
        for line in traceback.format_exc().splitlines():
            log.error(line)
        sys.exit(1)
