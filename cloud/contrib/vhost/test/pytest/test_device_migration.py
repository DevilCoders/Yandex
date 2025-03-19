from itertools import permutations
from time import sleep

import pytest
from pytest_lazyfixture import lazy_fixture

from qemu import *
from common import *
from utils import Fio


@pytest.fixture(scope="function")
def vm_for_migration_test(pytestconfig, request, boot_disk, tmp_dir):
    """
    This fixture represents VM with boot disk, VM is not started,
    test disks should be attached explicitly
    :param tmp_dir: fixture for temporary data storage
    :param boot_disk: fixture for boot disk
    """
    vm = VM(binary=get_qemu_bin(), work_dir=tmp_dir, ssh_key=get_ssh_key())
    vm.disk_add(boot_disk)
    yield vm

    if pytestconfig.getoption('--keep-on-failure') and (
            request.node.rep_setup.failed or request.node.rep_call.failed):
        # FIXME: print VM parameters for debugging (folders, ssh port, etc.)
        print("test failed, keeping VM and disk backend as is")
        return

    vm.shutdown()


# noinspection PyPep8Naming
@pytest.fixture(scope="module")
def YC_Vhost_User_Virtio(scratch_image, tmp_dir):
    yield VhostUserVirtioDisk(
        YCVhostUserDiskBackend(binary=get_aio_server_binary(),
                               disk_id="test_vhost",
                               image=scratch_image,
                               work_dir=tmp_dir))


# noinspection PyPep8Naming
@pytest.fixture(scope='module')
def VirtioBlk(scratch_image):
    yield VirtioDisk(
        FileBackend(disk_id="test_vhost",
                    image=scratch_image,
                    image_type="raw"), bootable=False, with_iothread=True)


def disk_pairs_gen():
    disk_list = [lazy_fixture('YC_Vhost_User_Virtio'),
                 lazy_fixture('VirtioBlk')]
    return permutations(disk_list, 2)


@pytest.mark.parametrize('disk_src, disk_dst', disk_pairs_gen())
@pytest.mark.full
@pytest.mark.virtio_blk
@pytest.mark.vhost_user_virtio
def test_migration_across_device_type(vm_for_migration_test, disk_src,
                                      disk_dst):
    vm = vm_for_migration_test
    if not vm.supports_device(disk_src.device_type):
        pytest.skip("unsupported src disk type {}".format(disk_src.device_type))
    if not vm.supports_device(disk_dst.device_type):
        pytest.skip("unsupported dst disk type {}".format(disk_dst.device_type))
    disk_src.backend.start()
    vm.disk_add(disk_src)
    vm.start()
    with Fio.start(vm, disk_src):
        sleep(5)
        vm.save_to_file()
        disk_src.backend.stop(hard=False)

        # TODO: this disk replace is quite hacky, find a better way
        vm._disk_list.remove(disk_src)
        disk_dst.backend.start()
        vm.disk_add(disk_dst)
        vm.load_from_file()
        sleep(5)
    disk_dst.backend.stop(hard=False)
