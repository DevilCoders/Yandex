import logging
from time import sleep

logger = logging.getLogger(__name__)


class FioError(Exception):
    def __init__(self, message):
        super().__init__(message)


class FioProcess:
    def __init__(self, vm, disk):
        # TODO: store just values that are necessary
        self.vm = vm
        self.disk = disk

        fio_job = [
            "[global]",
            "rw=randwrite",
            "ioengine=libaio",
            "blocksize_range=512-1M",
            "iodepth=128",
            "name=fio_load_{}".format(disk.disk_id),
            "filename={}".format(disk.get_guest_path()),
            "direct=1",
            "runtime=300",
            "time_based=1",
            "verify=crc32c",
            "verify_backlog=128",
            "size=1G",
            "exitall=1"
        ]

        for n in range(vm.cpu_count):
            fio_job += ["[write-verify-{num}]\noffset={num}G".format(num=n)]

        fio_job = "\n".join(fio_job)
        fio_job = "echo '" + fio_job + "' > /fio.job"

        status = vm.guest_exec([fio_job])
        status.check_returncode()

        status = vm.guest_exec(["rm -rf /fio.log"])
        status.check_returncode()

        args = "fio /fio.job >/fio.log 2>&1 & echo $!"

        status = self.vm.guest_exec([args])
        status.check_returncode()

        if len(status.stdout) > 0:
            decoded_stdout = status.stdout.decode()
            self._pid = int(decoded_stdout)
            return

        raise Exception("Can't find fio after start")

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.check_and_stop()

    def _wait_fio_dead(self, timeout=90):
        relax_delay = 10
        for _ in range(timeout//relax_delay):
            if not self.is_running():
                return

            result = self.vm.guest_exec(["ps -aux | grep [f]io"])
            logger.info("fio %u is still running: %s", self._pid, result.stdout)
            sleep(relax_delay)
        raise FioError("Failed to stop fio")

    def check_and_stop(self):
        if not self.is_running():
            res = self.vm.guest_exec(["cat", "/fio.log"])
            raise FioError('Fio process is not running. Log: {}'.format(res.stdout))
        self.vm.guest_exec(["kill", str(self._pid)])
        sleep(10)
        self._wait_fio_dead()
        self._pid = None

    def is_running(self) -> bool:
        if self._pid is None:
            raise FioError('Fio process was not run')
        status = self.vm.guest_exec(["kill", "-0", str(self._pid)])
        return status.returncode == 0

class Fio:
    def __init__(self):
        pass

    @staticmethod
    def start(vm, disk):
        return FioProcess(vm, disk)
