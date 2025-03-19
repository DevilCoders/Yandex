"""
Backups utils test
"""

import pytest

from dbaas_internal_api.utils.backups import extract_name_from_meta_key

# pylint: disable=missing-docstring, invalid-name


@pytest.mark.parametrize(
    's3_key, backup_prefix, meta_file, backup_name',
    [
        ['mongodb-backup/CID/20180323T133628/meta.json', 'mongodb-backup/CID/', 'meta.json', '20180323T133628'],
        ['redis-backup/20180930T114253/meta.json', 'redis-backup/', 'meta.json', '20180930T114253'],
        ['ch_backup/CID/20180323T133628/backup_struct.json', 'ch_backup/CID/', 'backup_struct.json', '20180323T133628'],
    ],
)
def test_extract_name_from_meta_key(s3_key, backup_prefix, backup_name, meta_file):
    assert extract_name_from_meta_key(s3_key=s3_key, backup_prefix=backup_prefix, meta_file=meta_file) == backup_name
