from time import sleep

import pytest
from pytest_lazyfixture import lazy_fixture

from qemu import *
from common import *
from utils import Fio


# noinspection PyPep8Naming
@pytest.fixture(scope='module')
def YC_Vhost_User(scratch_image, tmp_dir):
    yield VhostUserDisk(YCVhostUserDiskBackend(binary=get_aio_server_binary(),
                                               disk_id="vhost",
                                               image=scratch_image,
                                               work_dir=tmp_dir))


# noinspection PyPep8Naming
@pytest.fixture(scope='module')
def YC_Vhost_User_Virtio(scratch_image, tmp_dir):
    yield VhostUserVirtioDisk(
        YCVhostUserDiskBackend(binary=get_aio_server_binary(),
                               disk_id="vhost_virtio",
                               image=scratch_image,
                               work_dir=tmp_dir))


# noinspection PyPep8Naming
@pytest.fixture(scope='module')
def YC_Vhost_User_Slow(scratch_image, tmp_dir):
    yield VhostUserDisk(
        YCVhostUserSlowDiskBackend(binary=get_aio_server_binary(),
                                   disk_id="vhost_slow",
                                   image=scratch_image,
                                   work_dir=tmp_dir))


# noinspection PyPep8Naming
@pytest.fixture(scope='module')
def VirtioBlk(scratch_image, tmp_dir):
    yield VirtioDisk(
        FileBackend(disk_id="virtio_blk",
                    image=scratch_image,
                    image_type="raw"),
        bootable=False,
        with_iothread=True)


# noinspection PyPep8Naming
@pytest.fixture(scope='module')
def YC_VirtioFS(scratch_image, tmp_dir):
    yield VirtioFS(VirtioFSBackend(binary=get_virtiofs_server_binary(),
                                   disk_id="virtiofs",
                                   image=scratch_image,
                                   work_dir=tmp_dir))


class SimpleVM(VM):
    def __init__(self, binary, work_dir, ssh_key, boot_disk, test_disk, os_type=None):
        super().__init__(binary, work_dir, ssh_key, os_type=os_type)
        if test_disk is not None and not self.supports_device(test_disk.device_type):
            pytest.skip('unsupported device {}'.format(test_disk.device_type))
        self.test_disk = test_disk
        # It's possible that vm can't start in test for
        # some reasons (for example due to unfixed bugs)
        # but vm is constructed on setup phase of pytest fixture so
        # it won't be cleaned up automatically on fixture teardown code.
        # So handle this case manually and cleanup vm on exception.
        try:
            self.disk_add(boot_disk)
            if self.test_disk is not None:
                self.test_disk.backend.start()
                self.disk_add(self.test_disk)
            self.start()
        except Exception:
            if self.test_disk is not None:
                if self.test_disk.backend.proc is not None:
                    self.test_disk.backend.stop(hard=True)
            if self.proc is not None:
                self.kill(hard=True)
            raise


def gen_disks():
    return [
        pytest.param(lazy_fixture("YC_Vhost_User"),
                     marks=[pytest.mark.full, pytest.mark.vhost_user]),
        pytest.param(lazy_fixture('YC_Vhost_User_Slow'),
                     marks=[pytest.mark.vhost_user_slow]),
        pytest.param(lazy_fixture('YC_Vhost_User_Virtio'),
                     marks=[pytest.mark.full, pytest.mark.vhost_user_virtio]),
        # this one doesn't test vhost device but can be useful for
        # testcase validation
        pytest.param(lazy_fixture('VirtioBlk'),
                     marks=[pytest.mark.full, pytest.mark.virtio_blk]),
        pytest.param(lazy_fixture('YC_VirtioFS'),
                     marks=[pytest.mark.vhost_fs]),
    ]


@pytest.fixture(scope="class", params=gen_disks())
def vm(pytestconfig, request, boot_disk, tmp_dir):
    disk = request.param
    vm = SimpleVM(binary=get_qemu_bin(), work_dir=tmp_dir,
                  ssh_key=get_ssh_key(), boot_disk=boot_disk, test_disk=disk)
    yield vm

    if pytestconfig.getoption('--keep-on-failure') and (
            request.node.rep_setup.failed or request.node.rep_call.failed):
        # FIXME: print VM parameters for debugging (folders, ssh port, etc.)
        print("test failed, keeping VM and disk backend as is")
        return
    vm.shutdown()
    disk.backend.stop(hard=False)


@pytest.fixture(scope="function")
def _checker(request, vm):
    def check():
        vm.check()
        vm.test_disk.backend.check()

    request.addfinalizer(check)


@pytest.mark.usefixtures('_checker')
class TestCommon:
    def test_boot(self, vm):
        pass

    def test_vm_operation(self, vm):
        with Fio.start(vm, vm.test_disk):
            sleep(10)

    def write_to_ro_test_disk(self, vm):
        status = vm.guest_exec(["dd", "if=/dev/zero",
                                "of={}".format(vm.test_disk.get_guest_path()),
                                "bs=4096", "count=1", "oflag=direct"])
        assert status.returncode != 0

    def test_ro_device(self, vm):
        if not isinstance(vm.test_disk.backend, YCVhostUserDiskBackend):
            pytest.skip("Read-only test supports vhost disks only")

        vm.shutdown()
        vm.test_disk.backend.restart(read_only=True)
        vm.start()

        status = vm.guest_exec(["readlink", "-f",
                                vm.test_disk.get_guest_path()])
        assert status.returncode == 0

        devpath, devname = os.path.split(str(status.stdout.decode()).strip().rstrip("\n"))
        assert devpath == "/dev"
        assert devname

        status = vm.guest_exec(["cat", "/sys/block/{}/ro".format(devname)])
        assert status.returncode == 0
        assert status.stdout

        output = str(status.stdout.decode()).rstrip()
        assert output == "1"

        self.write_to_ro_test_disk(vm)
        sleep(10)

        vm.shutdown()
        vm.test_disk.backend.restart(read_only=False)
        vm.start()

    def test_remount_rw_as_ro_device(self, vm):
        if not isinstance(vm.test_disk.backend, YCVhostUserDiskBackend):
            pytest.skip("Read-only test supports vhost disks only")

        vm.test_disk.backend.restart(read_only=True)
        sleep(1)
        self.write_to_ro_test_disk(vm)
        sleep(10)
        vm.test_disk.backend.restart(read_only=False)

    @pytest.mark.full
    def test_reboots(self, vm):
        if isinstance(vm.test_disk, VirtioFS):
            pytest.skip("Doesn't work with virtio-fs CLOUD-95221")
        vm.reboot()
        sleep(15)
        vm.reboot()
        sleep(11)
        vm.reboot()
        sleep(7)
        vm.reboot()
        sleep(3)
        vm.reboot()
        vm.wait_guest_boot()
        with Fio.start(vm, vm.test_disk):
            sleep(10)

    def test_reconnect(self, vm):
        if isinstance(vm.test_disk, VirtioFS):
            pytest.skip("Doesn't work with virtio-fs CLOUD-95221")
        sleep(1)
        with Fio.start(vm, vm.test_disk) as fio:
            sleep(5)
            assert fio.is_running()
            vm.test_disk.backend.restart()
            sleep(5)

    @pytest.mark.full
    def test_reconnect_multiple(self, vm):
        if isinstance(vm.test_disk, VirtioFS):
            pytest.skip("Doesn't work with virtio-fs CLOUD-95221")
        for _ in range(5):
            self.test_reconnect(vm)

    def test_migration(self, vm):
        if isinstance(vm.test_disk, VirtioFS):
            pytest.skip("Virtio-fs is not migratable")
        with Fio.start(vm, vm.test_disk):
            sleep(5)
            vm.migrate()
            sleep(5)

    @pytest.mark.full
    def test_migration_paused(self, vm):
        if isinstance(vm.test_disk, VirtioFS):
            pytest.skip("Virtio-fs is not migratable")
        with Fio.start(vm, vm.test_disk):
            sleep(5)
            vm.pause()
            vm.migrate()
            sleep(5)

    @pytest.mark.full
    def test_migration_rejection(self, vm):
        if isinstance(vm.test_disk, VirtioFS):
            pytest.skip("Virtio-fs is not migratable")
        if not isinstance(vm.test_disk, VhostUserDisk):
            pytest.skip("Migration should be rejected only for vhost disks")
        with Fio.start(vm, vm.test_disk):
            sleep(5)
            vm.test_disk.backend.stop(hard=True)
            with pytest.raises(VM.MigrationError):
                vm.migrate()
            vm.test_disk.backend.start()
            sleep(5)
            vm.migrate()

    @pytest.mark.full
    def test_qemu_crash(self, vm):
        if isinstance(vm.test_disk, VirtioFS):
            pytest.skip("Doesn't work with virtio-fs CLOUD-95221")
        vm.kill(hard=True)
        sleep(1)
        vm.start()
        with Fio.start(vm, vm.test_disk):
            sleep(3)

    def test_mem_table_change(self, vm):
        with Fio.start(vm, vm.test_disk):
            sleep(3)
            vm.hotplug_ram()
            sleep(3)
