import json
import logging
import os
import re
import signal
import socket
import subprocess
import sys
import time
from collections import deque
from time import sleep

logger = logging.getLogger(__name__)


class QmpException(Exception):
    def __init__(self, qmp_result):
        message = "{}: {}".format(qmp_result["class"], qmp_result["desc"])
        super().__init__(message)


class QmpClient:
    timeout = 120

    def __init__(self, sock_path):
        self._events = deque()
        self._qmp_message = None
        self._sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self._sock.settimeout(self.timeout)

        time_end = time.time() + self.timeout
        while True:
            try:
                self._sock.connect(sock_path)
                break
            except (FileNotFoundError, ConnectionRefusedError) as e:
                if time.time() > time_end:
                    logger.debug("%s: timed out", sock_path)
                    raise

                logger.debug("%s: %s: retrying", sock_path, e)
                time.sleep(1)

        self.command("qmp_capabilities")

    def close(self):
        if self._sock:
            self._sock.close()
            self._sock = None

    def command(self, command, **arguments):
        cmd = {"execute": command}
        if arguments:
            cmd["arguments"] = arguments

        qmp_cmd = json.dumps(cmd).encode("utf-8")
        logger.debug("QMP> %s", qmp_cmd)
        self._sock.sendall(qmp_cmd)
        result = None
        while result is None:
            result = self._recv_data()

        logger.debug("QMP< %s", result)
        return result

    def wait_event(self):
        while not self._events:
            res = self._recv_data()
            if res:
                raise Exception("Unexpected result: {}".format(res))
        return self._events.popleft()

    def _recv_data(self, events_only=False):
        result = None
        for message in self._recv_messages():
            if "event" in message.keys():
                self._events.append(message["event"])
            elif "QMP" in message.keys():
                self._qmp_message = message["QMP"]
            elif "return" in message.keys():
                if result or events_only:
                    raise Exception("Unexpected result: {}".format(message))
                result = message["return"]
            elif "error" in message.keys():
                raise QmpException(message["error"])
            elif "timestamp" in message.keys():
                pass
            elif "data" in message.keys():
                pass
            else:
                raise Exception("Unknown message type: {}".format(message))
        return result

    def _recv_messages(self):
        data = []
        while not (data and data[-1].endswith(b"\n")):
            data.append(self._sock.recv(4096))
        messages = []
        for line in b"".join(data).decode("utf-8").splitlines():
            try:
                messages.append(json.loads(line))
            except Exception as e:
                logger.error("Failed to parse message %s: %s\n", line, e)
                raise
        return messages


class VMError(Exception):
    def __init__(self, message):
        super().__init__(message)


class VM:
    def _get_cpu_opts(self):
        opts = "Haswell-noTSX,l3-cache=on,+md-clear,+spec-ctrl,+ssbd,+stibp,+vmx"
        if self._os_type == "windows":
            opts += ",hv_relaxed,hv_spinlocks=0x1fff,hv_vapic,hv_time,hv_crash"
            opts += ",hv_reset,hv_vpindex,hv_runtime,hv_synic,hv_stimer"

        return opts

    def supports_device(self, device_type):
        return device_type in self._supported_device_list

    def _size_vm(self, size):
        host_frac_cpu = .25
        host_frac_mem = .25
        mem_per_cpu = 1.

        self.cpu_count = min(size, round(os.cpu_count() * host_frac_cpu))
        host_mem = os.sysconf('SC_PHYS_PAGES') * os.sysconf('SC_PAGESIZE')
        self.mem_size_gb = min(round(self.cpu_count * mem_per_cpu),
                               round(host_mem * host_frac_mem / (1 << 30)))
        # whatever is the limiting factor, we still want the proportion
        self.cpu_count = min(self.cpu_count, self.mem_size_gb / mem_per_cpu)

    def __init__(self, binary, work_dir, ssh_key, os_type=None):
        self.binary = binary
        self.work_dir = work_dir
        self._net_count = 0
        self._disks = []
        self._disk_list = []
        self._size_vm(16)
        self._mem_id = 'memory'
        self.hotplug_mem_size_gb = 1
        self.max_mem_size_gb = self.mem_size_gb + self.hotplug_mem_size_gb
        self.log_fd = open(os.path.join(self.work_dir, "qemu.log"), "w")
        self._check_qemu_bin()
        self._qmp_sock_path = os.path.join(self.work_dir, 'qmp.sock')
        self._vnc_sock = os.path.join(self.work_dir, 'vnc.sock')
        self._serial_log = os.path.join(self.work_dir, 'serial0.log')
        self._bios_log = os.path.join(self.work_dir, 'seabios.log')
        self.migrate_state_path = os.path.join(self.work_dir, 'migrate.state')
        self._pid_file = os.path.join(self.work_dir, 'qemu-pid')
        self._os_type = os_type
        self.proc = None
        self._ssh_port = None
        self._ssh_key = ssh_key
        self._qmp_client = None

        # even number greater than 0
        assert (self.cpu_count > 0 and self.cpu_count & 0x1 == 0)

        vm_uuid = "ffffffff-ffff-ffff-ffff-ffffffffffff"
        self.args = [
            self.binary,
            # Machine args
            "-uuid", "{}".format(vm_uuid),
            "-name", "test,debug-threads=on",
            "-qmp", "unix:{},server,nowait".format(self._qmp_sock_path),
            "-msg", "timestamp=on",

            # start in stopped state to hotplug devices
            "-S",

            # machine
            "-M", "q35,sata=false,usb=off,accel=kvm",

            # CPU and memory
            "-cpu", self._get_cpu_opts(),
            "-smp", "{},cores={},threads=2,sockets=1"
                .format(self.cpu_count, int(self.cpu_count / 2)),
            "-m", "size={}G,slots=2,maxmem={}G"
                .format(self.mem_size_gb, self.max_mem_size_gb),
            # FIXME: adding ",prealloc=on" breaks migration
            "-object", "memory-backend-memfd,id={},size={}G,policy=bind,"
                       "host-nodes=0"
                .format(self._mem_id, self.mem_size_gb),
            "-numa", "node,cpus=0-{},memdev={}"
                .format(self.cpu_count - 1, self._mem_id),

            # misc devices
            "-vga", "std",
            "-device", "usb-ehci",
            "-device", "usb-tablet",
            "-nodefaults",

            # devices
            # PCI topology
            "-device", "pxb-pcie,bus_nr=128,bus=pcie.0,id=pcie.1,numa_node=0",
            "-device", "pcie-root-port,id=s0,slot=0,bus=pcie.1",
            "-device", "pcie-root-port,id=s1,slot=1,bus=pcie.1",
            "-device", "pcie-root-port,id=s2,slot=2,bus=pcie.1",
            "-device", "pcie-root-port,id=s3,slot=3,bus=pcie.1",
            "-device", "pcie-root-port,id=s4,slot=4,bus=pcie.1",
            "-device", "pcie-root-port,id=s5,slot=5,bus=pcie.1",
            "-device", "pcie-root-port,id=s6,slot=6,bus=pcie.1",
            "-device", "pcie-root-port,id=s7,slot=7,bus=pcie.1",
            "-device", "pxb-pcie,bus_nr=137,bus=pcie.0,id=pcie.2,numa_node=0",
            "-device", "pcie-root-port,id=s8,slot=8,bus=pcie.2",
            "-device", "pcie-root-port,id=s9,slot=9,bus=pcie.2",
            "-device", "pcie-root-port,id=s10,slot=10,bus=pcie.2",
            "-device", "pcie-root-port,id=s11,slot=11,bus=pcie.2",
            "-device", "pcie-root-port,id=s12,slot=12,bus=pcie.2",
            "-device", "pcie-root-port,id=s13,slot=13,bus=pcie.2",
            "-device", "pcie-root-port,id=s14,slot=14,bus=pcie.2",
            "-device", "pcie-root-port,id=s15,slot=15,bus=pcie.2",

            # VNC
            # Use "socat TCP-LISTEN:5900 UNIX-CONNECT:vnc.sock" to connect
            "-vnc", "unix:{}".format(self._vnc_sock),

            # Serial ports:
            # Compute uses 4 serial ports connected to serial-proxies for
            # console
            # In test we need just two: one for guest and one for BIOS debugging
            "-chardev", "file,path={},id=charserial0".format(self._serial_log),
            "-device", "isa-serial,chardev=charserial0,id=serial0",

            "-chardev", "file,path={},id=debugcon".format(self._bios_log),
            "-device", "isa-debugcon,iobase=0x402,chardev=debugcon",

            # misc options
            '-boot', 'strict=on',

            # TODO: Not used in test
            # '-seccomp', 'on,\
            #    policy=deny,\
            #    denyaction=log,
            #    exception=/usr/share/qemu/seccomp_whitelist.json'

            # TODO: not used by compute, should they add this flag?
            "-no-user-config",
        ]

    def _wait(self, timeout=30):
        self.proc.wait(timeout=timeout)

        if os.path.isfile(self._pid_file):
            os.remove(self._pid_file)

        self.proc = None
        self._qmp_client = None
        self._ssh_port = None
        self._disks = []
        self._net_count = 0

    def kill(self, hard=False):
        self.check(check_guest_reachable=False)

        if not hard:
            self.proc.send_signal(signal.SIGINT)
        else:
            self.proc.kill()

        self._wait()


    def shutdown(self, guest_exec=False):
        self.check(check_guest_reachable=guest_exec)
        timeout = 60
        if guest_exec:
            if self._os_type == "windows":
                self.guest_exec(["shutdown", "/s", "/f", "/t", "0"])
                timeout = 180
            else:
                self.guest_exec(["shutdown", "--poweroff", "now"])
        else:
            self._qmp_command("system_powerdown")

        self._wait(timeout=timeout)


    def check(self, check_guest_reachable=True):
        if self.proc is None:
            raise VMError('VM process has not run')
        res = self.proc.poll()
        if res is not None:
            raise VMError('VM process exited with result: {}'.format(res))
        if check_guest_reachable:
            self._wait_for_ssh()

    def disk_add(self, disk):
        self._disk_list.append(disk)

    def _attach_disks(self):
        for disk in self._disk_list:
            if disk.backend is not None:
                cmd, backend_args = disk.backend.get_connect_cmd(self)
                self._qmp_command(cmd, **backend_args)

            if disk.with_iothread:
                self.add_iothread(disk.iothread_id)

            device_args = disk.get_hotplug_args(self)
            self._qmp_command('device_add', **device_args)
            self._disks.append(disk)

    def add_iothread(self, iothread_id):
        self._qmp_command(
            'object-add', **{
                "id": iothread_id,
                "qom-type": "iothread"
            })

    def reboot(self):
        self._qmp_command("system_reset")

    def start(self):
        self._start_qemu(migrate=False)
        self.wait_guest_boot()
        self._post_boot_setup()

    def _start_qemu(self, migrate=False):
        run_args = self.args.copy()
        if migrate:
            run_args.extend(["-incoming", "defer"])
        logger.debug("starting QEMU with cmdline: %s", ' '.join(run_args))

        # Write to qemu log start time and command with args before
        # launching process. Avoid writing to log_fd while VM is running to
        # be sure that logs are not overwritten
        # TODO: wrap log with timestamp and flush
        self.log_fd.write("starting QEMU at {}\ncmd: {}\n".format(
            time.strftime("%d.%m.%Y %H:%M:%S", time.localtime()),
            ' '.join(run_args)))
        self.log_fd.flush()

        if self.proc is not None:
            raise VMError('VM process has already been started')

        self.proc = subprocess.Popen(run_args, stdout=self.log_fd,
                                     stderr=subprocess.STDOUT)

        res = self.proc.poll()
        if res is not None:
            raise VMError('VM process exited with result: {}'.format(res))

        try:
            with open(self._pid_file, "w") as f:
                f.write(str(self.proc.pid))

            if self._qmp_client is not None:
                raise VMError('QMP is already set up')
            self._qmp_client = QmpClient(self._qmp_sock_path)

            self.hotplug_net_user()

            self._attach_disks()

            self._init_migration_params()

            # TODO: not sure if we need reset before incoming migration
            self._qmp_command("system_reset")
            if not migrate:
                self._qmp_command("cont")
            logger.debug("QEMU started with PID %d", self.proc.pid)
        except Exception:
            logger.info("Killing QEMU because of Exception during VM setup")
            self.kill()
            raise

    def hotplug_net_user(self):
        qemu_major_ver = self.qemu_version[0]
        # Allow QEMU to use any free port; find out the used port later via qmp
        hostfwd = [
            {"str": "tcp::0-:22"}] if qemu_major_ver >= 5 else "tcp::0-:22"

        self._qmp_command("netdev_add", **{
            "id": "netdev{}".format(self._net_count),
            "type": "user",
            "hostfwd": hostfwd,
        })

        device_add_arguments = {
            "driver": "virtio-net-pci",
            "id": "net{}".format(self._net_count),
            "netdev": "netdev{}".format(self._net_count),
            "mac": "aa:aa:aa:aa:aa:aa",  # TODO
            "mq": "on",
            "disable-legacy": "off",
            "vectors": str(self.cpu_count * 2 + 2),
            "bus": "s{}".format(self._net_count + 8),
        }

        # Due to bug in net-user https://st.yandex-team.ru/CLOUD-77917
        # TODO: drop this hack after bug is fixed
        if qemu_major_ver <= 2:
            device_add_arguments.update({"event_idx": "off"})

        self._qmp_command(
            "device_add", **device_add_arguments)
        self._net_count += 1

        if self._ssh_port is not None:
            raise VMError('VM has already setup SSH port')
        usernet_info = self._qmp_command("human-monitor-command",
                                         **{"command-line": "info usernet"})
        for line in usernet_info.splitlines():
            fields = line.split()
            if fields[0] == "TCP[HOST_FORWARD]" and fields[5] == "22":
                if self._ssh_port is not None:
                    raise Exception("Guest has multiple 22 ports")
                self._ssh_port = int(fields[3])

    def get_free_disk_bus(self):
        return 's{}'.format(len(self._disks))

    class MigrationError(Exception):
        def __init__(self, status):
            super().__init__(status)

    def migrate(self):
        self.save_to_file()
        self.load_from_file()

    def _init_migration_params(self):
        # setup migration parameters and capabilities
        # TODO: add code for remote migration
        local = True

        self.migration_parameters = {
            "downtime-limit": 1000,
            "max-bandwidth": sys.maxsize
        }

        self.migration_capabilities = [
            # Enable migration QMP events
            {"capability": "events", "state": True},

            # Enable RAM migration
            {"capability": "x-ignore-shared", "state": False},

            # QEMU will automatically throttle down the guest to speed up
            # convergence of RAM migration
            {"capability": "auto-converge", "state": True},

            # Disable migration state compression - it significantly slows down
            # migration and interfere with auto conversion logic
            {"capability": "compress", "state": False},
        ]

        # add UUID validation for remote migration if it is supported
        migrate_capabilities = self._qmp_command("query-migrate-capabilities")

        validate_uuid_capability_name = None
        for cap in migrate_capabilities:
            # "x-validate-uuid" or "validate-uuid"
            if "validate-uuid" in cap["capability"]:
                validate_uuid_capability_name = cap["capability"]

        if validate_uuid_capability_name is not None:
            self.migration_capabilities.append({
                "capability": validate_uuid_capability_name, "state": not local,
            })

    def save_to_file(self):
        # useful for debugging, enabled in preprod
        self._qmp_command("trace-event-set-state",
                          **{"name": "migration_*", "enable": True, })

        self._qmp_command("migrate-set-parameters", **self.migration_parameters)
        self._qmp_command("migrate-set-capabilities",
                          **{"capabilities": self.migration_capabilities})

        # compute uses fd but for purpose of this test file is OK
        self._qmp_command("migrate", **{
            "uri": ("exec:dd if=/dev/stdin of={} bs=1M " +
                    "count=32768 iflag=fullblock").format(self.migrate_state_path)})
        migrate_status = self._qmp_command("query-migrate")
        logger.info("migrate_status {}".format(json.dumps(migrate_status)))
        while migrate_status['status'] in ('active', 'setup', 'device'):
            sleep(1)
            migrate_status = self._qmp_command("query-migrate")
            logger.info("migrate_status {}".format(json.dumps(migrate_status)))

        if migrate_status['status'] != "completed":
            self.log_fd.write(
                "VM migration finished {0} with status {1}\n{2}\n".format(
                    time.strftime("%d.%m.%Y %H:%M:%S", time.localtime()),
                    migrate_status['status'],
                    json.dumps(migrate_status)))
            self.log_fd.flush()
            raise self.MigrationError(migrate_status['status'])

        self.log_fd.write("VM migrated to file {0} at {1}\n".format(
            self.migrate_state_path, time.strftime("%d.%m.%Y %H:%M:%S",
                                                   time.localtime())))
        self.log_fd.flush()

        # this part is to check that destination VM can load from
        # migration stream
        self.kill()

    def load_from_file(self):
        self._start_qemu(migrate=True)
        self._qmp_command("trace-event-set-state",
                          **{"name": "migration_*", "enable": True, })
        self._qmp_command("migrate-set-parameters", **self.migration_parameters)
        self._qmp_command("migrate-set-capabilities",
                          **{"capabilities": self.migration_capabilities})
        self._qmp_command("migrate-incoming",
                          **{"uri": "exec:cat {}".format(
                              self.migrate_state_path)})

        # we can enable migration events and wait for MIGRATION event
        # with {'status': 'completed'}
        migrate_status = self._qmp_command("query-status")
        while migrate_status['status'] == 'inmigrate':
            sleep(1)
            migrate_status = self._qmp_command("query-status")
        self._qmp_command("cont")
        self.log_fd.flush()

    def hotplug_ram(self):
        vm_hot_plug_mem_name = "ram-hot-plug"
        self._qmp_command(
            'object-add', **{
                "id": vm_hot_plug_mem_name,
                "qom-type": "memory-backend-memfd",
                "props": {
                    "size": self.hotplug_mem_size_gb * 1024 ** 3,
                    "policy": "bind",
                    "host-nodes": [0]
                }
            })

        self._qmp_command("device_add",
                          **{
                              "driver": "pc-dimm",
                              "id": "dimm10",
                              "memdev": vm_hot_plug_mem_name,
                          })

    def guest_exec(self, args) -> subprocess.CompletedProcess:
        """
        Run command in guest
        :rtype: subprocess.CompletedProcess
        :param args: list of string arguments
        :return: CompletedProcess instance
        """
        self._wait_for_ssh()
        return self._ssh(args)

    def _post_boot_setup(self):
        logger.debug("Running guest setup after boot")
        for disk in self._disk_list:
            disk.setup_guest(self)

    def wait_guest_boot(self):
        self._wait_for_ssh()

    def _get_ssh_user(self):
        if self._os_type == "windows":
            return "Administrator"
        else:
            return "root"

    def _get_ssh_passive_cmd(self):
        if self._os_type == "windows":
            return "echo."
        else:
            return "true"

    def _ssh(self, args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
             timeout=60) -> subprocess.CompletedProcess:
        ssh_args = [
            "ssh",
            "-oGlobalKnownHostsFile=" + os.devnull,
            "-oUserKnownHostsFile=" + os.devnull,
            "-oPasswordAuthentication=no",
            "-oStrictHostKeyChecking=no",
            "-oTCPKeepAlive=no",
            "-oServerAliveInterval=3",
            "-oServerAliveCountMax=10",
            "-i", self._ssh_key,  # default QEMU test key
            "-p", str(self._ssh_port),
            "{}@localhost".format(self._get_ssh_user()),
        ]

        try:
            res = subprocess.run(ssh_args + args, stdout=stdout, stderr=stderr,
                                 timeout=timeout)
        except Exception as e:
            logger.debug("SSH command failed: %s", e)
            raise
        logger.debug("SSH command returned: %s", res)
        return res

    def _wait_for_ssh(self, timeout=300, step=5):
        start_time = time.monotonic()
        end_time = start_time + timeout

        while time.monotonic() < end_time:
            try:
                ret = self._ssh([self._get_ssh_passive_cmd()], timeout=step)
            except subprocess.TimeoutExpired:
                continue
            if ret.returncode == 0:
                return
            if 'UNPROTECTED PRIVATE KEY FILE' in str(ret.stderr):
                raise Exception('Fix key permissions')
            sleep(step)

        logger.error("SSH inaccessible more than %d secs", timeout)
        raise TimeoutError("SSH inaccessible more than {} secs".format(timeout))

    def _check_qemu_bin(self):
        # this checks that qemu_bin actually works
        # if not, fire an exception
        self.qemu_version = self._get_qemu_version()
        self._supported_device_list = self._get_supported_devices()

    def _get_qemu_version(self):
        # throws an exception when qemu isn't found or timeout expired
        ver_str = subprocess.check_output([self.binary, "-version"],
                                          universal_newlines=True)
        # look for the first occurrence of version X.X{X}.X{X}
        s = re.search(r'\d\.\d+\.\d+', ver_str)
        if s is None:
            raise Exception("can't find version in qemu output: '{}'".
                            format(ver_str))
        v = s.group(0)
        # list with the major in [0] and minors further
        v = v.split(".")
        # convert ver strings to int numbers
        v = list(map(int, v))
        return v

    def _get_supported_devices(self):
        device_output = subprocess.check_output([self.binary, "-device", "?"],
                                                universal_newlines=True,
                                                # qemu2 prints device list to stderr
                                                stderr=subprocess.STDOUT)
        # parse device names, ignore descr and bus
        device_list = re.findall(r'name "(\S+)"', device_output)
        if device_list is None:
            raise Exception("can't load supported devices: '{}'".
                            format(device_output))
        return device_list

    def _qmp_command(self, cmd, **args):
        return self._qmp_client.command(cmd, **args)

    def pause(self):
        self._qmp_command('stop')
