#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import argparse
import enum
import logging
import subprocess
import requests
from datetime import date
from logging.handlers import RotatingFileHandler

JUGGLER_AGENT_PORT = 31579
BACKUP_DIR = '/var/opt/tableau/tableau_server/data/tabsvc/files/backups'


class TableauBackupStatus(enum.Enum):
    success = 0
    failure = 1


def parse_args():
    parser = argparse.ArgumentParser(description='Make Tableau backups')
    parser.add_argument('--full', action='store_true', help='Toggle full backup mode')
    parser.add_argument('--l', type=str, required=False, default='/var/log/tableau-backups',
                        help='Backup logs directory')
    parser.add_argument('--s3bucket', type=str, required=False, default='media-tableau-backups', help='S3 bucket name')
    return parser.parse_args()


def setup_logging(args):
    logs_dir = os.path.normpath(args.l)
    log_file = logs_dir + '/tableau.log'
    logging.basicConfig(
        format='%(asctime)s %(levelname)s  %(message)s',
        handlers=[RotatingFileHandler(log_file, maxBytes=20000000, backupCount=5)],
        level=logging.DEBUG,
        datefmt='%Y-%m-%dT%H:%M:%S')


def run_command(cmd):
    """given shell command, returns communication tuple of stdout and stderr"""
    logging.info('attempt to execute cmd: %s' % cmd)
    p = subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         stdin=subprocess.PIPE)
    output = p.stdout.read().decode('utf-8').strip()
    logging.info(output)
    exit_code = p.returncode
    return exit_code


def s3upload(filepath, bucket):
    s3cmd = 's3cmd put %s s3://%s/' % (filepath, bucket)
    run_command(s3cmd)


def start_tableau_backup(args):
    backup_cmd = 'tsm maintenance backup -d -f tableau --ignore-prompt'
    today_str = date.today().strftime('%Y-%m-%d')
    if not args.full:
        backup_cmd += ' --pg-only'
    backup_filename = '/tableau-%s.tsbak' % today_str
    run_command(backup_cmd)
    backup_file_abspath = os.path.normpath(BACKUP_DIR + backup_filename)
    return backup_file_abspath


def send_juggler_event(backup_status):
    fqdn = os.uname()[1]
    req = {
        'source': 'tableau_backup.py',
        'events': [
            {
                'description': 'tableau s3 backup status',
                'host': 'media-dwh-stable-tableau',
                'instance': fqdn,
                'service': 's3backups',
                'status': ''
            }]
    }
    if backup_status == TableauBackupStatus.success:
        logging.info('sending ok message to juggler')
        req['events'][0]['status'] = 'OK'
    else:
        logging.info('sending fail message to juggler')
        req['events'][0]['status'] = 'CRIT'
    requests.post('http://localhost:%d/events' % JUGGLER_AGENT_PORT,
                  json=req,
                  timeout=1)


if __name__ == "__main__":
    args = parse_args()
    setup_logging(args)
    logging.info('backup started')
    try:
        backup_file = start_tableau_backup(args)
        logging.debug(backup_file)
        s3upload(backup_file, args.s3bucket)
        logging.info('delete local backup: %s' % backup_file)
        os.remove(backup_file)
        send_juggler_event(TableauBackupStatus.success)
        logging.info('backup finished')
    except Exception as e:
        logging.error('backup failed %s' % e)
        send_juggler_event(TableauBackupStatus.failure)
