# coding: utf-8
from __future__ import unicode_literals, absolute_import

import socket
from importlib import import_module as imp


current_host = socket.gethostname()


def import_object(name, sep='.', **kwargs):
    sep = ':' if ':' in name else sep
    module_name, _, cls_name = name.rpartition(sep)

    if not module_name:
        cls_name, module_name = None, cls_name

    module = imp(module_name, **kwargs)

    return getattr(module, cls_name) if cls_name else module
