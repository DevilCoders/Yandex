#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DBaaS Cleaner

Delete old DBaaS clusters in "junk" folder.
"""

import logging
import logging.config

from .metadb import MetaDB
from .logic import (
    stop_clusters,
    delete_clusters,
)


def clean(config):
    stop_list = []
    delete_list = []

    with MetaDB(config) as db:
        if config['skip_ro'] and db.is_read_only():
            logging.info('Cleanup is skipped as MetaDB is in read-only mode')
            return

        if config.get('strategy') == 'delete':
            delete_list = db.get_visible_clusters_to_delete()
        elif config.get('strategy') == 'stop-delete':
            stop_list = db.get_running_clusters_to_stop()
            delete_list = db.get_stopped_clusters_to_delete()
        else:
            raise RuntimeError(f'Unknown strategy: {config.get("strategy")}')

    if not stop_list and not delete_list:
        logging.info('No database clusters older than %s were found', config['max_age'])
        return

    stop_clusters(config, stop_list)
    delete_clusters(config, delete_list)
