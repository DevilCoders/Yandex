from pytest_mock import MockerFixture
from queue import Queue
from test.mocks import _get_config

from cloud.mdb.dbaas_worker.internal.providers.metadb_security_group import MetaDBSecurityGroupInfo
from cloud.mdb.dbaas_worker.internal.providers.vpc import VPCProvider


def _get_task() -> dict:
    return {
        'cid': 'cid-test',
        'task_id': 'test_id',
        'task_type': 'test-task',
        'feature_flags': [],
        'folder_id': 'test_folder',
        'context': {},
        'timeout': 3600,
        'changes': [],
    }


def _get_vpc_provider() -> VPCProvider:
    config = _get_config()
    queue = Queue(maxsize=10000)
    return VPCProvider(config, _get_task(), queue)


def test_delete_service_security_group(mocker: MockerFixture):
    metadb = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.vpc.MetadbSecurityGroup').return_value
    test_cid = 'test-cid'
    test_serv_sg = 'test-service-sg'

    def get_sgroup_info(cid: str) -> MetaDBSecurityGroupInfo:
        if cid != test_cid:
            raise NotImplementedError
        return MetaDBSecurityGroupInfo(
            network_id='test-net-id',
            service_sg=test_serv_sg,
            user_sgs=['test-user-sg1'],
            service_sg_hash=0,
            service_sg_allow_all=False,
        )

    metadb.get_sgroup_info.side_effect = get_sgroup_info

    vpc_api = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.vpc.vpc.VPC').return_value

    def delete_s_g(sg_id: str):
        if sg_id != test_serv_sg:
            raise NotImplementedError
        return None

    vpc_api.delete_security_group.side_effect = delete_s_g

    vpc = _get_vpc_provider()
    spy = mocker.spy(vpc, 'delete_security_group')
    vpc.delete_service_security_group('test-cid')
    assert spy.call_count == 1
