"""
Passport mock configuration.
"""

# flake8: noqa
# pylint: skip-file

# Generated with https://github.yandex-team.ru/gist/d0uble/017a771f3de07ab88bb19b0244c5ac45
PASSPORT = {
    'oauth': {
        'tokens': {
            'testtoken': {
                'login': 'alice',
                'scope': 'mdb-deploy-api:read',
                'status': 'VALID'
            },
            'noscope': {
                'login': 'alice',
                'scope': 'not-mdb-deploy-api:read',
                'status': 'VALID'
            },
            'nowhitelist': {
                'login': 'bob',
                'scope': 'mdb-deploy-api:read',
                'status': 'VALID'
            }
        }
    },
    'sessions': {
        'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa': {
            'login': 'alice',
            'status': 'VALID'
        },
        'aaaaaaaa-aaaa-aaaa-aaaa-000000000000': {
            'login': 'alice',
            'status': 'INVALID'
        },
        'bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb': {
            'login': 'bob',
            'status': 'VALID'
        },
        'cccccccc-cccc-cccc-cccc-cccccccccccc': {
            'login': 'eva',
            'status': 'VALID'
        }
    }
}

