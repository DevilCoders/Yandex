#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import logging

import steps.parameters_types as _  # noqa


def before_all(context):
    """
    Setup environment
    """
    context.dbmdb_host = os.environ.get('DBM_POSTGRESQL_RECIPE_HOST')
    context.dbmdb_port = os.environ.get('DBM_POSTGRESQL_RECIPE_PORT')
    context.dbm_port = os.environ.get('MDB_DBM_PORT')
    context.dbm_metrics_port = os.environ.get('MDB_DBM_METRICS_PORT')


def after_scenario(context, scenario):
    """
    Close open connections
    """
    if 'open_connections' in context:
        for conn in context.open_connections:
            try:
                conn.close()
            except Exception as exc:
                logging.warning('Failed to close %r: %s', conn, exc)
