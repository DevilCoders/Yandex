"""
MySQL cluster upgrade 80 tests
"""

import base64
from test.mocks import checked_run_task_with_mocks
from test.tasks.mysql.utils import get_mysql_compute_host, get_mysql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_rejected


"""
example of shipment:
{
    'token': '12',
    'jobs': [
        {
            'commandID': '12',
            'createdAt': 1629461815,
            'extId': '20210820121655330363',
            'id': '12',
            'status': 'done',
            'updatedAt': 1629461820
        }
    ]
}
"""
test_jobs = {'host1-mdb_mysql.check_upgrade': [{'extId': 'ex123'}]}


"""
You can check your function output:
 mdb-admin -d porto-test deploy shipments create dev-test \
     --cmd=5m,mdb_mysql.get_db_variables,'connection_default_file="/root/.my.cnf"' \
     --fqdns myt-zhzo16rea9dghr9k.db.yandex.net

 mdb-admin -d porto-test deploy jobresults list --jobid 20220204131036072012 --asis
"""
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
        'extID': 'ex200',
        'fqdn': 'host1',
        'id': '200',
    },
    {
        'result': base64.b64encode(
            b'{'
            b'    "id": "sas-12345.yandex.net",'
            b'    "fun": "mdb_mysql.check_upgrade",'
            b'    "jid": "20210820121655330399",'
            b'    "retcode": 0,'
            b'    "success": true,'
            b'    "fun_args": [ "target-versio=\\\"8.0.0\\\"" ],'
            b'    "return": {'
            b'       "is_upgrade_safe": false,'
            b'       "comment": "Upgrade Prohibited"'
            b'    }'
            b'}'
        ).decode('ascii'),
        'extID': 'ex100',
        'fqdn': 'host1',
        'id': '100',
    },
    {
        'result': base64.b64encode(
            b'{'
            b'    "id": "sas-12345.yandex.net",'
            b'    "fun": "mdb_mysql.check_upgrade",'
            b'    "jid": "20210820121655330399",'
            b'    "retcode": 0,'
            b'    "success": true,'
            b'    "fun_args": [ "target-versio=\\\"8.0.0\\\"" ],'
            b'    "return": {'
            b'       "is_upgrade_safe": true,'
            b'       "comment": ""'
            b'    }'
            b'}'
        ).decode('ascii'),
        'extID': 'ex123',
        'fqdn': 'host1',
        'id': '123',
    },
]


def test_mysql_cluster_upgrade_80_rejected_when_unhealthy(mocker):
    """
    Check upgrade 80 rejected when cluster is unhealthy
    """
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
            'host2': get_mysql_porto_host(geo='geo2'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    state['zookeeper']['mysql'] = {'cid-test': {'master': '"host2"', 'maintenance': {'mysync_paused': True}}}
    state['juggler']['host1']['status'] = 'CRIT'
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_rejected(
        mocker,
        'mysql_cluster_upgrade_80',
        args,
        state,
    )


def test_porto_mysql_cluster_upgrade_80_interrupt_consistency(mocker):
    """
    Check porto upgrade 80 interruptions
    """
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
            'host2': get_mysql_porto_host(geo='geo2'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    state['zookeeper']['mysql'] = {'cid-test': {'master': '"host2"', 'maintenance': {'mysync_paused': True}}}
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_upgrade_80',
        args,
        state,
    )


def test_compute_mysql_cluster_upgrade_80_interrupt_consistency(mocker):
    """
    Check compute upgrade 80 interruptions
    """
    args = {
        'hosts': {
            'host1': get_mysql_compute_host(geo='geo1'),
            'host2': get_mysql_compute_host(geo='geo2'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    state['zookeeper']['mysql'] = {'cid-test': {'master': '"host2"', 'maintenance': {'mysync_paused': True}}}
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_upgrade_80',
        args,
        state,
    )


def test_mysql_cluster_upgrade_80_mlock_usage(mocker):
    """
    Check upgrade 80 mlock usage
    """
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
            'host2': get_mysql_porto_host(geo='geo2'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    state['zookeeper']['mysql'] = {'cid-test': {'master': '"host2"', 'maintenance': {'mysync_paused': True}}}
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_mlock_usage(
        mocker,
        'mysql_cluster_upgrade_80',
        args,
        state,
    )
