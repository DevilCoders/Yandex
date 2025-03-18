#!/usr/bin/env python
# coding: utf-8

import argparse
import os
import subprocess
import sys


def ParseArgs():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('ip', metavar='Ip4or6', help='Ip v4 or v6 for probe', nargs=1)
    parser.add_argument('--ip2backend', help='Path to ip2backend binary', default=os.path.join(os.path.dirname(__file__), 'ip2backend'))
    parser.add_argument('--deploy-path', help='Remote path to deploy ip2backend binary', default=os.getenv('HOME'))
    parser.add_argument('--fields', help='Collected ip2backend output fields', nargs='*', default=['Cachers', 'Processors'])
    parser.add_argument('--dont-print-fieldname', help='Dont print field name', action='store_true')
    parser.add_argument('--noupdate', help='Dont update remote ip2backend', action='store_true')
    parser.add_argument('--target', help='Select antirobot host(s)', default='I@itype_antirobot')
    # parser.add_argument('--verbose', help='More talkative', action='store_true')
    return parser.parse_args()


def main():
    ops = ParseArgs()

    ip2backend = os.path.realpath(ops.ip2backend)

    if not ops.noupdate:
        # Upload ip2backend binary
        savedCwd = os.getcwd()
        # Если не поменять PWD, то SkyNet зальет файл /SomePath/ip2backend в /HomePath/SomePath/ip2backend
        # вместо /HomePath/ip2backend.
        os.chdir(os.path.dirname(ip2backend))
        subprocess.call([
            'sky',
            'upload',
            '-U',
            '--cqudp',
            os.path.basename(ip2backend),
            ops.deploy_path,
            ops.target,
        ])
        os.chdir(savedCwd)

    # Run deployed ip2backend
    cummulativeOutput = ''
    try:
        cummulativeOutput = subprocess.check_output([
                        'sky',
                        'run',
                        '-U',
                        '--cqudp',
                        ' '.join([os.path.join(ops.deploy_path, os.path.basename(ip2backend))] + ops.ip),
                        ops.target,
                    ])
    except subprocess.CalledProcessError, e:
        cummulativeOutput = e.output

    # Parse output from all hosts
    info = dict()
    for field in ops.fields:
        info[field] = set()
    for s in cummulativeOutput.split('\n'):
        for field in ops.fields:
            if s.startswith(field):
                info[field].add(s.rstrip().split(' ')[1])

    # Print cummulative output
    for field in ops.fields:
        for value in sorted(info[field]):
            if not ops.dont_print_fieldname:
                sys.stdout.write(field + ': ')
            sys.stdout.write(value + '\n')


if __name__ == '__main__':
    main()
