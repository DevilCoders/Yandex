"""
dataproc-manager api and client
"""

from threading import local
from flask import current_app

from .api import DataprocManagerAPI
from .client import DataprocManagerClient

THREAD_CONTEXT = local()


def dataproc_manager_client() -> DataprocManagerAPI:
    """
    Returns cached dataproc-manager client instance
    """

    if not getattr(THREAD_CONTEXT, 'client', None):
        config = current_app.config['DATAPROC_MANAGER_CONFIG']
        klass = config.get('class', DataprocManagerClient)
        THREAD_CONTEXT.client = klass(config)

    return THREAD_CONTEXT.client
