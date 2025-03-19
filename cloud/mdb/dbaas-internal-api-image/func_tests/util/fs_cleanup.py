"""
Filesystem cleanup
"""
import os
import shutil


def fs_cleanup(context):
    """
    Remove all temporary files
    """
    shutil.rmtree(context.tmp_root, ignore_errors=True)
    os.makedirs(context.tmp_root, exist_ok=True)
