#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import json
import os
import logging
import logging.handlers
import subprocess
import sys
import configparser

# More details in https://st.yandex-team.ru/MDB-13800
MDB_CHECKS = [
    'oldTemporalCheck',
    'mysqlSchemaCheck',
    'foreignKeyLengthCheck',
    # yes, there is a typo in JSON output
    # proof: https://github.com/mysql/mysql-shell/blob/8.0.26/modules/util/upgrade_check.cc#L599
    'enumSetElementLenghtCheck',
    'partitionedTablesInSharedTablespaceCheck',
    'circularDirectoryCheck',
    'removedFunctionsCheck',
    'groupByAscCheck',
    'removedSysLogVars',
    'schemaInconsistencyCheck',
    'ftsTablenameCheck',
    'engineMixupCheck',
    'oldGeometryCheck',
    'checkTableOutput',
]

MYSQLSH_CHECKER_MIN_VERSION = (8, 0, 0)

def get_logger():
    """
    Initialize logger
    """
    logging.basicConfig()
    log = logging.getLogger()
    log.setLevel(logging.DEBUG)
    return log


def upgrade_checker(log, args):
    target_version_num = tuple(int(i) for i in args.target_version.split('.'))
    if target_version_num < MYSQLSH_CHECKER_MIN_VERSION:
        # TODO: write python euristics for 5.7 ?
        return

    config = configparser.ConfigParser()
    config.read(args.defaults_file)

    cmd = "mysqlsh  -- util check-for-server-upgrade \
        {{ --user={user} --password=\"{password}\" --host={host} --port={port} }} \
        --output-format=JSON \
        --config-path={mysql_config_path} \
        --target-version=\"{target_version}\"".format(
        user=config['client']['user'],
        password=config['client']['password'],
        host=config['client']['host'],
        port=config['client']['port'],
        mysql_config_path=args.config_path,
        target_version=args.target_version,
    )
    proc = subprocess.run(cmd, shell=True, check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    print("mysqlsh stderr: ", proc.stderr.decode(), file=sys.stderr)
    if proc.returncode != 0:
        log.info("Failed to run upgrade checker")

    out = json.loads(proc.stdout)

    result = []
    for check in out['checksPerformed']:
        if check['id'] in MDB_CHECKS and len(check['detectedProblems']) != 0:
            description = check.get('description', check.get('title', ''))
            objects = [x['dbObject'] for x in check['detectedProblems']]
            result.append(description + " [" + ','.join(objects) + "]")

    # print to stdout, so we can use it
    print('\n'.join(result))


def main():
    """
    Console entry-point
    """

    parser = argparse.ArgumentParser()
    parser.set_defaults(func=upgrade_checker)
    parser.add_argument(
        '--defaults-file',
        type=str,
        default=os.path.expanduser("~/.my.cnf"),
        help='path for my.cnf with login/password for cluster',
    )
    parser.add_argument(
        '--config-path',
        type=str,
        default='/etc/mysql/my.cnf',
        help='config will be parsed in order to find deprecated value',
    )
    parser.add_argument(
        '--target-version',
        type=str,
        required=True,
        help='target MySQL version (for example "8.0.20")',
    )
    log = get_logger()
    args = parser.parse_args()
    args.func(log, args)


if __name__ == '__main__':
    main()
