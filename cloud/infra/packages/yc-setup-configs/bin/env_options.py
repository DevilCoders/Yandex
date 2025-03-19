#!/usr/bin/env python3

import argparse
import json
import logging
import pprint
import socket
import sys
import yaml
from ycinfra import WalleClient

logging.basicConfig(level=logging.ERROR, format='%(levelname)s - %(message)s')
ENVIRONMENT_DATA_FILE = '/etc/profile.d/environment_data.yaml'


class NoEnvironmentException(Exception):
    pass


def return_environment_options(project, env_data):
    res = dict(env_data['defaults'])
    if project not in env_data['environments']:
        logging.warning('Environments was NOT found for project `%s`:', project)
        return res
    if env_data['environments'][project]:
        res.update(env_data['environments'][project])
    return res


def data_representation(data, output_format):
    if output_format in ('y', 'yaml'):
        return yaml.safe_dump(data)
    elif output_format in ('j', 'json'):
        return json.dumps(data)
    elif output_format in ('p', 'python'):
        return pprint.pformat(data)
    elif output_format in ('v', 'values'):
        return '\n'.join((repr(s) for s in data.values()))
    elif output_format in ('b', 'bash'):
        return '\n'.join(('export {}={}'.format(key, val) for key, val in data.items()))
    elif output_format in ('k', 'keyval'):
        return '\n'.join(('{} = {}'.format(key, val) for key, val in data.items()))


def parse_args():
    parser = argparse.ArgumentParser(description='Environment options')
    parser.add_argument('-l', '--level', action='store',
                        help='Logging level, default: {}'.format(logging.getLevelName(logging.root.level)),
                        choices=['CRITICAL', 'DEBUG', 'ERROR', 'FATAL', 'INFO', 'NOTSET', 'WARN', 'WARNING']
                        )
    parser.add_argument('-n', '--hostname', action='store', help='Use hostname')
    parser.add_argument('-p', '--parametr', action='store', help='Return only this parametr', type=str, default='')
    parser.add_argument('-s', '--strict', action='store_true',
                        help='Exit with error if environment not found for project')
    parser.add_argument('-e', '--environment-data', action='store', default=ENVIRONMENT_DATA_FILE,
                        help='Environment values data source file')
    parser.add_argument('-o', '--output', action='store',
                        help='Output format: yaml (y), json (j), bash (b), keyval (k), python (p), values (v)',
                        type=str, default='k')
    args = parser.parse_args()
    return args


if __name__ == '__main__':
    args = parse_args()
    with open(args.environment_data, 'r') as stream:
        env_data = yaml.safe_load(stream)
    if args.level:
        logging.root.setLevel(getattr(logging, args.level))
    if args.hostname:
        used_hostname = args.hostname
        logging.debug('Using hostname %s given by options', used_hostname),
    else:
        used_hostname = socket.gethostname()
        logging.debug('Using hostname %s returned by OS', used_hostname),
    try:
        project = WalleClient().get_project(used_hostname)
        logging.debug('Received project `%s` for host `%s`', project, args.hostname)
        opts = return_environment_options(project, env_data)
    except NoEnvironmentException as e:
        if args.strict:
            logging.error(e)
            sys.exit(1)
        else:
            logging.info(e)
            opts = {}
    except Exception as e:
        logging.error('Wall-E request error: %s', e)
        sys.exit(1)
    logging.debug('Environment options for project `%s`:', project),
    logging.debug(pprint.pformat(opts)),
    if args.parametr:
        opts = {args.parametr: opts.get(args.parametr, '')}
    print(data_representation(opts, args.output))
