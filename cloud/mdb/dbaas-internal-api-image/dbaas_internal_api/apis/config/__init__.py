# -*- coding: utf-8 -*-
"""
DBaaS Internal API minion config
"""

import marshmallow
from flask.views import MethodView
from webargs.flaskparser import use_kwargs

from .. import API
from ..config_auth import check_auth
from ..schemas.fields import Str
from .gen_config import gen_config, gen_config_unmanaged


@API.resource('/api/v1.0/config/<string:fqdn>')
class ConfigV1(MethodView):
    """
    Minion config (used as ext_pillar)
    """

    @use_kwargs(
        {
            'access_id': marshmallow.fields.UUID(required=True, location='headers', load_from='Access-Id'),
            'access_secret': Str(required=True, location='headers', load_from='Access-Secret'),
            'target_pillar_id': Str(missing=None, location='query', load_from='target-pillar-id'),
            'rev': Str(missing=None, location='query', load_from='rev'),
        }
    )
    def get(self, fqdn, access_id, access_secret, target_pillar_id, rev):
        """
        Check config host auth and return config
        """
        check_auth(access_id, access_secret, 'default')
        return gen_config(fqdn, target_pillar_id, rev)


@API.resource('/api/v1.0/config_unmanaged/<string:fqdn>')
class ConfigUnmanagedV1(MethodView):
    """
    Host config for worker usage
    """

    @use_kwargs(
        {
            'access_id': marshmallow.fields.UUID(required=True, location='headers', load_from='Access-Id'),
            'access_secret': Str(required=True, location='headers', load_from='Access-Secret'),
            'target_pillar_id': Str(missing=None, location='query', load_from='target-pillar-id'),
            'rev': Str(missing=None, location='query', load_from='rev'),
        }
    )
    def get(self, fqdn, access_id, access_secret, target_pillar_id, rev):
        """
        Check config host auth and return config
        """
        check_auth(access_id, access_secret, 'dbaas-worker')
        return gen_config_unmanaged(fqdn, target_pillar_id, rev)
