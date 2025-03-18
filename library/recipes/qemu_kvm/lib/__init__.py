import os
import signal
import logging
import getpass
import argparse

import yatest.common
import yatest.common.network
import qemu
import uuid
import retrying
import tempfile
import yaml
import pipes

import library.python.fs as fs
import library.python.testing.recipe


logger = logging.getLogger(__name__)


class QemuKvmRecipeException(Exception):

    def __init__(self, msg):
        super(QemuKvmRecipeException, self).__init__("[[bad]]{}[[rst]]".format(msg))


def start(argv):
    args = _parse_args(argv)

    qemu_kmv = _get_qemu_kvm()
    rootfs = _get_rootfs(args)
    kernel = _get_kernel(args)
    kcmdline = _get_kcmdline(args)
    initrd = _get_initrd(args)
    local_ssh_forwarding_port = yatest.common.network.PortManager().get_port()
    ssh_user = _get_ssh_user(args)
    ssh_key = _get_ssh_key(args)
    ssh_pubkey = _get_ssh_pubkey(args)
    mem = _get_vm_mem(args)
    proc = _get_vm_proc(args)
    qemu_options = _get_qemu_options(args)
    hostname = "qavm-{}.qemu".format(str(uuid.uuid4())[0:8])

    if not qemu.kvm_available():
        msg = "/dev/kvm is not available"
        logger.error(msg)
        raise QemuKvmRecipeException(msg)

    library.python.testing.recipe.set_env("QEMU_FORWARDING_PORT", str(local_ssh_forwarding_port))
    library.python.testing.recipe.set_env("QEMU_HOSTNAME", str(hostname))
    qemu_serial_log = yatest.common.output_path(os.path.basename(rootfs) + "_serial.out")

    # Generate cloud-init config
    # https://cloudinit.readthedocs.io/en/stable/topics/modules.html
    ssh_pubkey_blob = open(ssh_pubkey).read()
    cloud_init = {
        'meta-data': {
        },
        'user-data': {
            'disable_root': False,
            'apt': {
                'preserve_sources_list': True,
            },
            'runcmd': ["echo 127.0.0.1 {} >> /etc/hosts".format(hostname),
                       "echo ::1 {} >> /etc/hosts".format(hostname),
                       "hostname {}".format(hostname)],
            'users': [
                {
                    'name': 'root',
                    'lock_passwd': False,
                    'ssh_pwauth': True,
                    'ssh_authorized_keys': [
                        ssh_pubkey_blob
                    ],
                },
                {
                    'name': ssh_user,
                    'passwd': '$6$KpYsdCFY5wL3a$ZW/QU0/tWFEkiW3F/jvv1JqfwITs.FPBPolOEkyCNvTDUo6FT1hELjqr4wGopEG.lJOv1OFiRXo9AEdp9zhrl0',  # qemupass
                    'lock_passwd': False,
                    'ssh_pwauth': True,
                    'ssh_authorized_keys': [
                        ssh_pubkey_blob
                    ],
                    'sudo': [
                        'ALL=(ALL) NOPASSWD:ALL'
                    ],
                    'shell': '/bin/bash',
                },
            ],
        }
    }
    cloud_init_dir = tempfile.mkdtemp(prefix='cloud-init_')
    for filename, content in cloud_init.items():
        with open(os.path.join(cloud_init_dir, filename), 'w') as f:
            if filename == 'user-data':
                f.write('#cloud-config\n')
                f.write(yaml.dump(content))
    cmd = [
        qemu_kmv,
        "-nodefaults",
        "-enable-kvm",
        "-smp", proc,
        "-m", mem,
        "-cpu", "host",
        "-snapshot",
        "-net", "nic",
        "-net", "user,hostfwd=tcp::{}-:22,ipv6-net=fdc::/64,ipv6-host=fdc::1,hostname={}".format(local_ssh_forwarding_port, hostname),
        "-smbios", "type=1,serial=ds=nocloud;i={};l={}".format(hostname, hostname),
        "-device", "virtio-rng-pci",
        "-serial", "file:{}".format(qemu_serial_log),
        "-nographic",
        "-drive", "file={},if=none,id=drive0,cache=unsafe".format(rootfs),
        "-device", "virtio-blk,drive=drive0",
        "-drive", "file=fat:" + cloud_init_dir + ",if=virtio,file.label=cidata,readonly=on"
    ]
    # Place custom options before virtfs so custom devices will have predictable pci buses
    cmd += qemu_options
    for tag, path in _get_mount_paths():
        cmd += ["-virtfs", "local,path={path},mount_tag={tag},security_model=none".format(tag=tag, path=path)]

    if kernel:
        cmd += ["-kernel", kernel]
        if initrd:
            cmd += ["-initrd", initrd]
        if kcmdline:
            cmd += ["-append", kcmdline]
    res = yatest.common.execute(cmd, wait=False)
    current_user = getpass.getuser()
    _wait_ssh(local_ssh_forwarding_port, ssh_user, ssh_key)
    _prepare_test_environment(local_ssh_forwarding_port, ssh_user, ssh_key)
    library.python.testing.recipe.set_env("QEMU_KVM_PID", str(res.process.pid))
    logger.info("%s", " ".join(_get_ssh_command("sudo su {} -c /run_test.sh".format(current_user), local_ssh_forwarding_port, ssh_user, ssh_key)))
    library.python.testing.recipe.set_env(
        "TEST_COMMAND_WRAPPER",
        " ".join(_get_ssh_command("sudo /run_test.sh", local_ssh_forwarding_port, ssh_user, ssh_key)),
    )


def stop(argv):
    if "QEMU_KVM_PID" in os.environ:
        pid = os.environ["QEMU_KVM_PID"]
        logger.info("will kill process with pid `%s`", pid)
        try:
            os.kill(int(pid), signal.SIGTERM)
        except OSError:
            logger.exception("While killing pid `%s`", pid)
            raise


def _parse_args(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--kernel", help="Path to kernel image (Arcadia related)")
    parser.add_argument("--kcmdline", help="Path to kernel cmdline config (Arcadia related)")
    parser.add_argument("--initrd", help="Path to initrd image (Arcadia related)")
    parser.add_argument("--rootfs", help="Path to rootfs image (Arcadia related)")
    parser.add_argument("--mem", help="Amount of memory for running VM (default is 2G)")
    parser.add_argument("--proc", help="Amount of processors for running VM (default is 2)")
    parser.add_argument("--ssh-key", help="Path to ssh key to access VM (Arcadia related)")
    parser.add_argument("--ssh-pubkey", help="Path to public ssh key to access VM (Arcadia related)")
    parser.add_argument("--ssh-user", help="ssh user name to access VM")
    parser.add_argument("--use-as-test-host", help="Path to rootfs image (Arcadia related)", action="store_true")
    parser.add_argument("--qemu-options", help="Custom options for qemu")
    return parser.parse_args(argv)


def _get_qemu_kvm():
    qemu_kvm = yatest.common.build_path("infra/qemu/bin/qemu/bin/qemu-system-x86_64")
    if not os.path.exists(qemu_kvm):
        raise QemuKvmRecipeException("cannot find qemu-kvm binary by build_path '{}'".format(qemu_kvm))
    return qemu_kvm


def _get_rootfs(args):
    if args.rootfs == "$QEMU_ROOTFS":
        raise QemuKvmRecipeException("Rootfs image is not set for the recipe, please, foolow the docs to know how to fix it")
    rootfs = yatest.common.build_path(args.rootfs)
    if not os.path.exists(rootfs):
        raise QemuKvmRecipeException("Cannot find rootfs image by '{}'".format(rootfs))
    return rootfs


def _get_kernel(args):
    if args.kernel == "$QEMU_KERNEL":
        return None
    kernel = yatest.common.build_path(args.kernel)
    if not os.path.exists(kernel):
        raise QemuKvmRecipeException("Cannot find kernel image by '{}'".format(kernel))
    return kernel


def _get_kcmdline(args):
    if not args.kcmdline or args.kcmdline == "$QEMU_KCMDLINE":
        return None
    kcmdline = yatest.common.build_path(args.kcmdline)
    if not os.path.exists(kcmdline):
        raise QemuKvmRecipeException("Cannot find kcmdline image by '{}'".format(kcmdline))
    return open(kcmdline).read().strip()


def _get_initrd(args):
    if not args.initrd or args.initrd == "$QEMU_INITRD":
        return None
    initrd = yatest.common.build_path(args.initrd)
    if not os.path.exists(initrd):
        raise QemuKvmRecipeException("Cannot find initrd image by '{}'".format(initrd))
    return initrd


def _get_vm_mem(args):
    if args.mem == "$QEMU_MEM":
        return "2G"
    return args.mem


def _get_vm_proc(args):
    if args.proc == "$QEMU_PROC":
        return "2"
    return args.proc


def _get_qemu_options(args):
    if args.qemu_options == "$QEMU_OPTIONS":
        return []
    return args.qemu_options.split()


def _get_ssh_key(args):
    if args.ssh_key == "$QEMU_SSH_KEY":
        raise QemuKvmRecipeException("ssh key is not set for the recipe, please, foolow the docs to know how to fix it")
    ssh_key = yatest.common.build_path(args.ssh_key)
    if not os.path.exists(ssh_key):
        ssh_key = yatest.common.source_path(args.ssh_key)
    if not os.path.exists(ssh_key):
        raise QemuKvmRecipeException("Cannot find ssh key in either build or source root by '{}'".format(args.ssh_key))
    new_ssh_key = yatest.common.work_path(os.path.basename(ssh_key))
    fs.copy_file(ssh_key, new_ssh_key)
    os.chmod(new_ssh_key, 0600)
    return new_ssh_key


def _get_ssh_pubkey(args):
    if args.ssh_pubkey == "$QEMU_SSH_PUBKEY":
        raise QemuKvmRecipeException("ssh public key is not set for the recipe, please, foolow the docs to know how to fix it")
    ssh_pubkey = yatest.common.build_path(args.ssh_pubkey)
    if not os.path.exists(ssh_pubkey):
        ssh_pubkey = yatest.common.source_path(args.ssh_pubkey)
    if not os.path.exists(ssh_pubkey):
        raise QemuKvmRecipeException("Cannot find ssh public key in either build or source root by '{}'".format(args.ssh_pubkey))
    new_ssh_pubkey = yatest.common.work_path(os.path.basename(ssh_pubkey))
    fs.copy_file(ssh_pubkey, new_ssh_pubkey)
    os.chmod(new_ssh_pubkey, 0600)
    return new_ssh_pubkey


def _get_mount_paths():
    test_tool_dir = os.path.dirname(yatest.common.runtime.context.test_tool_path)

    if 'ASAN_SYMBOLIZER_PATH' in os.environ:
        toolchain = os.path.dirname(os.path.dirname(os.environ['ASAN_SYMBOLIZER_PATH']))
    mounts = [("source_path", yatest.common.source_path()),  # need to mound original source root as test environment has links into it
              ("build_path", yatest.common.build_path()),
              ("test_tool", test_tool_dir),
              ("toolchain", toolchain)]
    if yatest.common.ram_drive_path():
        mounts.append(tuple(("tmpfs_path", yatest.common.ram_drive_path())))
    return mounts


def _prepare_test_environment(port, user, key):
    for tag, path in _get_mount_paths():
        _ssh("sudo mkdir -p {path}".format(path=path), port, user, key)
        _ssh("sudo mount -t 9p {tag} {path} -o trans=virtio,version=9p2000.L,cache=loose,rw".format(tag=tag, path=path), port, user, key)
        # the code below fixes situation where /home is a link to /place/home and source and build roots can come
        # in both ways, e.g. build root can be in a form of /place/home/... and source root /home/...
        # to fix it, we create links if the real path is different from the given path
        real_path = os.path.realpath(path)
        if real_path != path:
            _ssh("sudo mkdir -p {} && sudo ln -s {} {}".format(os.path.dirname(real_path), path, real_path), port, user, key)
    # Workaround for https://st.yandex-team.ru/DISKMAN-63
    if "TMPDIR" in os.environ:
        _ssh("sudo mkdir -m a=rwx -p {}".format(os.environ['TMPDIR']), port, user, key)
    vm_env = os.environ
    vm_env['TEST_ENV_WRAPPER'] = ' '.join(_get_ssh_command("", port, user, key))
    library.python.testing.recipe.set_env("TEST_ENV_WRAPPER", vm_env['TEST_ENV_WRAPPER'])
    run_test_sh = " ".join("{}={}".format(k, pipes.quote(v)) for k, v in vm_env.iteritems()) + ' "$@"'
    run_test_sh += "\nexit_code=$?"
    run_test_sh += "\nsync"
    run_test_sh += "\nexit $exit_code"
    _ssh("cat <<'RUN-TEST' | sudo tee -a /run_test.sh && sudo chown {} /run_test.sh && sudo chmod +x /run_test.sh\n#!/bin/bash\n{}\nRUN-TEST".format(user, run_test_sh), port, user, key)
    _add_ipv6_def_route(port, user, key)


def _get_ssh_user(args):
    if args.ssh_user == "$QEMU_SSH_USER":
        raise QemuKvmRecipeException("ssh user is not set for the recipe, please, foolow the docs to know how to fix it")
    return args.ssh_user


def _get_ssh_bin():
    return "/usr/bin/ssh"


@retrying.retry(stop_max_delay=450000, wait_fixed=400)
def _wait_ssh(port, user, key):
    _ssh("exit 0", port, user, key)


@retrying.retry(stop_max_delay=90000, wait_fixed=500)
def _add_ipv6_def_route(port, user, key):
    _ssh("ip -6 route add default via fdc::1 metric 99", port, "root", key)


def _get_ssh_command(command, port, user, key):
    return [_get_ssh_bin(), "-n", "-q", "-F", os.devnull,
            "-o", "StrictHostKeyChecking=no",
            "-o", "UserKnownHostsFile=" + os.devnull,
            "-o", "ConnectTimeout=10",
            "-o", "ServerAliveInterval=10",
            "-o", "ServerAliveCountMax=10",
            "-i", key, "127.0.0.1", "-l", user, "-p", str(port), command]


def _ssh(command, port, user, key):
    yatest.common.execute(_get_ssh_command(command, port, user, key))
