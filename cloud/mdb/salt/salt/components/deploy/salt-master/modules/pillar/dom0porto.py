# -*- coding: utf-8 -*-
"""
A module that adds data to the Pillar structure retrieved by an http request to DB maintenance API


Configuring the dom0porto ext_pillar
=======================================

Set the following Salt config to setup dom0porto as external pillar source:

.. code-block:: yaml

  ext_pillar:
    - dom0porto:
        url: https://host/containers

"""
from __future__ import absolute_import

import fnmatch
import logging

from requests import Session
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry


def _call_dbm(minion_id, pillar, url, include_only, token):
    log = logging.getLogger(__name__)
    if include_only:
        match = False

        for i in include_only:
            if fnmatch.fnmatch(minion_id, i):
                match = True
                break
    else:
        match = True

    if not match:
        return

    try:
        retries = Retry(total=5,
                        backoff_factor=1,
                        status_forcelist=[500, 502, 503, 504])
        session = Session()
        session.mount('https://', HTTPAdapter(max_retries=retries))
        call_res = session.get(
            url + minion_id,
            headers={'Authorization': 'OAuth ' + token},
            timeout=1,
            verify='/opt/yandex/allCAs.pem')
        call_res.raise_for_status()
        ret = call_res.json()
        return ret
    except Exception as e:
        log.error(e)
        return {}


def ext_pillar(minion_id,
               pillar,
               configurations):
    """
    Read pillar data from HTTP response.
    """
    for configuration in configurations:
        data = _call_dbm(minion_id, pillar, **configuration)
        if data is not None:
            return data
    return {}
