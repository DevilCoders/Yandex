#!/usr/bin/python3.6

from abc import abstractmethod
import argparse
import copy
import logging
import os
import subprocess
import sys
import time

import json
import pymongo


DBAAS_CONF = '/etc/dbaas.conf'
DEFAULT_CONFIG = {
    'config_file': '/etc/yandex/mdb-mongodb-deadlock-detector.json',
    'helpers': {
        'mongodb': {
            'mongouri': 'mongodb://localhost:27018/admin?tls=true&tlsCAFile=/etc/mongodb/ssl/allCAs.pem&connectTimeoutMS=60000&socketTimeoutMS=60000&appName=mdb-mongodb-dd',
        },
    },
    'checks': {
        'active': ['is_master', 'diagnostic_data', 'log', 'ping'],
        'is_master': {
            'check': 'file_content',
            'file_path': '/tmp/.mongod_tier.cache',
            'content': 'primary',
        },
        'sli': {'check': 'file_content', 'file_path': '/tmp/mongod_sli.failed', 'content': lambda x: int(x) >= 4},
        'log': {
            'check': 'file_mtime',
            'path': '/var/log/mongodb/mongod.log',
            'last_ts': 1 * 60 * 30,  # 30 minutes
        },
        'diagnostic_data': {
            'check': 'file_mtime',
            'path': '/var/lib/mongodb/diagnostic.data/metrics.interim',
            'last_ts': 1 * 60 * 30,  # 30 Minutes
        },
        'ping': {
            'check': 'mongodb_not_ping',
        },
    },
    'actions': {
        'active': ['add_to_flag_file'],  # also 'dump_bt', 'restart'
        'dump_bt': {
            'action': 'exec',
            'cmd': ['/usr/local/bin/mdb-sample-backtraces.sh', 'mongod', '5', '10'],
            'timeout': 300,
        },
        'restart': {
            'action': 'exec',
            'cmd': ['/usr/sbin/service', 'mongodb', 'restart'],
        },
        'add_to_flag_file': {
            'action': 'file_append',
            'file_path': '/var/tmp/mdb-mdd.txt',
            'content': lambda: time.ctime() + '\n',
            'rewrite': False,
        },
    },
}

LOG_CONFIG = {
    'filename': '/var/log/mongodb/mdb-mongodb-deadlock-detector.log',
    'format': '%(asctime)s [%(levelname)s] %(process)d %(module)s:\t%(message)s',
    'level': logging.DEBUG,
}


def readJSON(fname):
    with open(fname) as f:
        return json.load(f)


def GetAllSubclassesRecursive(cls):
    return set(cls.__subclasses__()).union([s for c in cls.__subclasses__() for s in GetAllSubclassesRecursive(c)])


class Check:
    '''
    Base class for check
    '''

    name = 'abstract_check'

    def __init__(self, config, logger):
        self.config = copy.deepcopy(config)
        self.log = logger

    @abstractmethod
    def check(self, helpers):
        pass


class Action:
    '''
    Base class for action
    '''

    name = 'abstract_action'

    def __init__(self, config, logger):
        self.config = copy.deepcopy(config)
        self.log = logger

    @abstractmethod
    def run(self, helpers):
        pass


class Helper:
    '''
    Base class for helper
    '''

    name = 'abstract_helper'

    def __init__(self, config, logger):
        self.config = copy.deepcopy(config)
        self.log = logger


class FileContentCheck(Check):
    '''
    Check file content
    '''

    name = 'file_content'

    def check(self, helpers):
        fpath = self.config['file_path']
        expected_content = self.config['content']
        with open(fpath) as f:
            content = f.read()

            if callable(expected_content):
                return expected_content(content)
            else:
                return expected_content == content


class FileMTimeCheck(Check):
    '''
    Check if MTime of file is older than
    '''

    name = 'file_mtime'

    def check(self, helpers):
        fpath = self.config['path']
        expected_age = int(self.config['last_ts'])

        mtime = os.stat(fpath).st_mtime
        now = int(time.time())

        return mtime < (now - expected_age)


class MongoNotAliveCheck(Check):
    '''
    Check if we CAN'T ping mongo
    '''

    name = 'mongodb_not_ping'

    def check(self, helpers):
        return not helpers['mongodb'].is_alive()


class ExecAction(Action):
    '''
    Action: Exec some program
    '''

    name = 'exec'

    def run(self, helpers):
        timeout = self.config.get('timeout', None)
        cmd = self.config['cmd'][:]
        if timeout is not None:
            cmd = ['/usr/bin/timeout', '--kill-after={}'.format(timeout), timeout] + cmd
        ret = subprocess.run(self.config['cmd'], capture_output=True, text=True)
        return ret.returncode, ret.stdout, ret.stderr


class FileAppendAction(Action):
    '''
    Add some content to file
    '''

    name = 'file_append'

    def run(self, helpers):
        fopen_flags = 'a'
        if self.config.get('rewrite', False):
            fopen_flags = 'w'

        with open(self.config['file_path'], fopen_flags) as f:
            content = self.config['content']
            if callable(content):
                content = content()
            f.write(content)

        return True


class MongoDBHelper(Helper):
    '''
    Helper to work with MongoDB
    '''

    name = 'mongodb'

    def __init__(self, config, logger):
        super().__init__(config, logger)
        self.mongouri = self.config['mongouri']
        self.conn = None

    def get_conn(self):
        if self.conn is None:
            self.conn = pymongo.MongoClient(self.mongouri)
        return self.conn

    def is_alive(self):
        '''
        Ping mongodb to check if it is alive
        '''
        with self.get_conn() as conn:
            ret = conn['admin'].command('hello')
            return int(ret.get('ok', 0)) == 1


def getDefaultChecks(config):
    return {
        'file_content': FileContentCheck,
        'file_mtime': FileMTimeCheck,
        'mongodb_not_ping': MongoNotAliveCheck,
    }


def getDefaultActions(config):
    return {
        'exec': ExecAction,
        'file_append': FileAppendAction,
    }


def getDefaultHelpers(config):
    return {
        'mongodb': MongoDBHelper,
    }


def processChecksAndActions(config, checks, actions, helpers_list):
    '''
    Run checks and actions if any
    '''

    log = config['log']
    # create helper objects if any
    helpers = {}
    for hname, hclass in helpers_list.items():
        helpers[hname] = hclass(config['helpers'].get(hname, {}), log)

    # perform each check until some of them fails
    for check_name in config['checks']['active']:
        check_opts = config['checks'][check_name]
        log.debug('Running check %s with opts %s', check_name, check_opts)

        if not checks[check_opts['check']](check_opts, log).check(helpers):
            log.debug('Check %s failed, exit', check_opts)
            return False

    log.info('All check passed, perform actions')

    for action_name in config['actions']['active']:
        action_opts = config['actions'][action_name]
        log.debug('Performing action %s with options %s', action_name, action_opts)
        ret = actions[action_opts['action']](action_opts, log).run(helpers)
        log.info('Action %s produced output: %s', action_name, ret)

    log.info('All actions performed')
    return True


def updateConfig(config, newConfig):
    '''
    [Not So] Deep Update config
    '''
    for key in newConfig.keys():
        if key in config:
            # Not so deep, actually, kek
            config[key].update(newConfig[key])
        else:
            config[key] = copy.deepcopy(newConfig[key])

    return config


def readConfig(config, args=None):
    '''
    Initial Read config from arguments and file
    '''
    if args is not None and args.config:
        config['config_file'] = args.config

    try:
        if config['config_file']:
            updateConfig(config, readJSON('config_file'))
    except Exception as exc:
        config['log'].error(exc, exc_info=True)

    if args is None:
        return

    if args.dry_run:
        config['actions']['active'] = []

    if args.mongouri:
        config['helpers']['mongodb']['mongouri'] = args.mongouri

    config['log'].debug("New config: %s", config)
    return config


def processArguments(config, argv=None):
    '''
    Parse arguments and read config from file
    '''

    if argv is None:
        argv = sys.argv[1:]

    parser = argparse.ArgumentParser(description='MongoDB deadlock detector script')
    parser.add_argument('--config', help="Config file to use", default=config['config_file'])
    parser.add_argument(
        '--dry-run',
        help="Do not perform any actions, just perform checks and log",
        action='store_true',
        dest='dry_run',
    )
    parser.add_argument('--mongouri', help="mongouri", default=config['helpers']['mongodb']['mongouri'])

    args = parser.parse_args(argv)

    readConfig(config, args)

    config['log'].info("Started mongo-dd with args %s", args)
    return config


if __name__ == "__main__":
    logging.basicConfig(**LOG_CONFIG)
    log = logging.getLogger("mdb-mongo-dd")

    config = copy.deepcopy(DEFAULT_CONFIG)
    config['log'] = log
    config = processArguments(config, sys.argv[1:])

    checks = getDefaultChecks(config)
    actions = getDefaultActions(config)
    helpers = getDefaultHelpers(config)

    processChecksAndActions(config, checks, actions, helpers)
