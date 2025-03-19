#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DBaaS Cleaner

Delete old DBaaS clusters in "junk" folder.
"""

import argparse
import logging
import logging.config

from cloud.mdb.cleaner.internal.cleaner import clean as clean_procedure
from cloud.mdb.cleaner.internal.config import get_config
from cloud.mdb.cleaner.internal.metadb import MetaDB
from cloud.mdb.internal.python.ipython_repl import UserNamespace, start_repl


def clean(config):
    """
    Clean action
    """
    try:
        clean_procedure(config)
    except Exception:
        logging.exception('Internal error')
        raise


def repl(config):
    """
    Repl action
    """
    start_repl(
        [
            UserNamespace(MetaDB, 'MetaDB', 'metadb = MetaDB(config)'),
            UserNamespace(config, 'config'),
        ]
    )


COMMANDS = {
    'clean': clean,
    'repl': repl,
}


def _main():
    """
    DBaaS cleaner.

    Delete old database clusters in particular folder.
    """
    parser = argparse.ArgumentParser()

    parser.add_argument('action', choices=COMMANDS.keys(), type=str, help='Action to perform')
    parser.add_argument('-f', '--folder', type=str, help='Folder ID to search databases in')
    parser.add_argument('-m', '--max-age', type=str, help='Age of databases to consider them old and delete')
    parser.add_argument('-t', '--timeout', type=str, help='Maximum amount of time to wait for operation completion')
    parser.add_argument('-a', '--oauth-token', type=str, help='OAuth token for issuing requests to Internal API')
    parser.add_argument('-c', '--config', type=str, default='/opt/yandex/dbaas-cleaner/config.yaml',
                        help='Path config.yaml file')

    args = parser.parse_args()

    config = get_config(
        config_path=args.config,
        folder_id=args.folder,
        timeout=args.timeout,
        max_age=args.max_age,
        oauth_token=args.oauth_token,
    )
    logging.config.dictConfig(config['logging'])
    COMMANDS[args.action](config)
