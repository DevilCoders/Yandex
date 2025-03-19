"""
Pytest fixtures definitions
"""

from io import StringIO
from logging import getLogger
from logging.config import dictConfig

import pytest


@pytest.fixture
def logger():
    """
    Just retuns logger
    """
    return getLogger()


@pytest.fixture
def stream():
    """
    Create testing stream
    """
    log_stream = StringIO()

    dictConfig(
        {
            'version': 1,
            'disable_existing_loggers': False,
            'formatters': {
                'default': {
                    '()': 'dbaas_common.tskv.TSKVFormatter',
                    'tskv_format': 'test',
                },
            },
            'handlers': {
                'streamhandler': {
                    'level': 'DEBUG',
                    'class': 'logging.StreamHandler',
                    'formatter': 'default',
                    'stream': log_stream,
                },
            },
            'loggers': {
                '': {
                    'handlers': ['streamhandler'],
                    'level': 'DEBUG',
                },
            },
        }
    )
    return log_stream
