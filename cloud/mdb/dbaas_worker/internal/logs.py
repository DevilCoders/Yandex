"""
Logging utils
"""

import logging
from typing import Any

from library.python import svn_version
from raven import Client
from raven.base import DummyClient
from raven.transport.http import HTTPTransport

from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from .types import Task


def get_task_prefixed_logger(task: Task, logger_name: str) -> MdbLoggerAdapter:
    """
    Return logger with task_id

    logger_name: pass __name__ as argument
    """
    return MdbLoggerAdapter(
        logging.getLogger(logger_name),
        extra={
            'task_id': task['task_id'],
            'feature_flags': ','.join(sorted(task['feature_flags'])),
            'folder_ext_id': task['folder_id'],
        },
    )


def new_sentry_client(config: Any) -> Client:
    """
    Create sentry client from config
    """

    if not config.main.sentry_dsn:
        return DummyClient()
    release = f'1.{svn_version.svn_revision()}'  # noqa

    sentry_client = Client(config.main.sentry_dsn, transport=HTTPTransport, release=release)
    sentry_client.environment = config.main.sentry_environment
    return sentry_client
