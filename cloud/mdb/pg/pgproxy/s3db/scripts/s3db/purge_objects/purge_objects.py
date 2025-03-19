#!/usr/bin/env python
# encoding: utf-8

from contextlib import contextmanager
from optparse import OptionParser

import daemon.pidfile
import logging
import signal
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3db import S3DB
from util.helpers import init_logging, read_config


@contextmanager
def pidfile(pid_path, stop=False):

    pid_file = daemon.pidfile.TimeoutPIDLockFile(
        path=pid_path,
        acquire_timeout=1
    )

    if stop:
        pid = pid_file.read_pid()
        if pid is not None:
            os.kill(pid, signal.SIGINT)
        else:
            sys.stderr.write(
                "WARNING: pidfile '{0}' does not exist\n".format(
                    pid_path))
        sys.exit(1)
    else:
        pid_file.acquire()
        try:
            yield pid_file
        except KeyboardInterrupt:
            logging.warning('stopped by user')
            sys.exit(1)
        finally:
            pid_file.release()


def parse_cmd_args():
    usage = """
    %prog [-d db_connstring] [-l log_level]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3db",
                      help="Connection string to s3db database (default: %default)")
    parser.add_option("-b", "--batch-size", dest="batch_size", default="1000",
                      help="Batch size (default: %default)")
    parser.add_option("-t", "--ttl", dest="ttl", default="1 week",
                      help="Objects deleted more that 'ttl' time ago will be moved (default: %default)")
    parser.add_option("-p", "--pid-file", dest="pid_file", default="/var/run/s3/purge_objects.pid",
                      help="Pid file (default: %default)")
    parser.add_option("--stop", action="store_true")
    return parser.parse_args()


def start(config):
    with pidfile(config['pid_file'], config.get('stop')):
        logging.info("Starting %s", os.path.basename(__file__))
        mydb = S3DB(config['db_connstring'], autocommit=True)
        if not mydb.is_master():
            logging.info("My db is replica. Nothing to do. Exiting...")
            sys.exit(0)
        batch_size = config['batch_size']
        total_objects_cnt = 0
        total_parts_cnt = 0
        moved_objects_cnt = batch_size
        moved_parts_cnt = batch_size
        while moved_objects_cnt > 0 or moved_parts_cnt > 0:
            if moved_objects_cnt > 0:
                moved_objects_cnt = mydb.move_objects_to_delete_queue(config['ttl'], batch_size)
                total_objects_cnt += moved_objects_cnt
                logging.info("%s object(s) was moved to storage delete queue",
                             moved_objects_cnt)
            if moved_parts_cnt > 0:
                moved_parts_cnt = mydb.move_objects_parts_to_delete_queue(config['ttl'], batch_size)
                total_parts_cnt += moved_parts_cnt
                logging.info("%s objects part(s) was moved to storage delete queue",
                             moved_parts_cnt)

        logging.info("Totally moved %s objects and %s parts during this run",
                     total_objects_cnt, total_parts_cnt)
        logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
