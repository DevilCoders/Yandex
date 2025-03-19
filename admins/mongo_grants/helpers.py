# -*- coding: utf-8 -*-
""" The tools entry point and stuff """

import os
import re
import sys
import time
import json
import shutil
import traceback
import socket

from datetime import datetime, date

try:
    from urllib2 import urlopen
except ImportError:
    from urllib import urlopen
from subprocess import Popen, PIPE

import pymongo

KEY_FILE = "/etc/mongo.key"

ETC_FILE = "/etc/mongo-client.json"
RC_FILE = "/root/.mongorc.js"
RC_MARK = "/*M_GRANTS_RC_{0}*/"
RC_TEXT = "{0} (function(){{db.getSiblingDB('admin').auth('{1}','{2}')}}());\n"
RC_AUTH_RE = r".+?\)\.auth\('(?P<name>[^']+)'[^']+'(?P<passwd>[^']+)'.+"


def json_serial(obj):
    """JSON serializer for objects not serializable by default json code"""

    if isinstance(obj, (datetime, date)):
        return obj.isoformat()
    raise TypeError("Type %s not serializable" % type(obj))


class Console(object):
    """ Represents the ANSI console abstraction """
    palitra = {'error': 31, 'ok': 32, 'warn': 33, 'trace': '1;0',
               'notice': '3;34', 'info': '36', 'debug': 37}

    def __init__(self, debug=0):
        self.debug_level = debug
        self.attr = "info"
        self.color = self.palitra['info']

    def __getattr__(self, attr):
        self.attr = attr
        if attr == 'debug' and not self.debug_level:
            return lambda _: None
        if attr == 'trace' and self.debug_level < 3:
            return lambda _: None
        if attr == 'exception':
            return self.exception
        self.color = self.palitra.get(attr, 0)
        return self.say

    def say(self, msg):
        """ Write text to the console with ANSI prefix """
        print("\x1b[{0}m{2:<8}: {1}\x1b[0m".format(
            self.color, msg, self.attr.upper()))

    def exception(self, err):
        """ Dumps an exception """
        self.error("Exception {0[0].__name__}: {0[1]}!".format(err))
        if self.debug_level > 1:
            self.debug(traceback.format_exc(err[2]))
            self.debug("[35m{0}".format('=' * 80))


CONSOLE = Console()


def handle_error(msg=None, retry=1, exit_code=None, exception=None):
    """ The decorator that dumps errors and exits """
    def decorator(func):
        """ deco """
        def wrapper(*_args, **kwargs):
            """ wrapper """
            for i in range(retry):
                try:
                    time.sleep(pow(i, 2))
                    return func(*_args, **kwargs)
                except Exception:           # pylint: disable=W0703
                    exc = sys.exc_info()
                    if exception and exception != exc[0].__name__:
                        raise
                    if i:
                        CONSOLE.warn("Retry {1} #{0}".format(i, func.__name__))
                    CONSOLE.exception(exc)
            if msg:
                CONSOLE.error(msg)
            if exit_code:
                sys.exit(exit_code)
        return wrapper
    return decorator


def run_shell(cmd):
    """ Run the shell and write about it """
    CONSOLE.debug("Execute '{0}'".format(" ".join(cmd)))
    pipe = Popen(cmd, stdout=PIPE, stderr=PIPE)
    stdout_data, stderr_data = pipe.communicate()
    if pipe.returncode:
        CONSOLE.error(stderr_data)
        sys.exit(1)
    else:
        return stdout_data


def dict2tuple(dictonary):
    """ Convert a dict to a tuple """
    return tuple((k, dictonary[k]) for k in sorted(dictonary.keys()))


def grep_config(string, conf, default=None, regexp=False):
    """ Greps a config """
    if os.path.exists(conf):
        CONSOLE.debug("Find line `{0}' in config {1}".format(string, conf))
        with open(conf) as cfile:
            for line in cfile:
                line = line.split("#")[0].strip()
                if re.search(r'{0}\s*[:=]'.format(string), line):
                    msg = "Found right line in config {0}: {1}".format(conf,
                                                                       line)
                    CONSOLE.debug(msg)
                    if regexp:
                        line = line.partition(string)[2]
                        res = re.search(regexp, line).group()
                        msg = "Get {0}={1} by regexp {2}".format(string,
                                                                 res, regexp)
                    else:
                        res = re.split(r'\s*[:=]\s*', line)[-1].strip()
                        msg = r"Get {0}={1}, split string by '\s*[:=]\s*'".format(
                            string, res)
                    CONSOLE.debug(msg)
                    return res
    else:
        msg = "Try find `{0}' in config, but config {1} not exists! Skip."
        CONSOLE.error(msg.format(string, conf))
    return default


@handle_error(msg="Conductor not alive? Exiting", retry=3, exit_code=1)
def conductor(method, fmt=None):
    """ Fetch info from a conductor """
    url = 'http://c.yandex-team.ru/api-cached/{0}/{1}'.format(method, socket.getfqdn())
    CONSOLE.debug("conductor query {0}".format(url))
    data = 'format={0}'.format(fmt) if fmt else None
    response = urlopen(url, data=data)
    return response.read().strip()


def find_port(path):
    """ Finds a mongodb port """
    port = None
    config = '/etc/{0}.conf'.format(path)
    port = grep_config('port', config, default=27017, regexp=r"\d+")

    if not port:
        CONSOLE.warn("Fail to autodetect mongo port")
        port = input("[1;31mEnter port number: [0m")
    port = int(port)
    return port


@handle_error(exit_code=1)
def mongorc(mode, user=None, dry_run=True):
    """ Writes mongorc config to /root """
    marker = "/*M_GRANTS_RC_{0}*/".format(mode)
    rcl = []
    if os.path.exists(RC_FILE):
        with open(RC_FILE, 'r') as rcfile:
            rcl = rcfile.readlines()
        if not user:
            root = [x for x in rcl if marker in x]
            root = root[-1] if root else ""
            root = re.search(RC_AUTH_RE, root)
            root = root.groupdict() if root else None
            if root:
                msg = "Get credentials from mongorc.js for `{0[name]}'"
                CONSOLE.ok(msg.format(root))
            return root

    if user:
        CONSOLE.ok("Fix mongorc.js, add {0[name]}'s credentials.".format(user))
        rcl = [x for x in rcl if marker not in x]
        mark = RC_MARK.format(mode)
        rcl.append(RC_TEXT.format(mark, user["name"], user["passwd"]))
        if rcl:
            backup_rc = "{0}.backup.{1}".format(RC_FILE, mode)
            CONSOLE.debug("Backup mongorc to {0}".format(backup_rc))
            if not dry_run:
                if os.path.exists(RC_FILE):
                    shutil.copy(RC_FILE, backup_rc)
                with open(RC_FILE, 'w') as rcfile:
                    rcfile.writelines(rcl)


@handle_error(exit_code=1)
def write_etc_file(user):
    """ Writes a script-readable mongo access config """
    data = {
        "user": user["name"],
        "password": user["passwd"],
        "db": "admin",
    }
    with open(ETC_FILE, 'w') as etcfile:
        etcfile.write(json.dumps(data, sort_keys=True, default=str))


def connect(host='localhost', mode='mongodb', user=None, init=False):
    """ Connects to MongoDB and returns the connection """
    port = find_port(mode)
    CONSOLE.debug("Try connect on host: {0}, port: {1}".format(host, port))
    repl_set = ''
    if mode == 'mongodb' and not init:
        repl_set = grep_config('replSet(Name)?', '/etc/mongodb.conf', default='')
    client = pymongo.MongoClient(host=host, port=port, replicaset=repl_set)
    if user:
        CONSOLE.debug("Auth name={0[name]}, passwd={0[passwd]}, replset='{1}'".format(user, repl_set))
        res = client.admin.authenticate(user['name'], password=user['passwd'])
        CONSOLE.debug("Auth result: {0}".format(bool(res)))
    return client
