"""
PostgreSQL database create tests
"""

import base64
from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency

test_jobs = {'host3-state.sls': [{'extId': 'ex123'}]}
test_jobresults = [
    {
        'result': base64.b64encode(
            b'{'
            b'  "fun": "state.sls",'
            b'  "jid": "20210820121655330365",'
            b'  "return": { '
            b'      "sync-extensions-123": {'
            b'          "__id__": "sync-extensions",'
            b'          "changes": {'
            b'              "test-db": {'
            b'                  "pg_repack": "drop extension",'
            b'                  "rum": "add extension"'
            b'              }'
            b'          }'
            b'      }'
            b'  }'
            b'}'
        ).decode('ascii'),
        'extID': 'ex123',
        'fqdn': 'host3',
        'id': '123',
    },
]


def test_porto_postgresql_database_create_interrupt_consistency(mocker):
    """
    Check porto database create interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target-database'] = 'test-database'
    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_task_interrupt_consistency(
        mocker,
        'postgresql_database_create',
        args,
        state,
    )


def test_compute_postgresql_database_create_interrupt_consistency(mocker):
    """
    Check compute database create interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_compute_host(geo='geo1'),
            'host2': get_postgresql_compute_host(geo='geo2'),
            'host3': get_postgresql_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target-database'] = 'test-database'
    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_task_interrupt_consistency(
        mocker,
        'postgresql_database_create',
        args,
        state,
    )


def test_postgresql_database_create_mlock_usage(mocker):
    """
    Check database create mlock usage
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target-database'] = 'test-database'
    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_mlock_usage(
        mocker,
        'postgresql_database_create',
        args,
        state,
    )
