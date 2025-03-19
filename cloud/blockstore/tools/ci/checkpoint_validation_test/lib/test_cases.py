from dataclasses import dataclass

from .errors import Error

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.ycp import Ycp

_FILE_NAME = '/tmp/load-config.json'
_VM_NAME = 'eternal-640gb-verify-checkpoint-test-vm'
_DISK_NAME = 'eternal-640gb-verify-checkpoint-test-disk'


@dataclass
class TestConfig:
    instance: Ycp.Instance
    disk: Ycp.Disk
    checkpoint_id: str
    config_json: str


def find_instance(ycp: Ycp, args):
    instances = ycp.list_instances()
    if args.dry_run:
        return instances[0]
    for instance in instances:
        if instance.name == _VM_NAME:
            return instance


def find_disk(ycp: Ycp, args):
    disks = ycp.list_disks()
    if args.dry_run:
        return disks[0]
    for disk in disks:
        if disk.name == _DISK_NAME:
            return disk


def get_test_config(ycp: Ycp, helpers: common.Helpers, args, logger) -> TestConfig:
    logger.info('Generating test config')
    instance = find_instance(ycp, args)
    if instance is None:
        raise Error(f'No instance with name=<{_VM_NAME}> found')

    disk = find_disk(ycp, args)
    if disk is None:
        raise Error(f'No disk with name=<{_DISK_NAME}> found')

    with common.make_sftp_client(args.dry_run, instance.ip) as sftp:
        file = sftp.file(_FILE_NAME)
        cfg = "".join(file.readlines())

    return TestConfig(
        instance=instance,
        disk=disk,
        checkpoint_id=f'tmp-checkpoint-{helpers.generate_id()}',
        config_json=cfg)
