"""
Test for events utils
"""
import pytest

from dbaas_internal_api.utils.events import render_authentication, render_authorization


@pytest.mark.parametrize(
    ['auth_ctx', 'expected'],
    [
        [
            {
                'user_id': 'user_id1',
                'authentication': {
                    'user_type': 'user_account',
                },
            },
            {
                'authenticated': True,
                'subject_id': 'user_id1',
                'subject_type': 'YANDEX_PASSPORT_USER_ACCOUNT',
            },
        ],
        [
            {
                'user_id': 'service_user_id',
                'authentication': {
                    'user_type': 'service_account',
                },
            },
            {
                'authenticated': True,
                'subject_id': 'service_user_id',
                'subject_type': 'SERVICE_ACCOUNT',
            },
        ],
        [
            {
                'user_id': 'system-user-id',
                'authentication': {
                    'user_type': 'system',
                },
            },
            {
                'authenticated': True,
                'subject_id': 'system-user-id',
                'subject_type': 'SUBJECT_TYPE_UNSPECIFIED',
            },
        ],
    ],
)
def test_render_authentication(auth_ctx, expected):
    got = render_authentication(auth_ctx)
    # it's okay that we compare dicts,
    # cause them are not nested
    assert got == expected


def test_render_authentication_on_bad_context():
    with pytest.raises(RuntimeError):
        render_authentication({})


@pytest.mark.parametrize(
    ['auth_ctx', 'expected'],
    [
        [
            {'authorizations': [{'action': 'mdb.all.modify', 'entity_type': 'folder', 'entity_id': 'folder1'}]},
            {
                'authorized': True,
                'permissions': [
                    {
                        'permission': 'mdb.all.modify',
                        'resource_type': 'resource-manager.folder',
                        'resource_id': 'folder1',
                        'authorized': True,
                    }
                ],
            },
        ],
        [
            {
                'authorizations': [
                    {'action': 'mdb.all.modify', 'entity_type': 'cloud', 'entity_id': 'cloud1'},
                    {
                        'action': 'mdb.all.create',
                        'entity_type': 'folder',
                        'entity_id': 'folder2',
                    },
                ]
            },
            {
                'authorized': True,
                'permissions': [
                    {
                        'permission': 'mdb.all.modify',
                        'resource_type': 'resource-manager.cloud',
                        'resource_id': 'cloud1',
                        'authorized': True,
                    },
                    {
                        'permission': 'mdb.all.create',
                        'resource_type': 'resource-manager.folder',
                        'resource_id': 'folder2',
                        'authorized': True,
                    },
                ],
            },
        ],
    ],
)
def test_render_authorization(auth_ctx, expected):
    assert render_authorization(auth_ctx) == expected
