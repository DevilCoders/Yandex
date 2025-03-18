# coding: utf-8

from __future__ import unicode_literals

import os

import requests


_NOT_SET = object()
_ca_bundle_path = _NOT_SET


def get_ca_bundle_path():
    """
    @attention:
        Add yandex-internal-root-ca debian package in your runtime depends
    """
    global _ca_bundle_path
    if _ca_bundle_path is _NOT_SET:
        standard_ca_bundle_path = '/etc/ssl/certs/ca-certificates.crt'
        if os.path.exists(standard_ca_bundle_path):
            _ca_bundle_path = standard_ca_bundle_path
        else:
            _ca_bundle_path = None
    return _ca_bundle_path


def get_secure_session():
    session = requests.Session()
    ca_bundle_path = get_ca_bundle_path()
    session.verify = ca_bundle_path or False
    return session
