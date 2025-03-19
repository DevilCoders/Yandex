from datetime import datetime
from queue import Queue

from cloud.mdb.dbaas_worker.internal.providers.compute import (
    ComputeApi,
    check_security_groups_changes,
)
from cloud.mdb.internal.python.compute import instances, vpc
from cloud.mdb.internal.python.compute.instances.models import (
    OneToOneNat,
    Address,
    NetworkType,
    NetworkSettings,
    Resources,
    AttachedDisk,
    DiskMode,
    DiskStatus,
)
import pytest
from test.mocks import _get_config


def test_wait_for_empty_operations_list(mocker):
    """
    Method wait_for_any_finished_operation returns None if list of operation ids is empty
    """
    operation_id = get_compute_api().wait_for_any_finished_operation([])
    assert operation_id is None


def get_compute_api() -> ComputeApi:
    config = _get_config()
    queue = Queue(maxsize=10000)
    task = {
        'cid': 'cid-test',
        'task_id': 'test_id',
        'task_type': 'test-task',
        'feature_flags': [],
        'folder_id': 'test_folder',
        'context': {},
        'timeout': 3600,
        'changes': [],
    }
    return ComputeApi(config, task, queue)


@pytest.mark.parametrize(
    'instance_sg, desired_sg, expected',
    [
        (['service-sg', 'default-sg'], ['default-sg', 'service-sg'], False),
        (['default-sg'], ['default-sg', 'service-sg'], True),
        (['service-sg'], ['default-sg', 'service-sg'], True),
        (['service-sg', 'user-sg'], ['service-sg', 'user-sg'], False),
    ],
)
def test_check_security_groups_changes(instance_sg, desired_sg, expected):
    default_sg = 'default-sg'
    assert expected == check_security_groups_changes(
        instance=instances.InstanceModel(
            id='',
            folder_id='',
            status=instances.InstanceStatus.STATUS_UNSPECIFIED,
            fqdn='',
            name='',
            service_account_id='',
            platform_id='',
            zone_id='',
            metadata={},
            labels={},
            network_settings=NetworkSettings(type=NetworkType.TYPE_UNSPECIFIED),
            resources=Resources(memory=0, core_fractions=0, cores=0, gpus=0, nvme_disks=0),
            boot_disk=AttachedDisk(
                auto_delete=False, device_name='', mode=DiskMode.UNSPECIFIED, disk_id='', status=DiskStatus.UNSPECIFIED
            ),
            secondary_disks=[],
            network_interfaces=[
                instances.NetworkInterface(
                    primary_v4_address=Address(address='', one_to_one_nat=OneToOneNat(address=''), dns_records=[]),
                    primary_v6_address=Address(address='', one_to_one_nat=OneToOneNat(address=''), dns_records=[]),
                    index=0,
                    security_group_ids=instance_sg,
                    subnet_id='',
                ),
            ],
            created_at=datetime.now(),
        ),
        network=vpc.Network(
            network_id='',
            folder_id='',
            default_security_group_id=default_sg,
        ),
        desired_security_groups=desired_sg,
    )
