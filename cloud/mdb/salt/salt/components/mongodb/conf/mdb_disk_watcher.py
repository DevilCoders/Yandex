#!/usr/bin/python3.6

import argparse
import logging
import os
import sys
import time

import bson
import pymongo
import ssl

MBYTE = 1024**2
YEAR = 60 * 60 * 24 * 365

DEFAULT_CONFIG = {
    'fs': '/var/lib/mongodb',
    'free_percent_limit': 0.1,
    'stepdown_sec': 60,
    'free_mb_limit': 512,
    'unfreeze_timeout': 60,
    'flag_script_disabled': '/tmp/disabled_disk_watcher',
    'flag_file_locked': '/tmp/mdb-mongo-fsync.locked',
    'flag_file_unlocked': '/tmp/mdb-mongo-fsync.unlocked',
    'mongopass': '/root/.mongopass',
    'pid_file': '/var/run/mongodb.pid',
    'use_ssl': True,
    'ssl_ca_certs': '/etc/mongodb/ssl/allCAs.pem',
    'force': False,
    'unlock': False,
    'unfreeze': False,
    'enable': False,
    'disable': False,
}
LOG_CONFIG = {
    'filename': '/var/log/mongodb/mdb-disk-watcher.log',
    'format': '%(asctime)s [%(levelname)s] %(process)d %(module)s:\t%(message)s',
    'level': logging.DEBUG,
}


class MDBMongoDiskWatcher(object):
    def __init__(self, config=None):
        self.config = {}
        self.config.update(DEFAULT_CONFIG)
        if config is not None:
            self.config.update(config)

        self.log = logging.getLogger("mdb-disk-watcher")

    def get_connection_opts(self, **kwargs):
        '''
        Get connection opts for MongoDB
        '''
        with open(self.config['mongopass']) as mongopass:
            for line in mongopass:
                host, port, db, user, password = line.split(':', 4)
                password = password.rstrip('\n')
                # rc1b-xn55d1njt97viusm.mdb.cloud-preprod.yandex.net:27018:admin:admin:PASSWORD
                if user != 'admin':
                    continue

                self.log.debug("Connection string is: mongodb://%s:<PASSWORD>@%s:%s/%s", user, host, port, db)
                res = {
                    'host': host,
                    'port': int(port),
                    'username': user,
                    'password': password,
                    'authSource': db,
                    'appname': 'mdb-disk-watcher',
                    'serverSelectionTimeoutMS': 5000,
                }
                if self.config['use_ssl']:
                    res.update(
                        {
                            'ssl': True,
                            'ssl_ca_certs': self.config['ssl_ca_certs'],
                            'ssl_cert_reqs': ssl.CERT_NONE,
                        }
                    )
                if kwargs is not None:
                    res.update(kwargs)
                passwd = res['password']
                res['password'] = '<PASSWORD>'
                self.log.debug("Connection options: %s", res)  # hide password from logs
                res['password'] = passwd
                return res

        errorMessage = "Unable to get connection options: no admin user found in {} file".format(
            self.config['mongopass']
        )
        self.log.error(errorMessage)
        raise Exception(errorMessage)

    def make_mongo_connection(self, **kwargs):
        '''
        Create connection to MongoDB
        '''
        return pymongo.MongoClient(**self.get_connection_opts(**kwargs))

    def _check_mongodb_is_locked(self, conn=None):
        '''
        Check if MongoDB is locked now or not
        '''

        def _check_lock(conn):
            # We use a=1 here to get empty list of currentOp
            res = conn['admin'].command('currentOp', a=1)
            return res.get('fsyncLock', False)

        if conn:
            return _check_lock(conn)
        with self.make_mongo_connection() as conn:
            return _check_lock(conn)

    def lock_mongodb(self):
        '''
        Lock MongoDB (make read only)
        '''
        with self.make_mongo_connection() as conn:
            if self._check_mongodb_is_locked(conn):
                return

            self.stepdown_mongodb(conn)

            res = conn['admin'].command(
                bson.son.SON(
                    [
                        ('fsync', 1),
                        ('lock', True),
                    ]
                )
            )
            self.log.debug('db.fsyncLock = %s', res)

    def stepdown_mongodb(self, conn):
        '''
        stepDown mongodb
        '''
        try:
            is_master = conn['admin'].command('isMaster')
            self.log.debug('db.isMaster() = %s', is_master)
            if is_master.get('ismaster', False):
                res = conn['admin'].command(
                    bson.son.SON(
                        [
                            ('replSetStepDown', self.config['stepdown_sec']),
                        ]
                    )
                )
                self.log.debug('rs.stepDown(%d) = %s', self.config['stepdown_sec'], res)
        except Exception as exc:
            self.log.error(exc, exc_info=True)

    def freeze_mongodb(self):
        with self.make_mongo_connection() as conn:
            try:
                res = conn['admin'].command(
                    bson.son.SON(
                        [
                            ('replSetFreeze', YEAR),
                        ]
                    )
                )
                self.log.debug('rs.freeze(60*60*24*365) = %s', res)
            except Exception as exc:
                self.log.error(exc, exc_info=True)

    def unlock_mongodb(self):
        '''
        Unlock MongoDB (make read-write)
        '''
        with self.make_mongo_connection() as conn:
            while self._check_mongodb_is_locked(conn):
                res = conn['admin'].command('fsyncUnlock')
                self.log.debug('db.fsyncUnlock = %s', res)
                time.sleep(1)

    def enable(self):
        filename = self.config['flag_script_disabled']
        try:
            os.remove(filename)
            self.log.debug('File %s was removed', filename)
        except IOError as err:
            self.log.debug("File %s wasn't deleted cause of %s", filename, err)

    def disable(self):
        self.touch_file(self.config['flag_script_disabled'])

    def touch_file(self, filename):
        with open(filename, 'a'):
            os.utime(filename)
            self.log.debug('File %s was created', filename)

    def unfreeze_mongodb(self):
        '''
        Set MongoDB Freeze to 0 - makes eligible to become a primary
        '''
        with self.make_mongo_connection() as conn:
            stop_ts = time.time() + self.config['unfreeze_timeout']
            while time.time() < stop_ts:
                try:
                    is_master = conn['admin'].command('isMaster')
                    self.log.debug('db.isMaster() = %s', is_master)
                    if is_master.get('ismaster', False):
                        self.log.debug('skip rs.freeze(0) on master')
                        break
                    else:
                        res = conn['admin'].command(
                            bson.son.SON(
                                [
                                    ('replSetFreeze', 0),
                                ]
                            )
                        )
                        self.log.debug('rs.freeze(0) = %s', res)
                except pymongo.errors.OperationFailure as err:
                    self.log.error(err)
                    time.sleep(1)
                else:
                    break  # everything ok, so we can break this while

    def get_fs_stat(self):
        '''
        wrapper for os.statvfs (used for better mocking)
        '''

        return os.statvfs(self.config['fs'])

    def get_free_space(self):
        df = self.get_fs_stat()

        free_space_mb = 1.0 * df.f_bavail * df.f_bsize / MBYTE
        free_percent = (1.0 * df.f_bavail / df.f_blocks) * 100

        return free_space_mb, free_percent

    def check_is_ro_needed(self, free_space_mb, free_percent):
        '''
        Check if we neet MongoDB to be RO or not (check if there is enough free space on disk for RW)
        '''

        need_ro = (free_space_mb < self.config['free_mb_limit']) or (free_percent < self.config['free_percent_limit'])
        return need_ro

    def _get_pid_from_file(self, path):
        '''
        Get int(PID) from file
        '''
        with open(path, 'r') as f:
            pid = int(f.read())
            return pid

    def _put_pid_to_file(self, path, pid=None):
        '''
        Get int(PID) from file
        '''
        if pid is None:
            try:
                os.unlink(path)
            except FileNotFoundError:
                pass
        else:
            with open(path, 'w') as f:
                f.write(str(pid))

    def get_mongodb_pid(self):
        '''
        Get PID of running MongoDB
        '''
        return self._get_pid_from_file(self.config['pid_file'])

    def get_locked_pid(self):
        '''
        Get int(PID) of locked mongodb, or None
        '''
        try:
            return self._get_pid_from_file(self.config['flag_file_locked'])
        except IOError:
            pass
        except ValueError:
            pass
        return None

    def get_unlocked_pid(self):
        '''
        Get int(PID) of unlocked mongodb, or None
        '''
        try:
            return self._get_pid_from_file(self.config['flag_file_unlocked'])
        except IOError:
            pass
        except ValueError:
            pass
        return None

    def check_if_mongodb_is_locked_and_fix_pids(self):
        '''
        Check if MongoDB currently locked by us or not and fix pids for consistency
        '''
        is_ro = None
        lock_file_pid = self.get_locked_pid()
        unlock_file_pid = self.get_unlocked_pid()
        mongodb_pid = self.get_mongodb_pid()

        if any(
            (
                lock_file_pid is not None and unlock_file_pid is not None,
                lock_file_pid is None and unlock_file_pid is None,
                lock_file_pid is not None and lock_file_pid != mongodb_pid,
                unlock_file_pid is not None and unlock_file_pid != mongodb_pid,
            )
        ):
            is_ro = self._check_mongodb_is_locked()

            if is_ro:
                self.mark_mongodb_locked(mongodb_pid)
            else:
                self.mark_mongodb_unlocked(mongodb_pid)

        else:
            is_ro = lock_file_pid is not None

        return is_ro

    def mark_mongodb_locked(self, pid=None):
        '''
        Mark that we locked MongoDB (create lock file)
        '''
        if pid is None:
            pid = self.get_mongodb_pid()

        self._put_pid_to_file(self.config['flag_file_locked'], pid)
        self._put_pid_to_file(self.config['flag_file_unlocked'], None)

    def mark_mongodb_unlocked(self, pid=None):
        '''
        Mark that we unlocked MongoDB (delete lock file)
        '''
        if pid is None:
            pid = self.get_mongodb_pid()

        self._put_pid_to_file(self.config['flag_file_unlocked'], pid)
        self._put_pid_to_file(self.config['flag_file_locked'], None)

    def is_script_enabled(self):
        filename = self.config['flag_script_disabled']
        if not os.path.exists(filename):
            return True

        if self.config.get('force'):
            self.log.debug("Disable file %s exists, but --force arg was provided", filename)
            return True

        self.log.debug("Disabled by: %s, exiting", filename)
        return False

    def main(self, argv):
        try:
            parser = argparse.ArgumentParser(description='MDB Disk watching utility')

            parser.add_argument('--fs', help="FS patch to watch free space")
            parser.add_argument('--free-percent-limit', help="Free space limit in percents", type=float)
            parser.add_argument('--free-mb-limit', help="Free space limit in megabytes", type=float)
            parser.add_argument('--flag-file-locked', help="File with PID of MongoD if it's locked")
            parser.add_argument('--flag-file-unlocked', help="File with PID of MongoD it it's not locked")
            parser.add_argument('--mongopass', help="Path to .mongopass file with auth data")
            parser.add_argument('--pid-file', help="Path to PID file")
            parser.add_argument('--ssl-ca-certs', help="Path to CA certificates .pem file")
            parser.add_argument('--no-ssl', help="Do not use SSL on connection", action='store_false', dest='use_ssl')

            disabled_file = self.config['flag_script_disabled']
            force_help = "Ignore {} file, if file exists script exit with 0 code".format(disabled_file)
            parser.add_argument('--force', help=force_help, action='store_true', dest='force')

            parser.add_argument('--unlock', help="Unlock database", action='store_true', dest='unlock')
            parser.add_argument('--unfreeze', help="Set rs.freeze(0)", action='store_true', dest='unfreeze')

            parser.add_argument(
                '--enable',
                help="Enable script - remove file {}".format(disabled_file),
                action='store_true',
                dest='enable',
            )
            parser.add_argument(
                '--disable',
                help="Disable script - create file {}".format(disabled_file),
                action='store_true',
                dest='disable',
            )

            args = vars(parser.parse_args(argv))

            for arg in DEFAULT_CONFIG:
                if args.get(arg, None) is not None:
                    self.config[arg] = args.get(arg, DEFAULT_CONFIG[arg])

            self.log.debug("Started mdb-disk-watcher with args %s", args)
            self.log.debug("Config: %s", self.config)

            if self.config.get('enable'):
                self.log.debug('Enabling script')
                self.enable()
                return

            if self.config.get('disable'):
                self.log.debug('Disabling script')
                self.disable()
                return

            if not self.is_script_enabled():
                return

            free_space_mb, free_percent = self.get_free_space()
            need_ro = self.check_is_ro_needed(free_space_mb, free_percent)
            # We check our flag file instead of actual mongo state so we make less impact to system
            try:
                is_ro = self.check_if_mongodb_is_locked_and_fix_pids()
            except FileNotFoundError as err:
                self.log.error("File not found: %s - exiting", err)
                if need_ro:
                    # If mongodb need to be in RO and mongodb isn't running,
                    # still put lock file, so load-monitor will downtime SLI check
                    self.mark_mongodb_locked(0)
                return

            is_unlock_mode = self.config.get('unlock')
            is_unfreeze_mode = self.config.get('unfreeze')
            if is_unlock_mode or is_unfreeze_mode:
                if is_unlock_mode:
                    self.log.debug('Unlocking')
                    self.unlock_mongodb()
                    self.mark_mongodb_unlocked()
                if is_unfreeze_mode:
                    self.log.debug('Unfreezing')
                    self.unfreeze_mongodb()
                return

            if need_ro and not is_ro:
                self.log.warning(
                    "Free space is critical: %.1f MiB, %.2f %%, locking MongoDB to RO", free_space_mb, free_percent
                )
                self.freeze_mongodb()
                self.lock_mongodb()
                self.mark_mongodb_locked()
            elif is_ro and not need_ro:
                self.log.warning(
                    "Free space is NOT critical anymore: %.1f MiB, %.2f %%, unlocking MongoDB from RO",
                    free_space_mb,
                    free_percent,
                )
                self.unlock_mongodb()
                self.unfreeze_mongodb()
                self.mark_mongodb_unlocked()
        except Exception as exc:
            self.log.error(exc, exc_info=True)
            raise


if __name__ == "__main__":
    logging.basicConfig(**LOG_CONFIG)

    disk_watcher = MDBMongoDiskWatcher()
    disk_watcher.main(sys.argv[1:])
