# -*- coding: utf-8 -*-
"""
DBaaS Internal API config auth check
"""

from abc import ABC, abstractmethod
from typing import Sequence, Union

from argon2 import PasswordHasher
from argon2.exceptions import VerificationError
from flask import current_app, g
from flask_restful import abort


class ConfigAuthProvider(ABC):
    """
    Abstract config auth provider
    """

    @abstractmethod
    def auth(self, access_id, access_secret, config_types: Sequence[str]):
        """
        Check auth
        """


class MetadbConfigAuthProvider(ConfigAuthProvider):
    """
    MetaDB-based config auth provider
    """

    def auth(self, access_id, access_secret, config_types: Sequence[str]):
        """
        Check if config host has correct id/secret
        """
        auth_res = g.metadb.query('config_host_auth', access_id=str(access_id))
        if not auth_res:
            abort(403)
        record = auth_res[0]
        if record['type'] not in config_types:
            abort(403)
        try:
            hasher = PasswordHasher()
            hasher.verify(record['access_secret'], access_secret)
        except VerificationError:
            abort(403)


def check_auth(access_id, access_secret, config_types: Union[str, Sequence[str]]):
    """
    Check config auth
    """
    provider = current_app.config['CONFIG_AUTH_PROVIDER']()
    config_types = config_types if type(config_types) in (list, tuple) else (config_types)
    provider.auth(access_id, access_secret, config_types)
