# -*- coding: utf-8 -*-
"""
Config
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
        file_path = os.path.dirname(loader.name)
        file_name, file_ext = os.path.splitext(node.value)

        # Read config file if it exists
        config_file = os.path.join(file_path, file_name + file_ext)
        if os.path.isfile(config_file):
            return yaml.load(open(config_file))

        # If no config exists, read default config and override it by environment
        # config or environment default config
        config_file = os.path.join(file_path, file_name + '-default' + file_ext)
        data = yaml.load(open(config_file))

        config_file = os.path.join(file_path, file_name + '-' + env + file_ext)
        if not os.path.isfile(config_file):
            config_file = os.path.join(file_path, file_name + '-' + env + '-default' + file_ext)

        if os.path.isfile(config_file):
            env_data = yaml.load(open(config_file))
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
        args[1].msg = color + str(args[1].msg) +  '\x1b[0m'  # normal
        return fun(*args)
    return new


def load_config(force_lvl=None):
    """Config loader"""
    env = 'development'
    env_file = '/etc/yandex/environment.type'
    if os.path.isfile(env_file):
        with open(env_file) as inf:
            env = inf.read().strip()

    yaml.add_constructor('!include', yaml_include(env))

    confname = '/etc/mysql-configurator/main.yaml'
    hostname = socket.gethostname()
    if not os.path.isfile(confname):
        confname = 'conf/mysql-configurator/main.yaml'
        hostname = 'chrono02i.mdt.yandex.net'

    conf = dotmap.DotMap(yaml.load(open(confname)))

    # need to force log lvl for scripts, that may not write logs
    if force_lvl:
        conf.logging.loggers[''].level = force_lvl

    logging.config.dictConfig(conf.logging.toDict())

    conf.host_name = hostname
    conf.group_name = group_name(conf.conductor.group_url.format(hostname))

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
