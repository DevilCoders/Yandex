#!/usr/bin/python2
"""
Salt-minion watchdog
"""

import argparse
import calendar
import datetime
import hashlib
import imp
import json
import logging
import os
import subprocess
import time
from collections import namedtuple

import yaml
from dateutil import parser

if os.name == 'nt':
    PING_SALT_MASTER_CONFIG = r'C:\Program Files\MdbConfigSalt\mdb-ping-salt-master.yaml'
    DEPLOY_VERSION_PATH = r'C:\salt\conf\deploy_version'
    GET_MASTER_PATH = r'C:\Program Files\MdbConfigSalt\modules\get_master.py'
    SALT_PING_TS_PATH = r'C:\salt\var\run\mdb-salt-ping-ts'
    SALT_CONFIG_PATH = r'C:\salt\conf'
    SALT_PKI_PATH = r'C:\salt\conf\pki\minion'
    SALT_MASTER_KEY_PATH = r'C:\salt\conf\pki\minion\master_sign.pub'
    SALT_CONFIG_CHECKSUM_PATH = os.path.join(os.environ['TMP'], 'mdb-salt-config-checksum')
    SALT_CALL = r'C:\salt\salt-call.bat'
    SALT_MASTER_OVERRIDE_PATH = r'C:\salt\conf\minion_master_override'
    MDB_DEPLOY_API_HOST_PATH = r'C:\salt\conf\mdb_deploy_api_host'
    MDB_PORTO_FILE = ''
else:
    PING_SALT_MASTER_CONFIG = '/etc/yandex/mdb-ping-salt-master/mdb-ping-salt-master.yaml'
    DEPLOY_VERSION_PATH = '/etc/yandex/mdb-deploy/deploy_version'
    GET_MASTER_PATH = '/var/lib/salt/modules/get_master.py'
    SALT_PING_TS_PATH = '/tmp/mdb-salt-ping-ts'
    SALT_CONFIG_PATH = '/etc/salt/minion.d'
    SALT_PKI_PATH = '/etc/salt/pki/minion'
    SALT_MASTER_KEY_PATH = '/etc/salt/pki/minion/master_sign.pub'
    SALT_CONFIG_CHECKSUM_PATH = '/tmp/mdb-salt-config-checksum'
    SALT_CALL = 'salt-call'
    SALT_MASTER_OVERRIDE_PATH = '/etc/salt/minion_master_override'
    MDB_DEPLOY_API_HOST_PATH = '/etc/yandex/mdb-deploy/mdb_deploy_api_host'
    MDB_PORTO_FILE = '/etc/dom0hostname'


class SaltMinionPinger:
    """
    Salt-minion watchdog
    """

    def __init__(self, config_path):
        self.logger = None
        self.setup_logger()
        self.config = self.load_config(config_path)
        self.get_master = imp.load_source('get_master', self.config.get_master_path)
        self.timestamps = {
            'deploy_check': datetime.datetime.utcfromtimestamp(0),
            'master_check': datetime.datetime.utcfromtimestamp(0),
            'ping_check': datetime.datetime.utcfromtimestamp(0),
            'salt_config_check': datetime.datetime.utcfromtimestamp(0),
        }
        self.deploy_version = self.get_deploy_version()
        self.last_master = self.get_master.master()
        self.logger.info('Loaded with: Deploy version %s, Last master %s', self.deploy_version, self.last_master)

        # Check if we have correct master key on start
        if self.read_master_key() != self.get_master.master_public_key():
            self.update_master_key()
            self.restart_minion()

    def read_master_key(self):
        try:
            with open(SALT_MASTER_KEY_PATH, 'r') as inp:
                return inp.read().strip()
        except IOError:
            return None

    def update_master_key(self):
        new_key = self.get_master.master_public_key()
        if not new_key:
            self.logger.error('Empty salt-master public key')
            return
        try:
            self.logger.info('Updating salt-master public key')
            self.logger.debug('New salt master public key: \n%s', new_key)
            with open(SALT_MASTER_KEY_PATH, 'w') as out:
                return out.write(new_key)
        except Exception as exc:
            self.logger.info('Failed to write master public key: %s', repr(exc))

    def setup_logger(self):
        """
        Silence default loggers and load our own
        """
        root_logger = logging.getLogger()
        root_logger.setLevel(logging.WARNING)

        handler = logging.StreamHandler()

        formatter = logging.Formatter('%(asctime)s %(name)s [%(levelname)s]: %(message)s')
        handler.setFormatter(formatter)

        root_logger.addHandler(handler)

        logger = logging.getLogger('mdb-ping-salt-master')
        logger.setLevel(logging.DEBUG)
        self.logger = logger

    def load_config(self, config_path):
        """
        Load config from yaml file
        """
        config = {
            'iterations_interval': 1,
            'deploy_check_period': 30,
            'master_check_period': 30,
            'ping_check_period': 15,
            'salt_config_check_period': 300,
            'salt_timeout': 900,
            'deploy_version_file': DEPLOY_VERSION_PATH,
            'salt_ping_ts_file': SALT_PING_TS_PATH,
            'salt_config_path': SALT_CONFIG_PATH,
            'salt_config_checksum_file': SALT_CONFIG_CHECKSUM_PATH,
            'get_master_path': GET_MASTER_PATH,
        }

        Config = namedtuple('Config', list(config.keys()))

        try:
            with open(config_path) as config_file:
                loaded_cfg = yaml.safe_load(config_file)

            config.update(loaded_cfg)
        except BaseException as exc:
            self.logger.info('Failed to load config: %s', repr(exc))

        self.logger.info("Using config '%r'", config)
        return Config(**config)

    def read_ts(self):
        """
        Get master ping file flag timestamp
        """
        try:
            with open(self.config.salt_ping_ts_file) as inp:
                return parser.parse(inp.read())
        except BaseException as exc:
            self.logger.info('Salt ping timestamp is unknown: %s', repr(exc))

    def get_deploy_version(self):
        """
        Get deploy version from file
        """
        try:
            with open(self.config.deploy_version_file) as inp:
                return int(inp.read())
        except BaseException:
            return 2

    def update_ts_file(self, now):
        """
        Update master ping file flag timestamp
        """
        self.logger.info('Storing %s into timestamp file', now)
        with open(self.config.salt_ping_ts_file, 'w') as out:
            out.write(str(now))

    def is_restart_needed(self, now):
        """
        Check master ping file flag timestamp
        """
        timeout = datetime.timedelta(seconds=self.config.salt_timeout)
        last_ping = self.read_ts()
        if last_ping is None:
            self.logger.info('Last salt update time is unknown')
            self.update_ts_file(now)
            # Do not restart minion right away. Wait and see if timestamp file gets updated.
            return False

        diff = now - last_ping
        self.logger.info('Current time is %s, last salt update was at %s, diff is %s', now, last_ping, diff)

        if diff < timeout:
            return False

        self.logger.info('Salt-minion needs restart because last salt update was %s ago', diff)
        self.update_ts_file(now)
        return True

    def can_restart_minion(self):
        """
        Check if its safe to restart minion (it does not run any states)
        """
        output = yaml.load(subprocess.check_output([SALT_CALL, "--local", "state.running"]))
        state = output.get('local', None)
        if state:
            self.logger.info('Salt-minion has state(s) running: %r', state)
            return False

        # hinge to check for running backups (MDB-17206)
        output = yaml.load(subprocess.check_output([SALT_CALL, "--local", "status.pid", "run-backup"]))
        running_backup = output.get('local', None)
        if running_backup:
            self.logger.info('Salt-minion has running backup: %r', running_backup)
            return False

        self.logger.debug('No states or backups running on minion, its safe to restart')
        return True

    def restart_minion(self):
        """
        Restart salt-minion process under supervisor or service
        """
        if os.name == 'nt':
            import win32serviceutil

            win32serviceutil.RestartService("salt-minion")
        elif os.path.exists('/etc/supervisor/conf.d/salt-minion.conf'):
            self.logger.info('Detected salt-minion under supervisor')
            subprocess.call(['supervisorctl', 'restart', 'salt-minion'])
        else:
            self.logger.info('Restarting salt-minion under service')
            subprocess.call(['service', 'salt-minion', 'restart'])

    def check_deploy_version_change(self, now):
        """
        Check if deploy version on fs is changed
        """
        if now - self.timestamps['deploy_check'] >= datetime.timedelta(seconds=self.config.deploy_check_period):
            self.timestamps['deploy_check'] = now

            version = self.get_deploy_version()
            if version != self.deploy_version:
                self.logger.info('Deploy version changed from %s to %s', self.deploy_version, version)
                self.deploy_version = version
        return self.deploy_version != 2

    def check_master_change(self, now):
        """
        Check if master was changed in deploy api
        """
        if now - self.timestamps['master_check'] < datetime.timedelta(seconds=self.config.master_check_period):
            return False

        self.timestamps['master_check'] = now

        current_master = self.get_master.master()
        if not current_master:
            self.logger.info('Current master is unknown, last master is %s', self.last_master)
            return False

        if current_master == self.last_master:
            return False

        self.logger.info('Master changed from %s to %s', self.last_master, current_master)

        if not self.can_restart_minion():
            return False

        self.last_master = current_master
        self.update_master_key()
        self.restart_minion()
        return True

    def check_stale_ping(self, now):
        """
        Check for stale ping from master
        """
        if now - self.timestamps['ping_check'] < datetime.timedelta(seconds=self.config.ping_check_period):
            return False

        self.timestamps['ping_check'] = now

        if not self.is_restart_needed(now):
            return False

        if not self.can_restart_minion():
            return False

        self.restart_minion()
        return True

    def check_pki(self, now):
        """
        Check if minion keys are consistent (and drop them if not)
        """
        private_path = os.path.join(SALT_PKI_PATH, 'minion.pem')
        private_path_exists = os.path.exists(private_path)
        private_ok = private_path_exists and len(open(private_path).read()) > 0

        public_path = os.path.join(SALT_PKI_PATH, 'minion.pub')
        public_path_exists = os.path.exists(public_path)
        public_ok = public_path_exists and len(open(public_path).read()) > 0

        if (private_path_exists != public_path_exists) or (private_ok != public_ok):
            self.logger.info('Minion PKI seems broken. Private key ok: %s, public key ok: %s', private_ok, public_ok)
            now_unix = calendar.timegm(now.utctimetuple())
            if private_path_exists:
                private_ctime = os.stat(private_path).st_ctime
                if now_unix - private_ctime < 60:
                    self.logger.info('Private key is too new. Skipping consistency check')
                    return True
                self.logger.info('Removing %s', private_path)
                os.unlink(private_path)
            if public_path_exists:
                public_ctime = os.stat(public_path).st_ctime
                if now_unix - public_ctime < 60:
                    self.logger.info('Public key is too new. Skipping consistency check')
                    return True
                self.logger.info('Removing %s', public_path)
                os.unlink(public_path)
            self.restart_minion()
            return True

        return False

    def read_salt_config_checksum(self):
        """
        Get salt minion config checksum from file
        """
        try:
            with open(self.config.salt_config_checksum_file) as inp:
                return json.loads(inp.read())
        except BaseException as exc:
            self.logger.info('Salt config checksum is unknown: %s', repr(exc))

    def update_salt_config_checksum_file(self, checksum):
        """
        Update salt minion config checksum file
        """
        self.logger.info('Storing %s into salt config checksum file', checksum)
        with open(self.config.salt_config_checksum_file, 'w') as out:
            out.write(json.dumps(checksum))

    def check_salt_config_change(self, now):
        """
        Check for salt minion config change
        """
        if now - self.timestamps['salt_config_check'] < datetime.timedelta(
            seconds=self.config.salt_config_check_period
        ):
            return False

        self.timestamps['salt_config_check'] = now

        checksum = {}
        for dirpath, _, filenames in os.walk(self.config.salt_config_path):
            for filename in filenames:
                # Skip runtime-generated salt-minion files
                if filename.startswith('_'):
                    continue

                path = os.path.join(dirpath, filename)
                try:
                    with open(path, 'rb') as rfile:
                        hasher = hashlib.md5()
                        hasher.update(rfile.read())
                        checksum[path] = hasher.hexdigest()
                except BaseException as exc:
                    self.logger.info('Failed to load salt minion config file %s for hashing: %s', filename, repr(exc))

        old_checksum = self.read_salt_config_checksum()
        if not old_checksum:
            self.logger.info('Old salt config checksum is unknown')
            self.update_salt_config_checksum_file(checksum)
            return False

        if checksum == old_checksum:
            self.logger.info('Current salt minion config checksum equals the old one')
            return False

        self.logger.info(
            'Current salt minion config checksum %s does not equal the old one %s',
            json.dumps(checksum),
            json.dumps(old_checksum),
        )

        if not self.can_restart_minion():
            return False

        self.update_salt_config_checksum_file(checksum)
        self.restart_minion()
        return True

    def run(self):
        """
        Main loop
        """
        while True:
            time.sleep(self.config.iterations_interval)
            now = datetime.datetime.utcnow()

            if self.check_pki(now):
                continue
            if self.check_deploy_version_change(now):
                continue
            if self.check_master_change(now):
                continue
            if self.check_stale_ping(now):
                continue
            if self.check_salt_config_change(now):
                continue


def _main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-c', '--config', type=str, help='Config file path', default=PING_SALT_MASTER_CONFIG)
    args = arg_parser.parse_args()
    pinger = SaltMinionPinger(args.config)
    pinger.run()


if __name__ == '__main__':
    _main()
