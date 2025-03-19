#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import logging
import logging.handlers
import os
import os.path
import subprocess

import MySQLdb
import MySQLdb.cursors as cursors


HIGH_PORT = 3308


def get_logger():
    """
    Initialize logger
    """
    logging.basicConfig()
    log = logging.getLogger()
    log.setLevel(logging.DEBUG)
    return log


def run_mysql_on_high_port(log, defaults_file, high_port=HIGH_PORT):
    """
    runs mysql on high port
    """
    if subprocess.call('pgrep mysqld', shell=True) == 0:
        log.error('MySQL proc is already running')
        raise Exception('MySQL proc is already running')

    possible_options = str(subprocess.check_output(['/usr/sbin/mysqld', '--help', '--verbose']))
    if 'admin-port' in possible_options:
        admin_port_option = 'admin-port'
    else:
        admin_port_option = 'extra-port'

    log.info('running MySQL on high port')
    cmd = ('/usr/sbin/mysqld '
        '--socket=/tmp/mysqld.sock '
        '--pid-file=/tmp/mysql.pid '
        '--super-read-only=0 '
        '--default-time-zone=SYSTEM '
        '--port={high_port} '
        '--{admin_port_option}=0 '
        '--disabled_storage_engines="" '
    ).format(high_port=high_port, admin_port_option=admin_port_option)
    log.info("executing cmd: %s", cmd)

    return subprocess.Popen(cmd, shell=True)


def run_upgrade(log, defaults_file, high_port=HIGH_PORT):
    log.info("running upgrade")
    cmd = """
        mysql_upgrade --defaults-file={mycnf} --port={port} mysql
    """.format(mycnf=defaults_file, port=high_port).strip()
    log.info("executing cmd: %s", cmd)
    subprocess.call(cmd, shell=True)


def shutdown_mysql_on_high_port(proc):
    """
    Shutdowns mysql
    """
    if proc is None:
        return

    # send SIGTERM to MySQL to graceful shutdown
    proc.terminate()
    proc.wait()

    if proc.returncode != 0:
        raise Exception('failed to shutdown mysql on high port')


def main():
    """
    Console entry-point
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--defaults-file',
        type=str,
        default=os.path.expanduser("~/.my.cnf"),
        help='path for my.cnf with login/password for cluster',
    )
    parser.add_argument(
        '--port',
        type=int,
        default=HIGH_PORT,
        help='port for connect to local mysql',
    )

    args = parser.parse_args()
    log = get_logger()
    proc = None
    try:
        proc = run_mysql_on_high_port(log, args.defaults_file, args.port)

        cmd = 'my-wait-started -v --defaults-file={mycnf} --port={port}'.format(mycnf=args.defaults_file, port=args.port)
        if subprocess.call(cmd, shell=True) != 0:
            raise Exception('failed to run mysql on high port')

        run_upgrade(log, args.defaults_file, args.port)
    except Exception as e:
        log.exception('Unable to fill timezones table in mysql due %s', e)
        raise
    finally:
        shutdown_mysql_on_high_port(proc)


if __name__ == '__main__':
    main()
