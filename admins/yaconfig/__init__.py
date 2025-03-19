"""
The easy config-loading solution

Knows about the environment and extends generic configs
with their environmental counterparts.

I.e.:
    foo.yaml:
        foo: 10
        bar: 20
    foo-testing.yaml
        foo: 15

    in testing env foo will be 15 and bar 20

Usage:
    from yaconfig import load_config
    conf = load_config('foo.yaml')
"""

import os
import logging.config
import sys
import yaml
import dotmap


def _yaml_include(env):
    """
    An extension for pyyaml that enables !include

    Arguments:
        env {str} -- The environment type
    """

    def inner(loader, node):
        """ The real extension with parent env """
        file_name = os.path.join(os.path.dirname(loader.name), node.value)
        data = yaml.load(open(file_name))

        file_env_name = file_name.replace('.yaml', '-' + env + '.yaml')
        if os.path.isfile(file_env_name):
            env_data = yaml.load(open(file_env_name))
            data.update(env_data)

        return data

    return inner


def _add_coloring_to_emit_ansi(fun):
    """ Adds colors to the output logging """

    def new(*args):
        """ The real colorifier """
        levelno = args[1].levelno
        if levelno >= 50:
            color = '\x1b[31m'  # red
        elif levelno >= 40:
            color = '\x1b[31m'  # red
        elif levelno >= 30:
            color = '\x1b[33m'  # yellow
        elif levelno >= 20:
            color = '\x1b[32m'  # green
        elif levelno >= 10:
            color = '\x1b[35m'  # pink
        else:
            color = '\x1b[0m'  # normal
        args[1].msg = color + str(args[1].msg) + '\x1b[0m'  # normal
        return fun(*args)

    return new


def load_config(confname):
    """
    Load the config from the config file

    Has the !include option enabled.

    If the config has a toplevel logging key specified, sets up logging

    Arguments:
        confname {str} -- The config file name to start from

    Returns:
        dotmap -- The loaded config
    """
    env = 'development'
    env_file = '/etc/yandex/environment.type'
    if os.path.isfile(env_file):
        with open(env_file) as inf:
            env = inf.read().strip()

    yaml.add_constructor('!include', _yaml_include(env))

    if os.path.isfile(confname):
        conf = yaml.safe_load(open(confname))
    else:
        conf = yaml.safe_load(confname)

    conf['environment'] = env

    if 'logging' in conf:
        logging.config.dictConfig(conf['logging'])

    if sys.stdout.isatty():
        logging.StreamHandler.emit = \
            _add_coloring_to_emit_ansi(logging.StreamHandler.emit)

    return dotmap.DotMap(conf)
