# coding: utf-8
from __future__ import unicode_literals

from tvm2.protocol import BlackboxClientId
import yenv


IDM_INSTANCE = yenv.choose_key_by_type({
    'testing': 'testing',
    'production': 'production',
}, fallback=True)
IDM_URL_PREFIX = 'idm/'

IDM_TVM_CLIENT_ID = yenv.choose_key_by_type({
    'testing': 2001602,
    'production': 2001600,
}, fallback=True)

IDM_API_TVM_DEFAULTS = {
    'allowed_clients': (IDM_TVM_CLIENT_ID,),
    'blackbox_client': BlackboxClientId.ProdYateam
}

IDM_ENVIRONMENT_TYPE = 'intranet'
