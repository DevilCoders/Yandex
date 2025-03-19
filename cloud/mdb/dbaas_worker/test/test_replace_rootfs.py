"""
Replace RootFS tests
"""

import argparse
import json
import re
from copy import deepcopy

from queue import Queue

from httmock import urlmatch
from mock import Mock
from test import mocks

from .mocks import DEFAULT_STATE
from .mocks.metadb import metadb
from .mocks.utils import http_return

from cloud.mdb.dbaas_worker.bin.replace_rootfs import RootfsReplacer

HOST = 'host1'
DOM0 = 'test-dom0'

MD_FIX_CMD = r"""
if ls /dev/md* >/dev/null 2>&1;
then
    for i in /dev/md*;
    do
        mdadm --stop $i;
    done;
    echo 'CREATE owner=root group=disk mode=0660 auto=yes' > /etc/mdadm/mdadm.conf;
    echo 'HOMEHOST <system>' >> /etc/mdadm/mdadm.conf;
    echo 'MAILADDR root' >> /etc/mdadm/mdadm.conf;
    echo "$(/sbin/mdadm --examine --scan | /bin/sed 's/\/dev\/md\/0/\/dev\/md0/g')" >> /etc/mdadm/mdadm.conf;
    mdadm --assemble --scan;
    /usr/sbin/update-initramfs -u -k all;
fi
"""


def make_args(installation):
    args = argparse.Namespace(
        config=None,
        host=HOST,
        vtype=installation,
        image='postgresql',
        preserve=True,
        save=True,
        offline=True,
        tls=False,
        skip_dns=True,
        args='',
        bootstrap_cmd='bootstrap_cmd',
    )
    return args


def setup_common_mocks(mocker):
    mocker.patch('cloud.mdb.dbaas_worker.bin.replace_rootfs.Mlock')
    compute_api = mocker.patch('cloud.mdb.dbaas_worker.bin.replace_rootfs.ComputeApi')
    compute_api.return_value.get_instance.return_value = Mock(fqdn=HOST)
    compute_api.return_value.create_disk.return_value = 'operation_id', 'disk_id'


def make_http_mocks():
    shipment_log = []

    @urlmatch(netloc='deploy-v2.test', method='get', path='/v1/minions/.+')
    def get_minions(*_):
        return http_return(
            code=200, body={'autoReassign': True, 'fqdn': HOST, 'group': 'deploy-group', 'registered': True}
        )

    @urlmatch(netloc='deploy-v2.test', method='post', path='/v1/minions/.+/unregister')
    def unregister_minion(*_):
        return http_return()

    @urlmatch(netloc='deploy-v2.test', method='get', path='/v1/shipments/.+')
    def get_shipment(url, _):
        shipment_id = url.path.split('/')[-1]
        shipment = {'status': 'done', 'id': shipment_id}
        return http_return(body=shipment)

    @urlmatch(netloc='deploy-v2.test', method='post', path='/v1/shipments')
    def post_shipment(_, request):
        data = json.loads(request.body)
        fqdns = data['fqdns']
        assert len(fqdns) == 1
        fqdn = fqdns[0]
        shipment_log.append([(command['type'], fqdn, command['arguments']) for command in data['commands']])
        commands = '-'.join(x['type'] for x in data['commands'])
        shipment_id = f'{fqdn}-{commands}'
        shipment = {'status': 'done', 'id': shipment_id}
        return http_return(body=shipment)

    @urlmatch(netloc='dbm.test', method='get', path='/api/v2/containers/.+')
    def get_containers(*_):
        return http_return(
            body={
                'fqdn': HOST,
                'dom0': DOM0,
                'geo': 'test-geo',
                'cluster': 'test-cluster',
                'project': 'test-project',
                'bootstrap_cmd': 'test_bootstrap_cmd',
                'generation': 'test-generation',
            }
        )

    @urlmatch(netloc='dbm.test', method='post', path='/api/v2/containers/.+')
    def modify_containers(url, _):
        fqdn = url.path.split('/')[-1]
        shipment_id = f'{DOM0}-create-{fqdn}'
        return http_return(body={'deploy': {'deploy_id': shipment_id, 'deploy_version': 2, 'host': DOM0}})

    @urlmatch(netloc='dbm.test', method='get', path='/api/v2/volumes/.+')
    def get_volumes(*_):
        return http_return(
            body=[
                {
                    "path": "/",
                    "dom0_path": "test_dom0_path",
                }
            ]
        )

    http_mocks = [
        get_minions,
        unregister_minion,
        get_shipment,
        post_shipment,
        get_containers,
        modify_containers,
        get_volumes,
    ]
    return http_mocks, shipment_log


def get_replacer():
    return RootfsReplacer(
        mocks._get_config(),
        {
            'cid': f'rootfs-replace-{HOST}',
            'task_id': 'rootfs-replace',
            'timeout': 24 * 3600,
            'changes': [],
            'context': {},
            'feature_flags': [],
            'folder_id': 'test',
        },
        Queue(maxsize=10**6),
    )


def test_replace_rootfs_compute_linux(mocker):
    """
    Test replace rootfs for linux host in compute installation
    """

    args = make_args(installation='compute')
    setup_common_mocks(mocker)
    state = deepcopy(DEFAULT_STATE)
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'result': [{'fqdn': HOST, 'roles': ['postgresql_cluster']}],
        }
    )
    metadb(mocker, state)

    http_mocks, shipment_log = make_http_mocks()

    with mocks.HTTMock(*http_mocks, mocks.default_http_mock):
        replacer = get_replacer()
        replacer.replace_rootfs(args)

    assert shipment_log == [
        [('cmd.run', HOST, ['echo ",+," | sfdisk --force -N1 /dev/vda || /bin/true'])],
        [('cmd.run', HOST, ['partprobe /dev/vda'])],
        [('cmd.run', HOST, ['resize2fs /dev/vda1'])],
        [('cmd.run', HOST, [MD_FIX_CMD])],
        [('saltutil.sync_all', HOST, [])],
        [('state.highstate', HOST, ['queue=True'])],
    ]


def test_replace_rootfs_porto(mocker):
    """
    Test replace rootfs for host in porto installation
    """

    args = make_args(installation='porto')
    setup_common_mocks(mocker)

    http_mocks, shipment_log = make_http_mocks()

    with mocks.HTTMock(*http_mocks, mocks.default_http_mock):
        replacer = get_replacer()
        replacer.replace_rootfs(args)

    mv_argument = shipment_log[1][0][2][0]
    timestamp = re.search(r'\d{10}', mv_argument)[0]

    assert shipment_log == [
        [('cmd.run', DOM0, [f'portoctl destroy {HOST} || /bin/true'])],
        [('cmd.run', DOM0, [f'mv test_dom0_path test_dom0_path.{timestamp}.bak || /bin/true'])],
        [('saltutil.sync_all', HOST, [])],
        [('state.highstate', HOST, ['queue=True'])],
    ]
