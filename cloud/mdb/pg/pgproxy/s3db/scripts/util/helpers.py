#!/usr/bin/env python
# encoding: utf-8

import logging
import sys
import time

import psycopg2.errorcodes
import psycopg2.extensions

from .exceptions import TPCCommitException
from .const import CA_CERTS_PATH
from sentry_sdk import init as sentry_sdk_init
from sentry_sdk.integrations.logging import LoggingIntegration


def read_config(options, defaults={}):
    config = {}
    # Set default value if it not present in config
    for key, value in defaults.iteritems():
        if key not in config and value is not None:
            config[key] = value
    # Rewrite config options from command line
    for key, value in vars(options).iteritems():
        if value is not None:
            config[key] = value
    return config


def init_logging(config):
    level = config['log_level'].upper()
    # logging.setLevel in python < 2.7 doesn't resolve textual level
    # ("DEBUG" for example) to numeric representation (DEBUG = 10)
    if sys.version_info < (2, 7):
        for lvlnum, lvlname in logging._levelNames.iteritems():
            if lvlname == level:
                level = lvlnum
                break
        else:
            level = logging.WARNING
    root = logging.getLogger()
    root.setLevel(level)
    _format = logging.Formatter("%(levelname)s\t%(asctime)s\t\t%(message)s")
    if 'log_file' in config and config['log_file']:
        _handler = logging.FileHandler(config['log_file'])
    else:
        _handler = logging.StreamHandler(sys.stdout)
    _handler.setFormatter(_format)
    _handler.setLevel(level)

    root.handlers = [_handler, ]

    sentry_dsn = config.get('sentry_dsn')
    if sentry_dsn:
        sentry_logging = LoggingIntegration(
            level=level,
            event_level=logging.ERROR
        )
        sentry_sdk_init(
            dsn=sentry_dsn,
            integrations=[sentry_logging],
            ca_certs=CA_CERTS_PATH
        )
        root.handlers.append(sentry_logging._handler)


def complete_tpc(dbs, pgmetadb, attempts, sleep_interval, fail_step):
    for db in dbs:
        if not db.type:
            raise Exception("db '%s' needs type to be set" % db._connstring)
    rollback_possible = True
    try:
        # Here we need to prepare transactions on all database
        # and then commit them.
        for db in dbs:
            db.prepare_tpc()
            if fail_step == -1:
                raise BaseException("Requested fail at step %s", fail_step)
        if fail_step == 0:
            raise Exception("Requested fail at step %s", fail_step)
        step = 1
        for db in dbs:
            db.commit_tpc()
            rollback_possible = False
            if fail_step == step:
                raise Exception("Requested fail at step %d", fail_step)
            step += 1
    except Exception as error:
        # Something wrong, we need to handle this as follows:
        # If prepared transaction on first database was not committed we need
        # to rollback transaction on all databases. If transaction on first
        # database was committed we need commit transaction on all databases.
        logging.fatal(error)
        # reconnect to pgmeta to avoid problems with pgmeta
        pgmetadb.reconnect()

        # commit in direct order, rollback in reverse order
        if rollback_possible:
            dbs = reversed(dbs)

        for db in dbs:
            attempt_num = 0
            if db.status() != psycopg2.extensions.STATUS_PREPARED:
                logging.warn("Transaction was not prepared in '%s'", db._connstring)
                continue
            logging.warn("Start retry cycle for '%s'", db._connstring)
            while attempt_num < attempts:
                attempt_num += 1
                logging.warn("Sleeping sleep interval %d", sleep_interval)
                time.sleep(sleep_interval)
                logging.warn("Trying to find new master. Attempt %d", attempt_num)
                if db.shard_id is None:
                    raise Exception("db '%s' needs shard_id to be set" % db._connstring)
                try:
                    logging.warn("Reconnect to pgmeta '%s'", pgmetadb._connstring)
                    pgmetadb.reconnect()

                    logging.warn("Reconnect to master %s, shard %d", db.type, db.shard_id)
                    new_master = pgmetadb.get_master(db.type, db.shard_id,
                                                     user=db.user, password=db.password, autocommit=True)
                    logging.warn("Successfully connected to new master")
                    if rollback_possible:
                        logging.warn("Trying to rollback")
                        new_master.rollback_tpc(db.get_prepared_xid())
                        logging.warn("Successfull rollback")
                    else:
                        if db.status() == psycopg2.extensions.STATUS_PREPARED:
                            logging.warn("Trying to commit")
                            new_master.commit_tpc(db.get_prepared_xid())
                            logging.warn("Successfull commit")
                        else:
                            logging.warn("Nothing to do on this db")
                    break
                except Exception as e:
                    logging.fatal(e)
                if attempt_num == attempts:
                    raise TPCCommitException('Cannot {0} on {1} shard of {2}. Stop.'.format(
                        'rollback' if rollback_possible else 'commit', db.shard_id, db.type))
        raise error
