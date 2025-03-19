from __future__ import unicode_literals

from cloud.mdb.salt_tests.common.mocks import mock_version_cmp
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': '22.2.3.5 > 22.2.2.1',
        'args': {
            'version1': '22.2.3.5',
            'version2': '22.2.2.1',
            'result': 1,
        },
    },
    {
        'id': '22.2.3.5 > 22.2',
        'args': {
            'version1': '22.2.3.5',
            'version2': '22.2',
            'result': 1,
        },
    },
    {
        'id': '22.2.2.1 < 22.2.3.5',
        'args': {
            'version1': '22.2.2.1',
            'version2': '22.2.3.5',
            'result': -1,
        },
    },
    {
        'id': '22.2.3.5 = 22.2.3.5',
        'args': {
            'version1': '22.2.3.5',
            'version2': '22.2.3.5',
            'result': 0,
        },
    },
)
def test_initial_backup_command(version1, version2, result):
    salt = {}
    mock_version_cmp(salt)
    assert salt['pkg.version_cmp'](version1, version2) == result
