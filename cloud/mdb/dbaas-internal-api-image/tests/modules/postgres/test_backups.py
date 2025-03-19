# -*- coding: utf-8 -*-
"""
Postgre backup utilities tests
"""

from dbaas_internal_api.modules.postgres.backups import calculate_backup_prefix

# pylint: disable=missing-docstring


def test_calculate_backup_prefix():
    assert calculate_backup_prefix('CLUSTER-ID', '906') == 'wal-e/CLUSTER-ID/906/basebackups_005/'
