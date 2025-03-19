#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from subprocess import Popen, STDOUT, PIPE
import re
import os
import json
from collections import Counter
from configparser import ConfigParser
from flask import Flask


def check_cgi_fcgi_utility():
    """
    Check availability of cgi-fcgi utility
    :return:
    """
    proc = Popen('which cgi-fcgi', shell=True, stdout=PIPE, stderr=STDOUT)
    assert proc.wait() == 0, 'Error when check cgi-fcgi utility from libfcgi0ldbl package'


def discovery_configs():
    """
    Search runtime configs
    :return:
    """
    socket_pattern = re.compile('root.*php-fpm: master process \((?P<config>.+)\)')
    proc = Popen('ps aux', shell=True, stdout=PIPE, stderr=STDOUT)
    for line in iter(proc.stdout.readline, ''):
        if not line:
            break
        line = line.decode().strip()
        match = socket_pattern.search(line)
        if not match:
            continue
        yield match.group('config')
    proc.wait()


def cgi_fcgi_request(options):
    """
    Send fastcgi request to php-fpm with options from config
    :param options:
    :return:
    """
    os.environ['SCRIPT_NAME'] = options['pm.status_path']
    os.environ['SCRIPT_FILENAME'] = ''
    os.environ['QUERY_STRING'] = "full&json"
    os.environ['REQUEST_METHOD'] = "GET"
    proc = Popen('cgi-fcgi -bind -connect {}'.format(options['listen']), shell=True, stdout=PIPE, stderr=STDOUT)
    proc.wait()
    for line in iter(proc.stdout.readline, ''):
        if not line:
            break
        try:
            result = json.loads(line.decode())
            proc.wait()
            return result
        except ValueError:
            continue


def get_settings(cfg_file_name):
    """
    Read config, search need options and return it
    :param cfg_file_name:
    :return:
    """
    cfg_obj = ConfigParser()
    cfg_obj.read(cfg_file_name)
    need_options = ['listen', 'pm.status_path']
    ignore_sections = ['global']
    result = {}
    for section in cfg_obj.sections():
        if section in ignore_sections:
            continue
        result[section] = {}
        for option in need_options:
            err_msg = 'Option {} not found in section {} of config {}'.format(option, section, cfg_file_name)
            assert cfg_obj.has_option(section, option), err_msg
            value = cfg_obj.get(section, option)
            result[section].update({option: value})
    del cfg_obj
    # result == {'www': {'listen': '/var/run/php5-fpm.sock', 'pm.status_path': '/phpfpm-status'}, ... }
    return result


check_cgi_fcgi_utility()
check_list = []
for config in discovery_configs():
    check_list.append(get_settings(config))


app = Flask(__name__)
app.debug = True


@app.route("/")
def root():
    data = Counter()
    for settings in check_list:
        try:
            for section, options in iter(settings.items()):
                stat = cgi_fcgi_request(options)
                process_list = stat['processes']
                for proc in process_list:
                    proc_state = proc['state']
                    key = '{}.{}'.format(section, proc_state)
                    data[key] += 1
                del stat
                del process_list
        except (KeyError, TypeError):
            continue
    result = {"sensors": []}
    sensors = result['sensors']
    for key, val in iter(data.items()):
        sensors.append({"labels": {"sensor": 'phpfpm-pool.{}'.format(key)}, "value": val})
    del data
    return json.dumps(result, indent=2)


app.run(host='::', port=7887, threaded=True)
