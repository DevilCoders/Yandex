# -*- coding: utf-8 -*-
"""
Module for managing files in MDB.
"""

from __future__ import unicode_literals

import json
import logging
import requests
import os

try:
    import six
except ImportError:
    from salt.ext import six

__salt__ = {}

_default = object()
log = logging.getLogger(__name__)


def __virtual__():
    return True


def download_object(url, use_service_account_authorization):
    """
    Download file, with obtaining iam token if needed.
    """
    headers = {}
    if use_service_account_authorization:
        response = requests.get(
            "http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token",
            headers={'Metadata-Flavor': 'Google'},
        )
        if response.status_code != 200:
            raise RuntimeError("Failed to get IAM token, response: {}".format(response.text))
        data = json.loads(response.content)
        if data["token_type"] != "Bearer":
            raise RuntimeError("Unexpected token type, response: {}".format(response.text))
        headers = {'X-YaCloud-SubjectToken': data["access_token"]}
    response = requests.get(url, headers=headers)
    if response.status_code != 200:
        raise RuntimeError("Failed to get data, response: {}".format(response.text))
    return response.content


def path_exists(path):
    return os.path.exists(path)
