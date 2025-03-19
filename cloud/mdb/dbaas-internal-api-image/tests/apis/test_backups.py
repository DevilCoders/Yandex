"""
Backups utils test
"""

from datetime import timedelta
from typing import List

import pytest

from dbaas_internal_api.apis.backups import BackupProxy, MalformedGlobalBackupId
from dbaas_internal_api.apis.backups import get_backups_page
from dbaas_internal_api.utils.time import now
from dbaas_internal_api.utils.types import Backup
from dbaas_internal_api.utils.backup_id import decode_global_backup_id, encode_global_backup_id

# pylint: disable=missing-docstring, invalid-name


def mk_backup(backup_id: str, start_time=now(), end_time=now()) -> Backup:
    return Backup(id=backup_id, start_time=start_time, end_time=end_time)


def backups(*backup_ids: str) -> List[Backup]:
    return [mk_backup(c) for c in backup_ids]


class Test_get_backups_page:
    def test_return_empty_page_for_empty_backups(self):
        assert get_backups_page([], 100, None) == []

    def test_return_all_items_when_all_fits_one_page(self):
        assert get_backups_page(backups('1', '2', '3'), 100, None) == backups('1', '2', '3')

    def test_return_first_page(self):
        assert get_backups_page(backups('1', '2', '3'), 2, None) == backups('1', '2')

    def test_return_second_page(self):
        assert get_backups_page(backups('1', '2', '3', '4', '5'), limit=2, page_token_id='2') == backups('3', '4')


def test_encoded_global_backup_id_can_be_decode():
    encoded = encode_global_backup_id('Cluster-Id', 'backup-id')
    assert decode_global_backup_id(encoded) == ('Cluster-Id', 'backup-id')


BAD_GLOBAL_BACKUP_ID = 'xxx'


def test_decode_rase_for_strange_global_id():
    with pytest.raises(MalformedGlobalBackupId):
        decode_global_backup_id(BAD_GLOBAL_BACKUP_ID)


class TestBackupProxy:
    def test_return_global_id_instead_of_local_backup_id(self):
        pr = BackupProxy(mk_backup('42'), 'folder-id', 'cid')
        assert pr.id, 'Not empty backup id'
        assert pr.id != 'Backup id should not equals to local backup.id'

    def test_proxy_backup_attributes(self):
        class WebScaleBackup(Backup):
            web_scale = 'Not really...'

        back = WebScaleBackup(
            id='42',
            start_time=now(),
            end_time=now() + timedelta(days=1),
        )
        pr = BackupProxy(back, 'folder-id', 'cid')
        assert pr.start_time == back.start_time
        assert pr.end_time == back.end_time
        assert pr.web_scale == 'Not really...'

    def test_sort_key(self):
        back = mk_backup('42', start_time=now(), end_time=now() + timedelta(days=1))
        pr = BackupProxy(back, 'folder-id', 'cid')
        assert BackupProxy.sort_key(pr) == (back.end_time, 'cid')
