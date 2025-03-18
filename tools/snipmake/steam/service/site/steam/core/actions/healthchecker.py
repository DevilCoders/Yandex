# -*- coding: utf-8 -*-

import os
import subprocess

from celery.task.control import inspect
from datetime import timedelta
from django.db import connection
from django.utils import timezone
from djcelery.models import PeriodicTask
from _mysql_exceptions import OperationalError

from core.hard.loghandlers import SteamLogger
from core.settings import (STORAGE_ROOT, TEMP_ROOT,
                           ACCESS_LOG_FILE, LOG_LINES, GOOD_HTTP500_RATE,
                           DATABASE_VARIABLES_CHECKER)
from ui.settings import STATIC_ROOT, CELERYBEAT_SCHEDULE


EPSILON = timedelta(minutes=10)


class SteamHealthError(Exception):
    def __init__(self, message, exception=None):
        self.message = message
        if exception is None:
            msg_to_log = message
        else:
            msg_to_log = ' '.join((message, 'because of exception',
                                  str(exception)))
        SteamLogger.error(msg_to_log, type='STEAM_HEALTH_ERROR')

    def __str__(self):
        return self.message

#used in /health and middleware checker
def check_database():
    try:
        cursor = connection.cursor()
        for variable in DATABASE_VARIABLES_CHECKER:
            cursor.execute("SHOW VARIABLES LIKE '%s'" % variable)
            for row in cursor.fetchall():
                if int(row[1]) < DATABASE_VARIABLES_CHECKER[variable]:
                    raise SteamHealthError(
                        'Variable %s is equal %s and less %d '
                        % (row[0], row[1], DATABASE_VARIABLES_CHECKER[variable])
                    )
    except OperationalError as ex:
        raise SteamHealthError('Database is not alive', ex)
    except SteamHealthError as ex:
        raise ex
    except Exception as ex:
        raise SteamHealthError('Database is not alive', ex.__class__)


def check_gluster():
    for gluster_dir in (STORAGE_ROOT, TEMP_ROOT):
        if not os.path.isdir(gluster_dir):
            raise SteamHealthError('Gluster is not alive')
    if len(os.listdir(STORAGE_ROOT)) == 0:
        raise SteamHealthError('Gluster\'s directory %s is empty' % STORAGE_ROOT)


# inspect.stats() throws IOError if broker is not available
# and None if celery daemon is not alive
def check_celeryd_and_rabbitmq():
    try:
        insp = inspect()
        stats = insp.stats()
        if stats is None:
            raise SteamHealthError('CeleryD is not alive')
    except IOError as ex:
        raise SteamHealthError('RabbitMQ is not alive', ex)


def check_celerybeat():
    check_time = timezone.now()
    for pt in PeriodicTask.objects.filter(
        interval_id__isnull=False
    ):
        if (
            pt.name in CELERYBEAT_SCHEDULE and pt.last_run_at is not None and
            pt.last_run_at + CELERYBEAT_SCHEDULE[pt.name]['schedule'] <
            check_time - EPSILON
        ):
            raise SteamHealthError('Celerybeat loses task %s' % pt.name)


def check_http500_rate():
    # execute
    # tail -n LOG_LINES ACCESS_LOG_FILE |
    #     grep -c -e "\"[^\"]*\"[[:space:]]\+500[[:space:]]"
    tail = subprocess.Popen(('tail', '-n', str(LOG_LINES), ACCESS_LOG_FILE),
                            stdout=subprocess.PIPE)
    grep = subprocess.Popen(('grep', '-c', '-e',
                             r'\"[^\"]*\"[[:space:]]\+500[[:space:]]'),
                            stdin=tail.stdout, stdout=subprocess.PIPE)
    tail.stdout.close()
    tail.wait()
    tail_rc = tail.returncode
    if tail_rc:
        raise SteamHealthError(
            'Can\'t access nginx log: tail return code is %d' % tail_rc
        )
    try:
        http500_count = int(grep.communicate()[0].strip())
    except ValueError as ex:
        raise SteamHealthError('Bad HTTP 500 rate', ex)
    if http500_count >= LOG_LINES * GOOD_HTTP500_RATE:
        raise SteamHealthError('Bad HTTP 500 rate: %d errors' % http500_count)
