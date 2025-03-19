import logging
import os
import signal
import subprocess
from abc import ABC, abstractmethod
from time import sleep


class DiskBackendError(Exception):
    def __init__(self, message):
        super().__init__(message)


class DiskBase(ABC):
    def __init__(self, backend, bootable, with_iothread):
        self.disk_id = backend.disk_id
        self.backend = backend
        self.bootable = bootable
        self.with_iothread = with_iothread
        self.device_add_arguments = {}
        if self.with_iothread:
            self.iothread_id = "iot-{}".format(self.disk_id)
            self.device_add_arguments.update({"iothread": self.iothread_id})

    def get_hotplug_args(self, vm, additional_options=None, bootable=False):
        bus_id = vm.get_free_disk_bus()

        self.device_add_arguments.update({
            "bus": bus_id,
        })

        if bootable:
            # FIXME: check that VM has only one bootable disk
            self.device_add_arguments.update({"bootindex": '1'})
        if additional_options is not None:
            self.device_add_arguments.update(additional_options)

        return self.device_add_arguments

    @abstractmethod
    def get_guest_path(self):
        pass

    def setup_guest(self, vm):
        pass


class MultiQueueDevice(DiskBase, ABC):
    def get_hotplug_args(self, vm, additional_options=None, bootable=False):
        super().get_hotplug_args(vm, additional_options, bootable)
        self.device_add_arguments.update({
            "num-queues": vm.cpu_count,
        })

        return self.device_add_arguments


class VirtioDisk(MultiQueueDevice, DiskBase):
    device_type = "virtio-blk-pci"

    def __init__(self, backend, bootable, with_iothread=True):
        super().__init__(backend, bootable, with_iothread)
        self.disk_device_id = "virtio-disk-{}".format(self.disk_id)
        self.disk_serial = backend.disk_serial
        self.device_add_arguments.update({
            "driver": self.device_type,
            "disable-legacy": "off",
            "write-cache": "off",
            "rerror": "report",
            "werror": "report",
            "drive": self.backend.node_name,
            "serial": self.disk_serial,
            "id": self.disk_device_id,
            # "addr": ??? # used in K8S topology
            "config-wce": "off"
        })

    def get_hotplug_args(self, vm, additional_options=None, bootable=False):
        hotplug_args = super().get_hotplug_args(vm, additional_options,
                                                bootable)
        # We have to disable these features to make migration work since
        # libvhost doesn't currently implement them.
        if vm.qemu_version[0] >= 5:
            hotplug_args.update({
                "write-zeroes": "off",
                "discard": "off"
            })

        return hotplug_args

    def get_guest_path(self):
        return "/dev/disk/by-id/virtio-{}".format(self.disk_serial)


class VhostUserDisk(MultiQueueDevice, DiskBase):
    device_type = "vhost-user-blk-pci"

    def __init__(self, backend, bootable=False):
        super().__init__(backend, bootable, with_iothread=False)
        self.disk_device_id = "vhost-user-blk-{}".format(backend.disk_id)
        self.disk_serial = backend.disk_serial
        self.device_add_arguments.update({
            "driver": self.device_type,
            "id": self.disk_device_id,
            "disable-legacy": "off",
            "chardev": self.backend.chardev_id,
            "config-wce": "off",
        })

    def get_guest_path(self):
        return "/dev/disk/by-id/virtio-{}".format(self.disk_serial)


class VhostUserVirtioDisk(VhostUserDisk):
    """
    This is special device type that uses vhost backend and virtio-blk
    migration stream format. We use this device type for transition
    from virtio-blk to vhost.
    """
    device_type = "vhost-user-virtio-blk-pci"


class DiskBackendBase(ABC):
    def __init__(self, disk_id, image):
        self.disk_id = disk_id
        self.image = image

    @abstractmethod
    def get_connect_cmd(self, vm):
        """ Return tuple of QMP command and its args """
        pass

    # These methods are not abstract on purpose:
    # backends that run in separate processes override them
    # but common implementation is just doing nothing.
    def start(self):
        pass

    def check(self):
        pass

    def stop(self, hard):
        pass

    def restart(self, read_only=False):
        self.stop(hard=True)
        self.start()


class DaemonDiskBackend(DiskBackendBase, ABC):
    """
    This class represents disk backend running in separate
    process that communicates with QEMU via unix socket.
    """

    def __init__(self, binary, work_dir, wrapper='', **kwargs):
        super().__init__(**kwargs)
        self.work_dir = work_dir
        self.sock_path = os.path.join(work_dir, '{}.sock'.format(self.disk_id))
        self.proc = None
        self._pid_file = None
        self.binary = binary
        self.options = {}
        self._stop_timeout = 30  # seconds
        self.wrapper = wrapper
        log_file = "{}_backend.log".format(self.disk_id)
        self._backend_log_fd = open(os.path.join(self.work_dir,
                                                 log_file),
                                    "w")

    def start(self):
        option_list = []
        for key, value in self.options.items():
            if value is not None:
                option_list.append("{}={}".format(key, value))
            else:
                option_list.append(key)

        cmd = [
            'exec',
            self.wrapper,
            self.binary,
            *option_list
        ]

        logging.debug('Run backend: %s', ' '.join(cmd))
        if self.proc is not None:
            raise DiskBackendError('Disk backend process has already been '
                                   'started')
        if self.binary is None:
            raise DiskBackendError('Backend daemon binary not set')

        self.proc = subprocess.Popen(' '.join(cmd),
                                     shell=True,
                                     stdout=self._backend_log_fd,
                                     stderr=subprocess.STDOUT)

        # give a backend time to start (or fail)
        retry = 0
        retry_limit = 5
        while True:
            if os.path.exists(self.sock_path):
                break
            elif retry < retry_limit:
                retry += 1
                sleep(1)
            else:
                raise DiskBackendError("Failed to start backend")

        self.check()
        pid_file_name = os.path.basename(self.binary) + "-pid"
        self._pid_file = os.path.join(self.work_dir, pid_file_name)
        with open(self._pid_file, "w") as f:
            f.write(str(self.proc.pid))

    def check(self):
        if self.proc is None:
            raise DiskBackendError('Disk backend process not started')
        res = self.proc.poll()
        if res is not None:
            raise DiskBackendError('Disk backend daemon has exited with '
                                   'result: {}'.format(res))

    def stop(self, hard):
        self.check()

        if self._pid_file is not None and os.path.isfile(self._pid_file):
            os.remove(self._pid_file)

        if not hard:
            self.proc.send_signal(signal.SIGINT)
        else:
            self.proc.kill()
        self.proc.wait(timeout=self._stop_timeout)
        self.proc = None


class FileBackend(DiskBackendBase):
    """
    This type of disk is emulated inside QEMU, so it has no dedicated backend
    process
    """

    def __init__(self, image_type, **kwargs):
        super().__init__(**kwargs)
        self.node_name = "node-{}".format(self.disk_id)
        self.blockdev_args = {
            "driver": image_type,
            "node-name": self.node_name,
            "cache": {
                "direct": True
            },
            "detect-zeroes": "on",
            "file": {
                "driver": "file",
                "filename": self.image
            }
        }
        self.disk_serial = self.disk_id

    def get_connect_cmd(self, vm):
        return 'blockdev-add', self.blockdev_args


class VhostUserDiskBackend(DaemonDiskBackend):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.chardev_id = "vhost-user-blk-{}.sock".format(self.disk_id)
        # TODO: warn if disk_id is too long or trim it to length accepted by
        #  vhost protocol
        self.disk_serial = self.disk_id

    def get_connect_cmd(self, vm):
        chardev_args = {
            "id": self.chardev_id,
            "backend": {
                "type": "socket",
                "data": {
                    "reconnect": 1,
                    "server": False,
                    "addr": {
                        "type": "unix",
                        "data": {
                            "path": self.sock_path
                        }
                    }
                }
            }
        }
        return 'chardev-add', chardev_args


class YCVhostUserDiskBackend(VhostUserDiskBackend):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.sock_qemu = self.sock_path
        self.options = {
            '--blk-file': self.image,
            '--socket-path': self.sock_path,
            '--serial': self.disk_serial,
        }

    def restart(self, read_only=False):
        self.stop(hard=False)
        if read_only:
            self.options.update({"--readonly": None})
        else:
            self.options.pop("--readonly", None)
        self.start()


class YCVhostUserSlowDiskBackend(YCVhostUserDiskBackend):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.options.update({
            '--delay': str(1000),
        })


class VirtioFS(DiskBase):
    device_type = "vhost-user-fs-pci"

    def __init__(self, backend, bootable=False):
        super().__init__(backend, bootable, with_iothread=False)
        self.disk_device_id = "vhost-user-blk-{}".format(backend.disk_id)
        self.disk_serial = backend.disk_serial
        self.guest_dir = "/mnt/{}".format(self.disk_id)
        self.scratch_file_name = os.path.basename(self.backend.image)
        self.tag = self.disk_id + '_tag'
        self.device_add_arguments.update({
            "driver": self.device_type,
            "id": self.disk_device_id,
            "chardev": self.backend.chardev_id,
            "tag": self.tag
        })

    def get_guest_path(self):
        return "{}/{}".format(self.guest_dir, self.scratch_file_name)

    def setup_guest(self, vm):
        super().setup_guest(vm)
        # check if FS is already mounted
        logging.debug(vm.guest_exec(['cat /etc/fstab']).stdout.decode())
        res = vm.guest_exec(
            ['findmnt', '--noheadings', '-t virtiofs', self.tag, '--kernel'])
        if len(res.stdout) == 0:
            # check is FS is in fstab
            res = vm.guest_exec(
                ['findmnt', '--noheadings', '-t virtiofs', self.tag, '--fstab'])
            if len(res.stdout) == 0:
                # create dir and add FS to fstab
                vm.guest_exec(['mkdir {}'.format(self.guest_dir)])
                vm.guest_exec(["echo '{} {} virtiofs defaults 1 1 "
                               "# added by pytest' >> /etc/fstab".format(
                                    self.tag, self.guest_dir
                                )]).check_returncode()

            logging.debug(vm.guest_exec(['cat /etc/fstab']).stdout.decode())
            # mount FS
            vm.guest_exec(
                ['mount', '-t', 'virtiofs', self.tag]).check_returncode()
            vm.guest_exec(['sync']).check_returncode()


class VirtioFSBackend(DaemonDiskBackend):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.chardev_id = "virtiofs-{}.sock".format(self.disk_id)
        # TODO: warn if disk_id is too long or trim it to length accepted by
        #  vhost protocol
        self.disk_serial = self.disk_id

        self.options = {
            '--socket-path': self.sock_path,
            '-osource': os.path.dirname(self.image),
        }

    def get_connect_cmd(self, vm):
        chardev_args = {
            "id": self.chardev_id,
            "backend": {
                "type": "socket",
                "data": {
                    "reconnect": 1,
                    "server": False,
                    "addr": {
                        "type": "unix",
                        "data": {
                            "path": self.sock_path
                        }
                    }
                }
            }
        }
        return "chardev-add", chardev_args
