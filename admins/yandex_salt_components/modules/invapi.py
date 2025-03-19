#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
InvAPI module, provides minimal interface to RackTables Inventory API
"""

import json
import logging
import os
from typing import List, Dict

import requests

__author__ = "kglushen"

log = logging.getLogger(__name__)

# Define the module's virtual name
__virtualname__ = "invapi"


def __virtual__():
    return __virtualname__


def _request(_token: str, filter_type: str, filter_string: str, fields: List[str]) -> List[Dict]:
    """
    Query InvAPI
    """
    query_tmpl = """
        query myQuery{
            object (%s:"%s") {
                %s
            }
        }
        """
    headers = {"Authorization": "OAuth " + _token}
    request_json = {"query": query_tmpl % (filter_type, filter_string, "\n".join(fields))}
    ro_url = "https://ro.racktables.yandex-team.ru/api"
    with requests.Session() as s:
        resp = s.post(url=ro_url, headers=headers, json=request_json)
    return json.loads(resp.text)["data"]["object"]


def raw_request(filter_type: str, filter_string: str, fields: List[str]):
    """
    Wrapper around query to InvAPI
    """
    filter_string = filter_string.strip()
    filter_type = filter_type.strip()
    # TODO: Provide some basic input checks
    import salt.syspaths
    token_file = os.path.join(salt.syspaths.CONFIG_DIR, "minion.d", "salt_invapi.token")

    if os.path.exists(token_file):
        with open(token_file) as config:
            invapi_token = config.read().rstrip("\n")
    else:
        import salt.config

        __opts__ = salt.config.minion_config(os.path.join(salt.syspaths.CONFIG_DIR, "minion"))
        __opts__['skip_grains'] = True

        caller = salt.client.Caller(mopts=__opts__)
        invapi_token = caller.cmd('pillar.get', 'invapi-token')
    if not invapi_token:
        log.error("No InvAPI token provided either in file or in pillars.")
        return []

    return _request(invapi_token, filter_type, filter_string, fields)
