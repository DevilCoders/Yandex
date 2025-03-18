# coding: utf-8

from __future__ import unicode_literals

import requests

from tools.releaser.src.conf import cfg


def get_normal_session():
    session = requests.Session()
    return session


def get_internal_session():
    session = get_normal_session()
    session.verify = cfg.INTERNAL_CA_PATH
    return session
