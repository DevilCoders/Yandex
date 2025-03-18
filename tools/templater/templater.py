#!/usr/bin/env python
# $Id$
# $HeadURL$

import os
import sys
import argparse
import logging
import socket
import jinja2
import shutil
from ygetparam import YGetParam, GetParam, ExternalResolve, LoadJSON

TMPL_DIR = 'tmpl'
DST_DIR = '.'
LOG_MSG_FORMAT = '%(asctime)s %(levelname)s: %(message)s'
LOG_LEVELS = ('debug', 'info', 'warning', 'error', 'critical')

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)
formatter = logging.Formatter(LOG_MSG_FORMAT)


class Env(dict):
    def __init__(self, data=None):
        self.data = dict() if data is None else data

    def __getattr__(self, name):
        if self.data[name].isdigit():
            return int(self.data[name])
        return self.data[name]


def return_tuple(data):
    ''' * -> tuple
    '''
    if not isinstance(data, tuple):
        return (data,)
    return data


def short_hostname(hostname):
    ''' str -> str
    '''
    return hostname.split(".")[0]


def ygetparam_instance(service, param):
    ''' str, str -> tuple
    Return param from  manifest retrived by a service name.
    '''
    return return_tuple(GetParam(service, param))


def ygetparam_manifest(file_name, param):
    ''' str, str -> str
    Load json and pass thru ygetparam
    '''
    return return_tuple(GetParam(LoadJSON(file_name), param))


def ygetparam_module(module_name, string):
    ''' str, str -> str
    Load json and pass thru ygetparam
    '''
    return ExternalResolve(module_name, string)


def handle_exception(exc, filename):
    logging.error("Failed handling file: {}".format(filename), exc_info=True)
    return False


if __name__ == "__main__":
    # Parse cmd arguments
    args_parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description='Retrieve specified parameter value for given instance.')
    args_modificators = args_parser.add_mutually_exclusive_group()
    args_modificators.add_argument('-t', '--templates',
                                   type=str,
                                   default=TMPL_DIR,
                                   help='where to find templates '
                                        '[default: %(default)s ]')
    args_modificators2 = args_parser.add_mutually_exclusive_group()
    args_modificators2.add_argument('-d', '--destination',
                                    type=str,
                                    default=DST_DIR,
                                    help='where to store resulting file structures '
                                         '[default: %(default)s ]')
    args_parser.add_argument('--ignore-errors',
                             dest='ignore_errors',
                             action='store_true',
                             help='ignore failed files '
                                  '[default: %(default)s]')
    args_parser.add_argument('-l', '--log-file',
                             dest='log_file',
                             type=str,
                             default=None,
                             help='where to write log '
                                  '[default: STDERR ]')
    args_parser.add_argument('--log-level',
                             dest='log_level',
                             type=str,
                             choices=LOG_LEVELS,
                             default='error',
                             action='store',
                             help='set logging level '
                                  '[default: %(default)s ]')

    args = args_parser.parse_args()

    # Enable logging
    numeric_level = getattr(logging, args.log_level.upper(), None)
    if args.log_file:
        logHandler = logging.FileHandler(args.log_file)
    else:
        logHandler = logging.StreamHandler()
    logHandler.setLevel(numeric_level)
    logHandler.setFormatter(formatter)
    logger.addHandler(logHandler)

    # Set some service params (hacks around SWAT-2316,SWAT-2335)
    svc = {"name": "_".join(os.path.basename(os.path.abspath(".")).split("_")[1:-1]),
           "work_directory": os.path.abspath("."),
           "current_host": socket.gethostname()}

    loader = jinja2.FileSystemLoader(args.templates)
    templates = jinja2.Environment(loader=loader)

    funcs = {'env': Env(os.environ),
             'ygetparam': ygetparam_instance,
             'ygetparam_module': ygetparam_module,
             'ygetparam_manifest': ygetparam_manifest,
             'svc': Env(svc),
             'short_hostname': short_hostname}

    for item in templates.list_templates():
        logging.info("Processing file: {}".format(item))
        template = templates.get_template(item)
        try:
            rendered_file = template.render(funcs)
        except Exception as e:
            if not handle_exception(e, item):
                if args.ignore_errors:
                    continue
                else:
                    sys.exit(1)
        # Structure for final file
        full_file_path = os.path.join(args.destination, item)
        dir_name = os.path.dirname(full_file_path)
        if not os.path.exists(dir_name):
            os.makedirs(dir_name)
        with open(full_file_path, 'w') as f:
            f.write(rendered_file)
        shutil.copymode(os.path.join(args.templates, item), full_file_path)
