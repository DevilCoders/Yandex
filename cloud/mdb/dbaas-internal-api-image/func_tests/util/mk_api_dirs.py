"""
Make root dirs for tmp and logs
"""

import os
from types import SimpleNamespace

from . import project


def mk_api_dirs(context: SimpleNamespace) -> None:
    """
    Make logs and tmp roots. Write them to context if it required
    """
    if not hasattr(context, 'tmp_root'):
        context.tmp_root = project.tmp_root()
    if not hasattr(context, 'logs_root'):
        context.logs_root = project.logs_root()
    os.makedirs(context.tmp_root, exist_ok=True)
    os.makedirs(context.logs_root, exist_ok=True)
