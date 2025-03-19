"""
Logs save handler
"""

import json
import logging
import os
import sys
from distutils.dir_util import copy_tree
from logging.handlers import RotatingFileHandler

from retrying import retry

from .helpers.compute_driver import get_driver
from .helpers.hadoop_cluster import HadoopCluster
from .helpers.utils import context_to_dict


def init_logger():
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s [%(levelname)s]:\t%(message)s')
    console_handler = logging.StreamHandler(stream=sys.stdout)
    console_handler.setLevel(logging.INFO)
    console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)
    debug_log_size = 10 * 2**20
    file_handler = RotatingFileHandler('staging/logs/test.log', maxBytes=debug_log_size)
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)


def save_context(context, logs_dir):
    """
    Save behave context
    """
    with open(os.path.join(logs_dir, 'context.json'), 'w') as out:
        json.dump(context_to_dict(context), out, default=repr, indent=4)


@retry(wait_fixed=250, stop_max_attempt_number=20)
def save_logs(context):
    """
    Save logs and support materials
    """
    logs_dir = os.path.join(context.conf['staging_dir'], 'logs', context.scenario.name.replace(' ', '_'))
    os.makedirs(logs_dir, exist_ok=True)

    save_context(context, logs_dir)

    # Copy logs from controlplane
    compute_driver = get_driver(context.state, context.state['config'])
    compute_driver.save_logs(logs_dir)

    # Copy yc logs
    from_dir = os.path.join('venv', 'logs')
    to_dir = os.path.join(logs_dir, 'yc')
    if os.path.exists(from_dir):
        copy_tree(from_dir, to_dir)


def save_diagnostics(context, logs_dir):
    """
    Collect and download diagnostics.tgz from all nodes of the cluster
    """
    if 'cluster' in context:
        diagnostics_path = os.path.join(logs_dir, 'diagnostics')
        os.makedirs(diagnostics_path, exist_ok=True)
        HadoopCluster(context).download_diagnostics(diagnostics_path)
