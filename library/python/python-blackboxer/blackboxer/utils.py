# encoding: utf-8
from __future__ import unicode_literals

import socket

import requests
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util import Retry


def get_my_ip(fallback='127.0.0.1'):
    """
    Возвращаем ip-адрес сервера или fallback, если не получилось определить.
    :param fallback:
    :return:
    """
    ips = [ip for ip in socket.gethostbyname_ex(socket.gethostname())[2] if not ip.startswith("127.")]

    if ips:
        return ips[0]
    else:
        return fallback


def requests_retry_session(
        retries=3,
        backoff_factor=0.3,
        status_forcelist=(500, 502, 504),
        session=None,
):
    session = session or requests.Session()
    retry = Retry(
        total=retries,
        read=retries,
        connect=retries,
        backoff_factor=backoff_factor,
        status_forcelist=status_forcelist,
    )
    adapter = HTTPAdapter(max_retries=retry)
    session.mount('http://', adapter)
    session.mount('https://', adapter)
    return session


def choose_first_not_none(iterable):
    if isinstance(iterable, dict):
        for key, value in iterable.items():
            if value:
                return {key: value}
    else:
        for item in iterable:
            if item:
                return item
