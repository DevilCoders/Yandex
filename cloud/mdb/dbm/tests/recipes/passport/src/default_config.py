"""
Passport mock configuration.
"""

# flake8: noqa
# pylint: skip-file

# Generated with https://github.yandex-team.ru/gist/d0uble/017a771f3de07ab88bb19b0244c5ac45
PASSPORT = {
    'oauth': {
        'tokens': {
            '11111111-1111-1111-1111-111111111111': {'login': 'alice', 'scope': 'pgaas:all', 'status': 'VALID'},
            '11111111-1111-1111-1111-000000000000': {'login': 'alice', 'scope': 'pgaas:all', 'status': 'DISABLED'},
            '11111111-0000-0000-0000-111111111111': {'login': 'alice', 'scope': 'pgaas:all', 'status': 'INVALID'},
            '22222222-2222-2222-2222-222222222222': {'login': 'bob', 'scope': 'pgaas:all', 'status': 'VALID'},
            '33333333-3333-3333-3333-333333333333': {'login': 'eva', 'scope': 'pgaas:all', 'status': 'VALID'},
        }
    },
    'sessions': {
        'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa': {'login': 'alice', 'status': 'VALID'},
        'aaaaaaaa-aaaa-aaaa-aaaa-000000000000': {'login': 'alice', 'status': 'INVALID'},
        'bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb': {'login': 'bob', 'status': 'VALID'},
        'cccccccc-cccc-cccc-cccc-cccccccccccc': {'login': 'eva', 'status': 'VALID'},
    },
}
