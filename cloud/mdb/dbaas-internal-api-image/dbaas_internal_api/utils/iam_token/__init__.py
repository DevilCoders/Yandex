"""
iam api and client for service_account service
"""

from threading import local
from flask import current_app

from .api import IAM
from .client import IAMClient

THREAD_CONTEXT = local()


def get_iam_client() -> IAM:
    """
    Get IAM provider according to config and flags
    """
    if not getattr(THREAD_CONTEXT, 'client', None):
        config = current_app.config['IAM_TOKEN_CONFIG']
        klass = config.get('class', IAMClient)
        THREAD_CONTEXT.client = klass(config)
    return THREAD_CONTEXT.client
