# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from mock import patch
import crypt  # noqa: F401

from cloud.mdb.salt.salt._modules import mdb_elasticsearch
from cloud.mdb.salt_tests.common.mocks import mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize

HTPASSWD_FILE = """mdb_admin:$6$EFUO5IkKfuS9faUR$CESeevaG5bONIupxSa1sFWRCEWsPM6kJxpj6jKo4uag45rX/oHACyQIgjnGMiKBpgqzkJc9N80NaW9getyJOe.
user1:$6$CgQblGLKpKMbrDVn$Mi9FMBR8QDB/OeMm05.lRXDjqWi3R4BpVBP31RjGwIzs2PSlHyHxh1.shWErkXZ0HDL2KtnJ04syPbILVhNm80
"""

ES_USERS_FILE = """mdb_admin:$2a$10$EFUO5IkKfuS9faUR8MXLk.01MJoL1rj4AGcK6kgGNZErBq6wfx5E6
mdb_kibana:$2a$10$CvgOX8hyAUTIlwn31i6MoeGCDRGRTRM5Nd7td0U.Xlld7acpL5Omu
mdb_monitor:$2a$10$r7o3J7h3/ad/9hqHBlapEOWSxGZsieXOYRfzAcYhKmPBuZRlOTrHO
user1:$2a$10$CgQblGLKpKMbrDVn4Lbm/O8WFYdOr4d/q.yFkZ0hsP5xrp3aA1.ti
"""

ES_USERS_ROLES_FILE = """kibana_admin:mdb_kibana
mdb_monitor:mdb_monitor
superuser:mdb_admin,user1
"""


@parametrize(
    {
        'id': 'Render htpasswd',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'users': {
                            'user1': {
                                'name': 'user1',
                                'password': 'password',
                            },
                            'mdb_admin': {
                                'name': 'mdb_admin',
                                'password': 'password',
                                'internal': True,
                            },
                        },
                    },
                },
            },
            'result': HTPASSWD_FILE,
        },
    },
)
def test_render_htpasswd(pillar, result):
    mock_pillar(mdb_elasticsearch.__salt__, pillar)
    with patch('crypt.crypt') as crypt_mock:
        crypt_mock.side_effect = [
            '$6$EFUO5IkKfuS9faUR$CESeevaG5bONIupxSa1sFWRCEWsPM6kJxpj6jKo4uag45rX/oHACyQIgjnGMiKBpgqzkJc9N80NaW9getyJOe.',
            '$6$CgQblGLKpKMbrDVn$Mi9FMBR8QDB/OeMm05.lRXDjqWi3R4BpVBP31RjGwIzs2PSlHyHxh1.shWErkXZ0HDL2KtnJ04syPbILVhNm80',
        ]
        assert mdb_elasticsearch.render_htpasswd() == result


@parametrize(
    {
        'id': 'Render ES users file',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'users': {
                            'mdb_kibana': {
                                'password': 'MKZXLo2VDK42sNt2',
                                'internal': True,
                                'name': 'mdb_kibana',
                            },
                            'mdb_admin': {
                                'password': 'I1rBz9cIhAtuq0Sm',
                                'internal': True,
                                'name': 'mdb_admin',
                            },
                            'user1': {
                                'password': 'MKZXLo2VDK42sNt2',
                                'name': 'user1',
                                'roles': [
                                    'superuser',
                                ],
                            },
                            'mdb_monitor': {
                                'password': '6rwFwknkcs7OPc8a',
                                'internal': True,
                                'name': 'mdb_monitor',
                            },
                        },
                    },
                },
            },
            'result': ES_USERS_FILE,
        },
    },
)
def test_render_users(pillar, result):
    mock_pillar(mdb_elasticsearch.__salt__, pillar)
    assert mdb_elasticsearch.render_users_file() == result


@parametrize(
    {
        'id': 'Render ES users roles file',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'users': {
                            'mdb_kibana': {
                                'password': 'MKZXLo2VDK42sNt2',
                                'internal': True,
                                'name': 'mdb_kibana',
                            },
                            'mdb_admin': {
                                'password': 'I1rBz9cIhAtuq0Sm',
                                'internal': True,
                                'name': 'mdb_admin',
                            },
                            'user1': {
                                'password': 'MKZXLo2VDK42sNt2',
                                'name': 'user1',
                                'roles': [
                                    'superuser',
                                ],
                            },
                            'mdb_monitor': {
                                'password': '6rwFwknkcs7OPc8a',
                                'internal': True,
                                'name': 'mdb_monitor',
                            },
                        },
                    },
                },
            },
            'result': ES_USERS_ROLES_FILE,
        },
    },
)
def test_render_users_roles(pillar, result):
    mock_pillar(mdb_elasticsearch.__salt__, pillar)
    assert mdb_elasticsearch.render_users_roles_file() == result
