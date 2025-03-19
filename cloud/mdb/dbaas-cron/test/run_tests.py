#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# pylint: disable=no-self-use,missing-docstring

import json
import os
import shutil
import signal
import subprocess
import sys
import time
import types
import unittest
from copy import deepcopy

sys.path.insert(0, '.')
import cron


class TestLoadConfig(unittest.TestCase):
    def test_equals_json_loads(self):
        config = cron.load_config('test/config.json')
        config_without_tasks = deepcopy(config)
        del config_without_tasks['tasks']
        with open('test/config.json') as config_file:
            self.assertDictEqual(config_without_tasks, json.loads(config_file.read()))
        task_dir = 'test/tasks/'
        for task_file in os.listdir(task_dir):
            with open(os.path.join(task_dir, task_file)) as fil:
                json_task = json.loads(fil.read())
            for task in config['tasks']:
                if task['id'] == json_task['id']:
                    self.assertDictEqual(task, json_task)
                    break
            else:
                raise AssertionError('Task {id} not found in config'.format(id=task['id']))

    def test_validation(self):
        with open('test/config.json') as config_file:
            orig = json.loads(config_file.read())
        orig['log_level'] = 'incorrect'
        with open('test/malformed_config.json', 'w') as config_file:
            config_file.write(json.dumps(orig))
        with self.assertRaises(RuntimeError):
            cron.load_config('test/malformed_config.json')
        os.unlink('test/malformed_config.json')


class TestResolve(unittest.TestCase):
    def test_successful_resolve(self):
        assert isinstance(cron.resolve('test', 'sleep_time', 1), types.FunctionType)

    def test_fail_resolve(self):
        with self.assertRaises(RuntimeError):
            cron.resolve('test', 'incorrect_sleep_time.py', 1)


class TestCmdLine(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        shutil.copy('test/config.json', 'test/test_config.json')
        cls.proc = subprocess.Popen([
            './venv/bin/coverage',
            'run',
            '-p',
            '--include=cron.py',
            './cron.py',
            '-c',
            'test/test_config.json',
        ])
        time.sleep(5)

    @classmethod
    def tearDownClass(cls):
        cls.proc.terminate()
        time.sleep(5)
        for path in ['test/test_config.json', 'test/status.json', 'test/tasks/test2.json']:
            if os.path.exists(path):
                os.unlink(path)

    def test_status(self):
        with open('test/status.json') as status_file:
            status = json.loads(status_file.read())

        self.assertEqual(status['test']['success'], True)
        self.assertEqual(status['test_fail']['success'], False)

    def test_reload(self):
        orig = cron.load_config('test/config.json')
        new_conf = deepcopy(orig)
        with open('test/tasks/test.json') as fil:
            orig_task_content = fil.read()
        for idx, task in enumerate(new_conf['tasks']):
            if task['id'] == 'test':
                new_task = deepcopy(task)
                del new_conf['tasks'][idx]
                break
        else:
            raise RuntimeError('Task with id "test" not found')
        new_task['id'] = 'test2'
        new_conf['tasks'].append(new_task)
        with open('test/test_config.json', 'w') as config_file:
            config_file.write(json.dumps(new_conf))
        with open('test/tasks/test.json', 'w') as conf_file:
            conf_file.write(json.dumps(new_task))
        os.rename('test/tasks/test.json', 'test/tasks/test2.json')
        self.proc.send_signal(signal.SIGHUP)
        time.sleep(5)

        with open('test/status.json') as status_file:
            status = json.loads(status_file.read())

        self.assertEqual(status['test2']['success'], True)
        self.assertEqual(status['test_fail']['success'], False)

        with open('test/test_config.json', 'w') as config_file:
            config_file.write(json.dumps(orig))
        os.unlink('test/tasks/test2.json')
        with open('test/tasks/test.json', 'w') as conf_file:
            conf_file.write(orig_task_content)
        self.proc.send_signal(signal.SIGHUP)
        time.sleep(5)


if __name__ == '__main__':
    unittest.main()
