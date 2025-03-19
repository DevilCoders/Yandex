from time import sleep

import pytest
from pytest_lazyfixture import lazy_fixture

from qemu import *
from common import *
from test_disk import gen_disks, SimpleVM
from test_disk import YC_Vhost_User, YC_Vhost_User_Slow, YC_Vhost_User_Virtio,\
    VirtioBlk, YC_VirtioFS


@pytest.fixture(scope="class")
def win_vm_simple(pytestconfig, request, win_boot_disk, tmp_dir):
    vm = SimpleVM(binary=get_qemu_bin(), work_dir=tmp_dir,
                  ssh_key=get_ssh_key(), boot_disk=win_boot_disk,
                  test_disk=None, os_type="windows")
    yield vm

    if pytestconfig.getoption('--keep-on-failure') and (
            request.node.rep_setup.failed or request.node.rep_call.failed):
        # FIXME: print VM parameters for debugging (folders, ssh port, etc.)
        print("test failed, keeping VM and disk backend as is")
        return
    vm.shutdown(guest_exec=True)


@pytest.fixture(scope="function")
def _checker_win_simple(request, win_vm_simple):
    def check():
        win_vm_simple.check()

    request.addfinalizer(check)


@pytest.fixture(scope="class", params=gen_disks())
def win_vm(pytestconfig, request, win_boot_disk, tmp_dir):
    disk = request.param
    if isinstance(disk, VirtioFS):
        pytest.skip("Virtio-fs doesn't work for Windows yet CLOUD-104209")
    vm = SimpleVM(binary=get_qemu_bin(), work_dir=tmp_dir,
                  ssh_key=get_ssh_key(), boot_disk=win_boot_disk,
                  test_disk=disk, os_type="windows")
    yield vm

    if pytestconfig.getoption('--keep-on-failure') and (
            request.node.rep_setup.failed or request.node.rep_call.failed):
        # FIXME: print VM parameters for debugging (folders, ssh port, etc.)
        print("test failed, keeping VM and disk backend as is")
        return
    vm.shutdown(guest_exec=True)
    disk.backend.stop(hard=False)


@pytest.fixture(scope="function")
def _checker_win(request, win_vm):
    def check():
        win_vm.check()
        win_vm.test_disk.backend.check()

    request.addfinalizer(check)

# Test cases to test Windows vm without secondary disk
# (with backend) just to test vm is booting successfuly
@pytest.mark.usefixtures('_checker_win_simple')
class TestWinSimple:
    @pytest.mark.full
    def test_win_boot(self, win_vm_simple):
        win_vm_simple.guest_exec(["tasklist"])


# Test cases to test Windows vm correctness of I/O on secondary
# disk with vhost backend
@pytest.mark.usefixtures('_checker_win')
class TestWinCommon:
    # find and return pid of running process by image name
    def find_process(self, vm, image_file_name):
        # get list of all processes
        status = vm.guest_exec(["tasklist", "/fi",
                                "\"IMAGENAME eq {}\"".format(image_file_name),
                                "/fo", "csv"])
        status.check_returncode()

        # parse output like
        # '"Image Name","PID","Session Name","Session#","Mem Usage"\r\n"fio.exe"\
        # ,"6984","Services","0","1,966,536 K"\r\n'
        stdout = status.stdout.decode().split("\r\n")
        if len(stdout) == 3:
            words = stdout[1].split(",")
            if len(words) > 2:
                if words[0].strip("\"") == image_file_name:
                    return int(words[1].strip("\""))

        return None

    def _kill_fio(self, vm, timeout=90):
        relax_delay = 10
        for _ in range(timeout//relax_delay):
            if self.find_process(vm, "fio.exe") is None:
                return

            vm.guest_exec(["taskkill", "/f", "/im", "fio.exe"])
            sleep(relax_delay)

    @pytest.mark.full
    def test_win_fio(self, win_vm):
        test_dir = "c:\\fiotest"

        # recreate test directory by remove & create
        win_vm.guest_exec(["rmdir", "/s", "/q", test_dir])
        status = win_vm.guest_exec(["mkdir", test_dir])
        status.check_returncode()

        # create and save fio job
        fio_job = [
            "[global]",
            "rw=randwrite",
            "ioengine=windowsaio",
            # Longest descriptor chain in windows virtio-blk driver is 515, out
            # of which 513 are for the request data.  513 * 4k = 2052k.
            # Make some requests bigger than that.
            "blocksize_range=512-4M",
            "iodepth=128",
            "name=fio_load",
            "filename=\\\\.\\PhysicalDrive1", # secondary raw disk
            "direct=1",
            "runtime=300",
            "time_based=1",
            "verify=crc32c",
            "verify_backlog=128",
            "size=1G",
            "exitall=1",
            "thread=1"
        ]

        for n in range(win_vm.cpu_count):
            fio_job.append("[write-verify-{num}]".format(num=n))
            fio_job.append("offset={num}G".format(num=n))

        for line in fio_job:
            status = win_vm.guest_exec(
                ["echo " + line + " >> {}\\fio.job".format(test_dir)])
            status.check_returncode()

        # launch fio
        status = win_vm.guest_exec(["wmic", "process", "call", "create",
                "'fio --output {}\\fio.log {}\\fio.job'".format(test_dir, test_dir)])
        status.check_returncode()
        sleep(5)

        # check fio is running
        assert self.find_process(win_vm, "fio.exe") > 0

        # wait a bit for I/O progress
        sleep(60)

        # check fio is still running
        assert self.find_process(win_vm, "fio.exe") > 0

        # kill fio
        self._kill_fio(win_vm)

        # check fio is not running
        assert self.find_process(win_vm, "fio.exe") is None
