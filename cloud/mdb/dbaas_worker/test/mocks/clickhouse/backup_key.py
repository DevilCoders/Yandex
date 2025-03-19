"""
Simple ch_backup_key mock
"""


def ch_backup_key(mocker, _):
    """
    Setup ch_backup_key mock
    """
    random = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.clickhouse.backup_key.random.SystemRandom')
    random.return_value.choice.return_value = '0'
