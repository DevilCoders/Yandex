"""
SqlServer backup export tests
"""
import base64

from test.mocks import checked_run_task_with_mocks
from test.tasks.sqlserver.utils import get_sqlserver_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency

test_jobs = {
    'host1-mdb_sqlserver.check_external_storage': [{'extId': 'ex123'}],
}


test_jobresults = [
    {
        # saltutils.sync_all
        'result': base64.b64encode(
            b'{'
            b'    "id": "sas-12345.yandex.net",'
            b'    "fun": "saltutil.sync_all",'
            b'    "jid": "20210820121655330399",'
            b'    "return": {'
            b'        "sdb": [],'
            b'        "utils": [],'
            b'        "clouds": []'
            b'    }'
            b'}'
        ).decode('ascii'),
        'extID': 'ex111',
        'fqdn': 'host1',
        'id': '200',
    },
    {
        'result': base64.b64encode(
            b'{'
            b'    "id": "sas-12345.yandex.net",'
            b'    "fun": "mdb_sqlserver.check_external_storage",'
            b'    "jid": "20210820121655330399",'
            b'    "retcode": 0,'
            b'    "success": true,'
            b'    "fun_args": [ "operation=backup-export", "pillar={}" ],'
            b'    "return": {'
            b'       "is_storage_ok": true'
            b'    }'
            b'}'
        ).decode('ascii'),
        'extID': 'ex123',
        'fqdn': 'host1',
        'id': '100',
    },
]


def test_compute_sqlserver_database_backup_export_interrupt_consistency(mocker):
    """
    Check compute backup export interruptions
    """
    args = {
        'hosts': {
            'host1': get_sqlserver_compute_host(geo='geo1'),
            'host2': get_sqlserver_compute_host(geo='geo2'),
            'host3': get_sqlserver_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'sqlserver_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['backup-export'] = {}
    args['target-database'] = 'test-database'
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_task_interrupt_consistency(
        mocker,
        'sqlserver_database_backup_export',
        args,
        state,
    )


def test_sqlserver_database_backup_export_mlock_usage(mocker):
    """
    Check backup export mlock usage
    """
    args = {
        'hosts': {
            'host1': get_sqlserver_compute_host(geo='geo1'),
            'host2': get_sqlserver_compute_host(geo='geo2'),
            'host3': get_sqlserver_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'sqlserver_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['backup-export'] = {}
    args['target-database'] = 'test-database'
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_mlock_usage(
        mocker,
        'sqlserver_database_backup_export',
        args,
        state,
    )
