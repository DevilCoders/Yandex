# -*- coding: utf-8 -*-
"""
A module that adds data to the Pillar structure retrieved by an https request
to DBaaS Internal API


Configuring the dbaas_pillar ext_pillar
=======================================

Set the following Salt config to setup dbaas_pillar as external pillar source:

.. code-block:: yaml

  ext_pillar:
    - dbaas_pillar:
        url: https://internal-api-host/api/v1.0/config/
        access_id: <Internal API id>
        access_secret: <Internal API secret>
        api_pub_key: <internal api public key>
        salt_sec_key: <salt secret key>
        target_pillar_id: <optional target id>
        rev: <optional rev>

"""
from __future__ import absolute_import

import json
import logging
import os
import time

try:
    import collections.abc as collections_abc
except ImportError:
    import collections as collections_abc

from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
from requests import Session
from nacl.encoding import URLSafeBase64Encoder as encoder
from nacl.public import Box, PrivateKey, PublicKey


def _decrypt(secret_key, public_key, data):
    """
    Decrypt data using secret key and public key
    """
    version = data['encryption_version']
    if version == 1:
        box = Box(
            PrivateKey(secret_key.encode('utf-8'), encoder),
            PublicKey(public_key.encode('utf-8'), encoder))
        try:
            return box.decrypt(encoder.decode(
                data['data'].encode('utf-8'))).decode('utf-8')
        except:
            log = logging.getLogger(__name__)
            log.error(data['data'].encode('utf-8'))
            raise
    else:
        raise RuntimeError('Unexpected encryption version: %d' % version)


def _rec_decrypt(inp_dict, sec_key, pub_key):
    """
    Recursive dictionary decryption
    """

    def _update(ret, update):
        try:
            for key, value in update.items():
                if isinstance(value, collections_abc.Mapping):
                    if 'encryption_version' in value and 'data' in value:
                        ret[key] = _decrypt(sec_key, pub_key, value)
                    else:
                        ret[key] = _update({}, value)
                elif isinstance(value, list):
                    ret[key] = [(_update({}, item) if isinstance(item, collections_abc.Mapping) else item) for item in value]
                else:
                    ret[key] = update[key]

            return ret
        except:
            log = logging.getLogger(__name__)
            log.exception('Failed on ret=%r, update=%r', ret, update)
            raise

    result = {}
    _update(result, inp_dict)
    return result


def _call_urls(urls, minion_id, request_params, request_headers):
    response = None
    retries = Retry(total=5,
                    backoff_factor=1,
                    status_forcelist=[500, 502, 503, 504])
    session = Session()
    session.mount('https://', HTTPAdapter(max_retries=retries))
    try:
        for url in urls:
            response = session.get(
                url + minion_id,
                params=request_params,
                headers=request_headers,
                verify='/opt/yandex/allCAs.pem',
                timeout=3,
            ).json()
            if response:
                break
        if not response:
            time.sleep(1)
    except Exception as exc:
        log = logging.getLogger(__name__)
        log.exception(exc)
        response = {}

    return response


def ext_pillar(  # pylint: disable=too-many-arguments
        minion_id, pillar, urls, access_id, access_secret, api_pub_key,
        salt_sec_key):
    """
    Read pillar data from HTTPS response
    """
    log = logging.getLogger(__name__)

    request_headers = {
        'Accept': 'application/json',
        'Access-Id': access_id,
        'Access-Secret': access_secret,
        'Content-Type': 'application/json',
    }

    runlist = pillar.get('data', {}).get('runlist', [])
    if runlist:
        return {}

    request_params = None
    if 'target-pillar-id' in pillar:
        request_params = {'target-pillar-id': pillar['target-pillar-id']}

    if 'rev' in pillar:
        request_params = {'rev': pillar['rev']}

    for _ in range(5):
        response = _call_urls(urls, minion_id, request_params, request_headers)
        if response:
            break
    try:
        return _rec_decrypt(response, salt_sec_key, api_pub_key)
    except Exception as exc:
        log.exception(exc)
        return {}
