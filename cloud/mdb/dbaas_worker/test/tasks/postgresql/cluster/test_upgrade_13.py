"""
PostgreSQL cluster upgrade 13 tests
"""

import base64
from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


test_jobs = {
    'host3-mdb_postgresql.generate_ssh_keys': [{'extId': 'ext1'}],
    'host3-mdb_postgresql.upgrade': [{'extId': 'ext2'}],
}
test_jobresults = [
    {
        'result': base64.b64encode(
            b'{'
            b'      "id": "iva-kygmp6g6j22u84qq.db.yandex.net", '
            b'      "fun": "mdb_postgresql.generate_ssh_keys", '
            b'      "jid": "20220216125615846291", '
            b'      "return": "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDQCD/BDRgozwLfwEq9dY8dDZ/T4P84VtxwInmjT8T6dkeZ+Mx9'
            b'                 nPHeTxvxk4qa8hKVQCBXFm8hIhiTGNb9g3vjZDsxvo8ngRvwyo67SgWSzGF6UmmYUXttELvx2JC/86N8Qncx0zT1'
            b'                 HA/3fLpoxUMFLb0Fvxsc+1xw/jUycrU6zsxheVQhCT4UelPmsdZQzzJ7f5ajbtvr5MNhFLnlg6EuG/2LrSb7GMXd'
            b'                 EfO3vTMAZnUm29kcdQ+7c4K8+2nqdRn/1Ok6zhOZdna1XOmvxzVZtY0WOK1saUJ04SEieGe/Sh2o6GhBcqP4xJx8'
            b'                 eJXfICqrQwNtgsbjSkRxSZaPmH8Z iva-kygmp6g6j22u84qq.db.yandex.net", '
            b'      "retcode": 0, '
            b'      "success": true, '
            b'      "fun_args": []'
            b'}'
        ).decode('ascii'),
        'extID': 'ext1',
        'fqdn': 'host3',
        'id': '1',
    },
    {
        'result': base64.b64encode(
            b'{'
            b'      "id": "iva-kygmp6g6j22u84qq.db.yandex.net", '
            b'      "fun": "mdb_postgresql.upgrade", '
            b'      "jid": "20220216125643467466", '
            b'      "return": {'
            b'          "result": true, '
            b'          "stderr": "", '
            b'          "stdout": "", '
            b'          "is_upgraded": true, '
            b'          "is_rolled_back": false, '
            b'          "user_exposed_error": ""'
            b'      }, '
            b'      "retcode": 0, '
            b'      "success": true, '
            b'      "fun_args": ["11", "cluster_node_addrs={}"]'
            b'}'
        ).decode('ascii'),
        'extID': 'ext2',
        'fqdn': 'host3',
        'id': '2',
    },
]


def test_porto_postgresql_cluster_upgrade_13_interrupt_consistency(mocker):
    """
    Check porto upgrade 13 interruptions
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

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_upgrade_13',
        args,
        state,
    )


def test_compute_postgresql_cluster_upgrade_13_interrupt_consistency(mocker):
    """
    Check compute upgrade 13 interruptions
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

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_upgrade_13',
        args,
        state,
    )


def test_postgresql_cluster_upgrade_13_mlock_usage(mocker):
    """
    Check upgrade 13 mlock usage
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

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_mlock_usage(
        mocker,
        'postgresql_cluster_upgrade_13',
        args,
        state,
    )
