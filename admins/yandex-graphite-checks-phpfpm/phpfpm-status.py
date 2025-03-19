#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from subprocess import Popen, STDOUT, PIPE
import re
import os
import json
from collections import Counter
from configparser import ConfigParser
from argparse import ArgumentParser
from termcolor import colored, cprint
from datetime import datetime
import time


class Printer(object):
    """
    Class with some methods for stdout print
    """
    def __init__(self, mode, result):
        f = getattr(self, mode, result)
        f(result)

    @staticmethod
    def console(data):
        """
        Method for colored console stdout
        :param data:
        :return:
        """
        c_idle = 'blue'
        c_running = 'yellow'
        c_finishing = 'magenta'

        idle = data['state_Idle']
        running = data['state_Running']
        finishing = data['state_Finishing']

        total = sum([idle, finishing, running])
        idle_prc = round(float(idle) / total * 100, 1)
        running_prc = round(float(running) / total * 100, 1)
        finishing_prc = round(float(finishing) / total * 100, 1)

        text = "{dt}: {run_val}{fin_val}{idle_val}\t{run_descr} {idle_descr}".format(
            dt = datetime.now().strftime('%c'),
            run_val=colored('*', c_running, attrs=['reverse']) * running,
            run_descr=colored('running={}%'.format(running_prc), c_running, attrs=['reverse']),
            fin_val=colored('.', c_finishing, attrs=['reverse']) * finishing,
            idle_val=colored('_', c_idle, attrs=['reverse']) * idle,
            idle_descr=colored('idle={}%'.format(idle_prc), c_idle, attrs=['reverse']),
        )

        print(text)

    @staticmethod
    def graphite(data):
        """
        Method for graphite format stdout
        :param data:
        :return:
        """
        for key, val in iter(data.items()):
            print('{} {}'.format(key, val))


def parse_args():
    """
    Function for parser script arguments
    :return:
    """
    p = ArgumentParser()
    p.add_argument('--mode', default='graphite', choices=['graphite', 'console'], help='stdout mode')
    p.add_argument('--cycle', action='store_true', help='run script with infinite cycle')
    p.add_argument('--sleep', type=float, default=1.0, help='pause between cycle steps')
    p.add_argument('--socket', help='socket name filter')
    p.add_argument('--pool', help='pool name filter')
    p.add_argument('--debug', action='store_true')
    return p.parse_args()
    

def check_cgi_fcgi_utility():
    """
    Check availability of cgi-fcgi utility
    :return:
    """
    proc = Popen('which cgi-fcgi', shell=True, stdout=PIPE, stderr=STDOUT)
    proc.communicate()
    assert proc.returncode == 0, 'Error when check cgi-fcgi utility from libfcgi0ldbl package'


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
    p = Popen('cgi-fcgi -bind -connect {}'.format(options['listen']), shell=True, stdout=PIPE, stderr=STDOUT)
    for line in iter(p.stdout.readline, ''):
        if not line:
            break
        try:
            return json.loads(line.decode())
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
    return result


def main():
    check_cgi_fcgi_utility()
    args = parse_args()
    while True:
        result = Counter()
        for cfg_file_name in discovery_configs():
            try:
                # {'www': {'listen': '/var/run/php5-fpm.sock', 'pm.status_path': '/phpfpm-status'}}
                settings = get_settings(cfg_file_name)
                for section, options in iter(settings.items()):
                    if args.pool and section != args.pool:
                        continue
                    if args.socket and options['listen'] != args.socket:
                        continue
                    stat = cgi_fcgi_request(options)
                    process_list = stat['processes']
                    for proc in process_list:
                        proc_state = proc['state']
                        key = 'state_{}'.format(proc_state)
                        result[key] += 1
            except (KeyError, TypeError):
                if not args.debug:
                    continue
                import traceback
                print(traceback.format_exc())
        Printer(args.mode, result)
        if not args.cycle:
            break
        time.sleep(args.sleep)


if __name__ == '__main__':
    try:
        main()
    except (KeyboardInterrupt, SystemExit):
        pass
