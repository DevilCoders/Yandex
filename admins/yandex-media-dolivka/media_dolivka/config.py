# -*- coding: utf-8 -*-
"""
Config by paulus@
"""
import time
import logging.config
import os
import socket
import sys
import yaml
import dotmap
import requests


def yaml_include(env):
    """
    Extension of yaml loader for support the yandex/environment.type
    """
    def inner(loader, node):
        """wrapper"""
        file_name = os.path.join(os.path.dirname(loader.name), node.value)
        file_env_name = file_name.replace('.yaml', '-' + env + '.yaml')
        if not os.path.isfile(file_name):
            file_name = file_name.replace('.yaml', '-default.yaml')

        data = yaml.load(open(file_name))
        if os.path.isfile(file_env_name):
            env_data = yaml.load(open(file_env_name))
            data.update(env_data)

        return data
    return inner


def group_name(group_url):
    """Return the conductor group name

    A cached version of the conductor fetcher

    Arguments:
        group_url {str} -- The url with the host embedded

    Returns:
        str -- The group name

    Raises:
        requests.exceptions.HTTPError -- On conductor errors
    """
    if not hasattr(group_name, "result"):
        group_name.result = {}

    res = group_name.result.get(group_url, None)
    if res:
        # last 10 seconds of lifetime of the result try update it
        if time.time() < (res['expire_time'] - 10):
            return res["text"]
        if time.time() < res['expire_time']:
            group_name.result[group_url] = None

    resp = requests.get(group_url)
    if resp.status_code != 200:
        if res:
            return res["text"]
        raise requests.exceptions.HTTPError('Cannot get group name')

    if resp.status_code == 404:
        raise requests.exceptions.HTTPError("Not found")

    res = {
        "text": resp.text.strip(),
        "expire_time": time.time() + 300,
    }
    group_name.result[group_url] = res
    return res["text"]


def add_coloring_to_emit_ansi(fun):
    """ Adds colors to the output logging """
    def new(*args):
        """Wrapper"""
        levelno = args[1].levelno
        if levelno >= 50:
            color = '\x1b[31m' # red
        elif levelno >= 40:
            color = '\x1b[31m' # red
        elif levelno >= 30:
            color = '\x1b[33m' # yellow
        elif levelno >= 20:
            color = '\x1b[32m' # green
        elif levelno >= 10:
            color = '\x1b[35m' # pink
        else:
            color = '\x1b[0m' # normal
        args[1].msg = color + args[1].msg +  '\x1b[0m'  # normal
        return fun(*args)
    return new


def load_config():
    """Config loader"""
    env = 'development'
    env_file = '/etc/yandex/environment.type'
    if os.path.isfile(env_file):
        with open(env_file) as inf:
            env = inf.read().strip()

    yaml.add_constructor('!include', yaml_include(env))

    confname = '/etc/yandex-media-dolivka/main.yaml'
    hostname = socket.gethostname()
    if not os.path.isfile(confname):
        confname = 'conf/main.yaml'

    conf = dotmap.DotMap(yaml.load(open(confname)))
    logging.config.dictConfig(conf.logging.toDict())

    conf.host_name = hostname

    if sys.stdout.isatty():
        logging.StreamHandler.emit = add_coloring_to_emit_ansi(logging.StreamHandler.emit)

    log = logging.getLogger('Config')
    log.info('Running in a %s environment', env)
    return conf

def log2file(filename,
             level=logging.DEBUG,
             fmt='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
            ):
    """Add file handler for logging"""
    handler = logging.handlers.WatchedFileHandler(filename)
    handler.setLevel(level)
    handler.setFormatter(logging.Formatter(fmt))
    logging.getLogger().addHandler(handler)
