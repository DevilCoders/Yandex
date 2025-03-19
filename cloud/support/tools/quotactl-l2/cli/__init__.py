#!/usr/bin/env python3
"""This module contains CLI args."""

import argparse

from quota.version import __version__

OPTIONAL_ARGS = [
       'config', 'token', 'ssl', 'ca', 'preprod',
       'debug', 'version', 'help', 'name', 'limit',
       'set', 'multiply', 'zeroize', 'default',
       'set-from-yaml', 'code', 'services', 'subject',
       'gpn',
]

parser = argparse.ArgumentParser(prog='qctl',
                                 description='Yandex.Cloud Quota Manager',
                                 usage='qctl [service] cloud_id [subcommands] [options]')

parser.add_argument('--version', action='version', version=__version__)

commands = parser.add_argument_group('Commands')
commands.add_argument('--compute', '-c', type=str, metavar='cloud', default=False)
commands.add_argument('--compute-old', type=str, metavar='cloud', default=False)
commands.add_argument('--container-registry', '-cr', metavar='cloud', type=str, default=False)
commands.add_argument('--instance-group', '-ig', type=str, metavar='cloud', default=False)
commands.add_argument('--kubernetes', '-k8s', type=str, default=False, metavar='cloud')
commands.add_argument('--managed-database', '-mdb', type=str, default=False, metavar='cloud')
commands.add_argument('--object-storage', '-s3', type=str, default=False, metavar='cloud')
commands.add_argument('--resource-manager', '-rm', type=str, default=False, metavar='folder')
commands.add_argument('--functions', '-f', type=str, default=False, metavar='cloud')
commands.add_argument('--triggers', '-t', type=str, default=False, metavar='cloud')
commands.add_argument('--virtual-private-cloud', '-vpc', type=str, default=False, metavar='cloud')
commands.add_argument('--load-balancer', '-lb', type=str, default=False, metavar='cloud')
commands.add_argument('--monitoring', '-m', type=str, default=False, metavar='cloud')
commands.add_argument('--internet-of-things', '-iot', type=str, default=False, metavar='cloud')

subcommands = parser.add_argument_group('Subcommands of [service]')
subcommands.add_argument('--set', '-s', action='store_true', help='set quota metrics')
subcommands.add_argument('--name', '-n', metavar='str', type=str, help='quota metric name [for --set]')
subcommands.add_argument('--limit', '-l', metavar='str', type=str, help='quota metric limit [for --set]')
subcommands.add_argument('--multiply', type=int, metavar='N', help='multiply service metrics to multiplier')
subcommands.add_argument('--show-balance', action='store_true', default=False,
                         help='show billing metadata in interactive mode')

addons = parser.add_argument_group('Other commands')
addons.add_argument('--zeroize', type=str, metavar='cloud', help='set metrics to zero')
addons.add_argument('--default', type=str, metavar='cloud', help='set metrics to default')
addons.add_argument('--services', type=str, metavar='services',  help='service1,service2,.. [for --zeroize/default]')
addons.add_argument('--code', '-q', type=str, metavar='code', help='update metrics from quotacalc')
addons.add_argument('--set-from-yaml', type=str, metavar='file', help='set quota from yaml')
addons.add_argument('--subject', type=str, metavar='cloud', help='optional id for --set-from-yaml')

options = parser.add_argument_group('Options')
options.add_argument('--config', type=str, metavar='file', help='path to config file')
options.add_argument('--token', type=str, metavar='str', help='iam or oauth token')
options.add_argument('--ssl', '--ca', type=str, metavar='file', help='path to AllCAs.pem')
options.add_argument('--preprod', action='store_true', help='preprod environment')
options.add_argument('--gpn', action='store_true', help='gpn environment')
options.add_argument('--debug', '-v', action='store_true', help='debug mode')
options.add_argument('--raw', '-r', action='store_true', help='print raw quotas value, not in human-readable format ') # DL ADD

args = parser.parse_args()
