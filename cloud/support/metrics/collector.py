#!/usr/bin/env python3
"""This module contains CLI core for MetricsColletor."""

import re
import os
import sys
import time
import logging
import argparse

from yaml import load, Loader
from logging.handlers import RotatingFileHandler

from metrics_collector import MetricsCollector
from metrics_collector.helpers import string_delta_time
from metrics_collector.constants import (DEFAULT_INTERVAL, BASE_FILTER,
                                         DEFAULT_MAX_LOGFILE_SIZE,
                                         DEFAULT_LOG_BACKUP_COUNT)
from metrics_collector.database import Database
from metrics_collector.error import ConfigError

RUNNING = True

parser = argparse.ArgumentParser(description='Support issue metrics collector.')
parser.add_argument('--version', action='version', version='collector 0.1.3')
parser.add_argument('-i', '--issue', type=str, metavar='key', help='print issue comments. key must be CLOUDSUPPORT-123')
parser.add_argument('-inc', '--incident', type=str, metavar='key', help='show cloudinc metadata')
parser.add_argument('-e', '--export', action='store_true', help='export comments to CSV file')
parser.add_argument('--clean', action='store_true', help='clean all non support comments from DB')
parser.add_argument('-r', '--run-forever', action='store_true', help='run forever collector with upload to DB')
parser.add_argument('-l', '--write-logs', action='store_true', help='write logs to /var/log/ or path from config.')
parser.add_argument('--config', type=str, metavar='file', help='path to config file')
parser.add_argument('--interval', type=int, metavar='int', help='interval in secs for `run-forever`')
parser.add_argument('--filter', type=str, metavar='str', help='queue filter, like a ' + \
    '"Queue: CLOUDSUPPORT and Updated: >= now() - 1d"')

args = parser.parse_args()
homedir = os.path.expanduser('~')
config_file = args.config or os.path.join(homedir, '.yc-support', 'config.yaml')


class Config:
    try:
        with open(config_file, 'r') as cfgfile:
            config = load(cfgfile, Loader=Loader) or {}
    except FileNotFoundError:
        raise ConfigError('Config file not found')
    except TypeError:
        raise ConfigError('Corrupted config file or bad format')

    # Startrek
    FILTER = config.get('startrek', {}).get('filter')
    TOKEN = config.get('startrek', {}).get('token')
    if TOKEN is None:
        raise ConfigError('OAuth token is empty. Get startrek token here: ' + \
            'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=a7597fe0fbbf4cd896e64f795d532ad2')

    # Database
    HOST = config.get('database', {}).get('host')
    PORT = config.get('database', {}).get('port')
    USER = config.get('database', {}).get('user')
    PASSWD = config.get('database', {}).get('passwd')
    DB_NAME = config.get('database', {}).get('db_name')
    CA_PATH = config.get('database', {}).get('ca_path')

    # Logs
    LOGPATH = config.get('logs', {}).get('path') or os.path.join('/', 'var', 'log')
    if LOGPATH is not None and not LOGPATH.endswith('.log'):
        LOGPATH = os.path.join(LOGPATH, 'metrics_collector.log')
    LOGLEVEL = config.get('logs', {}).get('level') or 'info'
    LOGLEVEL = getattr(logging, LOGLEVEL.upper())
    if not isinstance(LOGLEVEL, int):
        raise ValueError('loglevel must be: debug/info/warning/error')
    if LOGLEVEL >= 10:
        logging.getLogger('yandex_tracker_client').setLevel(logging.WARNING)
    MAX_LOGFILE_SIZE = config.get('logs', {}).get('max_size') or DEFAULT_MAX_LOGFILE_SIZE
    LOG_BACKUP_COUNT = config.get('logs', {}).get('backup_count') or DEFAULT_LOG_BACKUP_COUNT


log_handlers = [logging.StreamHandler()]
if args.write_logs:
    log_handlers.append(RotatingFileHandler(Config.LOGPATH,
        maxBytes=Config.MAX_LOGFILE_SIZE, encoding='utf8', backupCount=Config.LOG_BACKUP_COUNT))

logging.basicConfig(level=Config.LOGLEVEL, datefmt='%d/%b/%y %H:%M:%S', handlers=log_handlers,
    format='[%(asctime)s] [%(levelname)s] %(filename)s:%(lineno)s – %(message)s')
logger = logging.getLogger(__name__)


def _init_collector(db_client=None, token=None, st_filter=None):
    st_filter = st_filter or Config.FILTER
    collector = MetricsCollector(token=token, st_filter=st_filter, db_client=db_client)
    return collector


def _database(host=None, port=None, db_name=None, user=None, passwd=None, ssl=None):
    host = host or Config.HOST
    port = port or Config.PORT
    db_name = db_name or Config.DB_NAME
    user = user or Config.USER
    passwd = passwd or Config.PASSWD
    ssl = ssl or Config.CA_PATH

    return Database(host=host, port=port, db_name=db_name,
        user=user, passwd=passwd, ssl=ssl)


def issue_comments(issue, token=None):
    if re.search(u'st.yandex-team.ru', issue):
        issue = issue.split('/')[-1]

    collector = _init_collector(token=token)
    collector.print_comments_as_table(issue)


def export_comments(path=None, token=None, st_filter=None):
    st_filter = st_filter or Config.FILTER
    logging.info(f'Starting collect issues with filter: "{st_filter}"')
    collector = _init_collector(token=token, st_filter=st_filter)
    collector.export_comments_to_csv(path=path)


def run_forever(interval=None, token=None, st_filter=None):
    interval = interval or DEFAULT_INTERVAL
    st_filter = st_filter or BASE_FILTER
    timeout = 0

    while RUNNING:
        if time.time() < timeout:
            time.sleep(0.5)
            continue

        start_time = time.time()
        collector = _init_collector(token=token, st_filter=st_filter, db_client=_database())
        collector.run()

        logger.info('===============================================================================')
        logger.info(f'The collection is completed. Elapsed time: {string_delta_time(time.time() - start_time)}. ' + \
                    f'Going to sleep for {string_delta_time(interval)}...')
        logger.info('===============================================================================')
        timeout = time.time() + int(interval)


def clear_non_support_comments():
    from metrics_collector.constants import SUPPORTS

    db = _database()
    sql = f'DELETE FROM `comments` ' + \
          f'WHERE author NOT IN {SUPPORTS}'

    return db.query(sql, close=True)


# FIXME: print as table
def show_incident(issue, token=None):
    if re.search(u'st.yandex-team.ru', issue):
        issue = issue.split('/')[-1]

    collector = _init_collector(token=token)
    collector.print_incident(issue)

def main():
    st_filter = args.filter or BASE_FILTER
    token = Config.TOKEN
    interval = int(args.interval) if args.interval else DEFAULT_INTERVAL

    if args.issue:
        issue_comments(args.issue, token=token)
    elif args.incident:
        show_incident(args.incident, token=token)
    elif args.export:
        export_comments(st_filter=st_filter, token=token)
    elif args.run_forever:
        run_forever(interval=interval, token=token, st_filter=st_filter)
    elif args.clean:
        clear_non_support_comments()


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        RUNNING = False
        print('')
        sys.exit('Aborted by SIGINT (Ctrl + C). Shutting down..')
