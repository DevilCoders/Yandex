#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DBaaS Container cron module
"""

import argparse
import importlib
import json
import logging
import os
import signal
import sys
import time
from threading import Lock

import jsonschema
from apscheduler.events import EVENT_JOB_ERROR, EVENT_JOB_EXECUTED
from apscheduler.schedulers.background import BackgroundScheduler

from timeout import func_timeout

CONFIG_SCHEMA = {
    'description': 'JSON Schema for DBaaS cron conf file',
    'type': 'object',
    'properties': {
        'log_level': {
            'enum': [
                'CRITICAL',
                'ERROR',
                'WARNING',
                'INFO',
                'DEBUG',
            ],
        },
        'log_file': {
            'type': 'string',
        },
        'modules_path': {
            'type': 'string',
        },
        'status_path': {
            'type': 'string',
        },
        'tasks_path': {
            'type': 'string',
        },
    },
    'required': [
        'log_level',
        'log_file',
        'modules_path',
        'status_path',
        'tasks_path',
    ],
}

TASK_SCHEMA = {
    'description': 'JSON Schema for DBaaS cron task file',
    'type': 'object',
    'properties': {
        'id': {
            'type': 'string',
        },
        'module': {
            'type': 'string',
        },
        'description': {
            'type': 'string',
        },
        'misfire_grace_time': {
            'type': 'integer',
            'minimum': 1,
        },
        'timeout': {
            'type': 'integer',
            'minimum': 1,
        },
        'args': {
            'type': 'object',
        },
        'schedule': {
            'type':
                'object',
            'oneOf': [
                {
                    'type': 'object',
                    'properties': {
                        'trigger': {
                            'enum': ['cron'],
                        },
                        'year': {
                            'type': 'string',
                        },
                        'month': {
                            'type': 'string',
                        },
                        'day': {
                            'type': 'string',
                        },
                        'week': {
                            'type': 'string',
                        },
                        'day_of_week': {
                            'enum': [
                                '*',
                                'mon',
                                'tue',
                                'wed',
                                'thu',
                                'fri',
                                'sat',
                                'sun',
                            ],
                        },
                        'hour': {
                            'type': 'string',
                        },
                        'minute': {
                            'type': 'string',
                        },
                        'second': {
                            'type': 'string',
                        },
                        'start_date': {
                            'type': 'string',
                        },
                        'end_date': {
                            'type': 'string',
                        },
                        'timezone': {
                            'type': 'string',
                        },
                    },
                },
                {
                    'type': 'object',
                    'properties': {
                        'trigger': {
                            'enum': ['interval'],
                        },
                        'weeks': {
                            'type': 'integer',
                            'minimum': 0,
                        },
                        'days': {
                            'type': 'integer',
                            'minimum': 0,
                        },
                        'hours': {
                            'type': 'integer',
                            'minimum': 0,
                        },
                        'minutes': {
                            'type': 'integer',
                            'minimum': 0,
                        },
                        'seconds': {
                            'type': 'integer',
                            'minimum': 0,
                        },
                        'start_date': {
                            'type': 'string',
                        },
                        'end_date': {
                            'type': 'string',
                        },
                        'timezone': {
                            'type': 'string',
                        },
                    },
                },
            ],
        },
    },
    'required': [
        'id',
        'module',
        'misfire_grace_time',
        'timeout',
        'args',
        'schedule',
    ],
}


def _load_config_file(path, schema):
    validator = jsonschema.Draft4Validator(schema)
    with open(path) as config_file:
        config = json.loads(config_file.read())
    config_errors = validator.iter_errors(config)
    report_error = jsonschema.exceptions.best_match(config_errors)
    if report_error is not None:
        raise RuntimeError('Malformed config {path}: {message}'.format(path=path, message=report_error.message))
    return config


def load_config(path):
    """
    Load config with validation
    """
    config = _load_config_file(path, CONFIG_SCHEMA)
    config['tasks'] = list()
    tasks_dir = config['tasks_path']
    for task_file in os.listdir(tasks_dir):
        config['tasks'].append(_load_config_file(os.path.join(tasks_dir, task_file), TASK_SCHEMA))
    return config


def resolve(path, module, timeout):
    """
    Load cron module in path and get wrapper with timeout
    """
    if path not in sys.path:
        sys.path.insert(0, path)

    fun = None

    if os.path.exists(os.path.join(path, '{module}.py'.format(module=module))):
        imp = importlib.import_module(module)
        fun = getattr(imp, module)

    if not fun:
        raise RuntimeError('Error loading module {module}'.format(module=module))

    def func(**kwargs):
        """
        Simple function wrapper with timeout
        """
        func_timeout(timeout, fun, kwargs=kwargs)

    return func


class Cron:
    """
    DBaaS container cron
    """
    def __init__(self, config, config_path):
        self.logger = logging.getLogger('main')
        self.jobs = {}
        self.config_path = config_path
        self.status_path = config['status_path']
        self.sched = BackgroundScheduler()
        self.sched.add_listener(self.update_status, EVENT_JOB_EXECUTED | EVENT_JOB_ERROR)
        self.update(config)
        self.should_run = True
        self.status_lock = Lock()

    def update(self, config):
        """
        Update cron internal state with config
        """
        modules_path = config['modules_path']
        self.status_path = config['status_path']
        seen = set()
        for task in config['tasks']:
            seen.add(task['id'])
            func = resolve(modules_path, task['module'], task['timeout'])
            job = self.sched.add_job(func,
                                     kwargs=task['args'],
                                     misfire_grace_time=task['misfire_grace_time'],
                                     id=task['id'],
                                     replace_existing=True,
                                     max_instances=1,
                                     coalesce=True,
                                     name=task['module'],
                                     **task['schedule'])
            self.jobs[task['id']] = job

        current_jobs = list(self.jobs.keys())
        for job_id in current_jobs:
            if job_id not in seen:
                self.logger.info('Removing %s from schedule', job_id)
                self.jobs[job_id].remove()
                del self.jobs[job_id]

    def update_status(self, event):
        """
        Event listener populating status file
        """
        # https://github.com/PyCQA/pylint/issues/782
        # pylint: disable=E1129
        with self.status_lock:
            self.logger.debug('Updating status of %s', event.job_id)
            try:
                with open(self.status_path) as status_file:
                    status = json.loads(status_file.read())
            except Exception:
                status = {}

            rm_list = []
            for job_id in status:
                if job_id not in self.jobs:
                    rm_list.append(job_id)
            for job_id in rm_list:
                self.logger.info('Removing %s from status', job_id)
                del status[job_id]

            if event.exception:
                status[event.job_id] = {
                    'success': False,
                    'error': event.traceback,
                }
            else:
                status[event.job_id] = {'success': True}

            with open(self.status_path + '.tmp', 'w') as status_file:
                status_file.write(json.dumps(status))

            os.rename(self.status_path + '.tmp', self.status_path)

    def reload(self, *_):
        """
        Reload cron configuration (should be used as sighup handle)
        """
        conf = load_config(self.config_path)
        self.update(conf)

    def stop(self, *_):
        """
        Stop cron
        """
        self.logger.info('Stopping')
        self.should_run = False
        if self.sched.running:
            self.sched.shutdown(wait=False)

    def start(self):
        """
        Start cron iterations
        """
        self.sched.start()
        while self.should_run:
            logging.debug('Cron is running')
            time.sleep(1)


def _main():
    parser = argparse.ArgumentParser()

    parser.add_argument('-c', '--config', default='/etc/dbaas-cron.conf', type=str, help='Config path')
    args = parser.parse_args()

    config = load_config(args.config)

    logging.basicConfig(level=getattr(logging, config['log_level'], None),
                        handlers=[logging.FileHandler(config['log_file'])],
                        format='%(asctime)s [%(levelname)s] %(name)s:\t%(message)s')

    cron = Cron(config, args.config)

    signal.signal(signal.SIGHUP, cron.reload)

    signal.signal(signal.SIGTERM, cron.stop)

    cron.start()


if __name__ == '__main__':
    _main()
