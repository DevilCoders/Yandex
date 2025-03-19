#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import os.path
import sys
import time
import socket

sys.path.append(os.path.join(os.path.dirname(__file__), '../'))

from util.database import Database
from util.pgmeta import PgmetaDB
from util.helpers import init_logging, read_config

CLOSER_DEFAULT_FILEPATH = '/var/lib/postgresql/.s3_closer'


def parse_cmd_args():
    usage = """
    %prog [-p pgmeta_connstring] [-d db_connstring] [-u user] [-l log_level] open|close|check [comment]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-p", "--pgmeta-connstring", dest="pgmeta_connstring", default="dbname=pgmeta",
                      help="Connection string to pgmeta database (default: %default)")
    parser.add_option("-d", "--db-connstring", dest="db_connstring",
                      help="Connection string to s3db/s3meta database")
    parser.add_option("-u", "--user", dest="user",
                      help="User which will be used for authentication in s3db database")
    parser.add_option("--dry-run", dest="dry_run", default=False, action='store_true',
                      help="Only check closing possibility")
    parser.add_option("--force", dest="force", default=False, action='store_true',
                      help="If you know what you're doing")
    parser.add_option("-c", "--crit", dest="crit_delay", default=6 * 60 * 60,
                      help="CRIT delay in seconds for check command")
    parser.add_option("--crit-count", dest="crit_count", type="int", default=1,
                      help="CRIT if closed hosts count >")
    parser.add_option("--closer-filepath", dest="closer_filepath", default=CLOSER_DEFAULT_FILEPATH,
                      help="Closer file path for checking timestamp and store comment")
    return parser.parse_args()


def _touch_closer_file(closer_filepath, comment=''):
    if not os.path.exists(closer_filepath):
        with open(closer_filepath, 'w') as f:
            f.write(comment)


def _rm_closer_file(closer_filepath):
    if os.path.exists(closer_filepath):
        os.remove(closer_filepath)


def _get_file_lifetime_and_content(closer_filepath):
    closed_time = 0
    comment = ''
    if os.path.exists(closer_filepath):
        closed_time = time.time() - os.path.getctime(closer_filepath)
        with open(closer_filepath, 'r') as f:
            comment = f.read()
    return closed_time, comment


def main():
    options, args = parse_cmd_args()
    if not args or args[0] not in {'open', 'close', 'check'}:
        logging.error('Bad command: %s' % args)
        sys.exit(1)

    config = read_config(options=options)
    init_logging(config)

    try:
        db = Database(config['db_connstring'], autocommit=True)
        pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True)
        hostname = socket.gethostname()
        crit_count = config.get('crit_count')
        closer_filepath = config.get('closer_filepath')
    except Exception as e:
        if args[0] == 'check':
            print('1;Error while checking: %s' % str(e).replace('\n', ' '))
            exit(1)
        else:
            raise

    if args[0] == 'open':
        if not os.path.exists(closer_filepath):
            logging.info('Closer file not found')
        else:
            _rm_closer_file(closer_filepath)
        db.open()
        logging.info('Opened')
    elif args[0] == 'close':
        force = config.get('force')
        if os.path.exists(closer_filepath):
            if db.check_is_open():
                logging.warning('File exists, but db is open')
                if not config.get('dry_run'):
                    db.close()
                    logging.info('Closed')
            else:
                logging.info('Already closed')
        elif not force and db.is_master():
            logging.error("Cannot close because I'm master.")
            exit(1)
        elif not force and len(pgmeta.get_closed_hosts_in_cluster(hostname, user=config.get('user'))) >= crit_count:
            closed_hosts = pgmeta.get_closed_hosts_in_cluster(hostname, user=config.get('user'))
            logging.error('Cannot close because closed >= %d host(s): %s', crit_count, ', '.join(closed_hosts))
            exit(1)
        elif not config.get('dry_run'):
            db.close()
            _touch_closer_file(closer_filepath, args[1] if len(args) > 1 else '')
            logging.info('Closed')
        else:
            logging.info('Possible to close')
    else:   # check
        try:
            if db.check_is_open():
                print('0;Host is open')
            else:
                closed_time, comment = _get_file_lifetime_and_content(closer_filepath)
                monrun_code = '1'
                if closed_time == 0:
                    err_messages = ['Host is not available']
                else:
                    err_messages = ['Closed %d seconds' % closed_time]
                if closed_time > config.get('crit_delay'):
                    monrun_code = '2'
                if db.is_master():
                    monrun_code = '2'
                    err_messages.append('master')
                closed_hosts = pgmeta.get_closed_hosts_in_cluster(hostname, user=config.get('user'))
                if len(closed_hosts) > crit_count:
                    monrun_code = '2'
                    err_messages.append('closed more than %d host(s): %s' % (crit_count, ', '.join(closed_hosts)))
                if comment:
                    err_messages.append('reason: %s' % comment)
                print('%s;%s' % (monrun_code, ', '.join(err_messages)))
        except Exception as e:
            print('1;Error while checking: %s' % str(e).replace('\n', ' '))

if __name__ == '__main__':
    main()
