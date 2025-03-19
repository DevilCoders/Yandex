import argparse
import getpass
import json
import logging
import os
import sys

from yalibrary import find_root

from subprocess import PIPE, run as exec

import cloud.blockstore.tools.cms.lib.pssh as libpssh


def prepare_logging(args):
    log_level = logging.ERROR

    if args.silent:
        log_level = logging.INFO
    elif args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))

    logging.basicConfig(
        stream=sys.stderr,
        level=log_level,
        format="[%(levelname)s] [%(asctime)s] %(message)s")


class YcpImpl:

    def __init__(self, profile):
        self.__profile = profile
        self.__bin = 'ycp'

    def invoke(self, cmd, env=None):
        logging.debug(f"ycp.invoke({self.__profile}, {cmd}) ...")

        p = exec([
            self.__bin,
            '--format', 'json',
            '--profile', self.__profile,
        ] + cmd, stdout=PIPE, env=env)

        logging.debug(f"ycp.invoke({cmd}): {p.returncode}")

        assert(p.returncode == 0)

        r = None
        if p.stdout is not None:
            r = json.loads(p.stdout)

        logging.debug(f"ycp.invoke({cmd}): {r}")

        return r


class YcpMock:

    def __init__(self) -> None:
        pass

    def invoke(self, cmd):
        logging.info(f"ycp.invoke({cmd}) ...")
        if cmd[:3] == ['compute', 'instance', 'get']:
            return {
                'id': cmd[3],
                'status': "RUNNING",
                'zone_id': 'ru-central1-b',
            }

        if cmd[:3] == ['compute', 'disk', 'get']:
            return {
                'id': cmd[3],
                'size': '99857989632',
                'block_size': '4096',
                'status': 'READY',
            }

        if cmd[:2] == ['iam', 'create-token']:
            return {"iam_token": "IAM-TOKEN", "expires_at": "2021-10-06T22:24:19.469Z"}

        return None


class Ycp:

    def __init__(self, impl):
        self.__impl = impl

    def get_instance(self, instance_id):
        return self.__impl.invoke(['compute', 'instance', 'get', instance_id])

    def stop_instance(self, instance_id):
        return self.__impl.invoke(['compute', 'instance', 'stop', instance_id])

    def start_instance(self, instance_id):
        return self.__impl.invoke(['compute', 'instance', 'start', instance_id])

    def create_iam_token(self):
        env = os.environ.copy()
        env.pop("YC_IAM_TOKEN")
        return self.__impl.invoke(['iam', 'create-token'], env=env)["iam_token"]


class Pssh:

    def __init__(self, bin_path):
        self.__impl = libpssh.Pssh(exe=bin_path)
        self.__username = getpass.getuser()

    def scp(self, src, dst, compute_node):
        logging.debug(f"[pssh.scp] src={src} dst={dst} node={compute_node} ...")
        r = self.__impl.scp_to(compute_node, src, f'/home/{self.__username}/{dst}')
        assert(r)

    def run(self, cmd, compute_node):
        return self.__impl.run(cmd, compute_node, attempts=1)

    def resolve(self, query):
        return self.__impl.resolve(query)


class PsshMock:

    def scp(self, src, dst, compute_node):
        logging.info(f"[pssh.run] {src} to {dst} on {compute_node}")

    def run(self, cmd, compute_node):
        logging.info(f"[pssh.run] {cmd} on {compute_node}")
        return 'SUCCESS'

    def resolve(self, query):
        logging.info(f"[pssh.resolve] {query}")

        return 'myt1-ct7-5.cloud.yandex.net'


class Env:

    def __init__(self, args) -> None:
        self.__args = args
        self.__compute_nodes = set()

        if args.dry_run:
            self.__pssh = PsshMock()
            self.__ycp = Ycp(YcpMock())
        else:
            self.__pssh = Pssh(args.pssh_path)
            self.__ycp = Ycp(YcpImpl(args.ycp_profile))

    def get_instance(self, instance_id):
        return self.__ycp.get_instance(instance_id)

    def stop_instance(self, instance_id):
        return self.__ycp.stop_instance(instance_id)

    def start_instance(self, instance_id):
        return self.__ycp.start_instance(instance_id)

    def resolve_compute_node(self, zone_id):
        query = ''

        if self.__args.cluster == "hw-nbs-stable-lab":
            query = "C@cloud_hw-nbs-stable-lab_nbs_sas[1]"
        else:
            m = {
                "ru-central1-a": "vla",
                "ru-central1-b": "sas",
                "ru-central1-c": "myt"
            }

            z = m[zone_id]

            query = f"C@cloud_{self.__args.cluster}_nbs_{z}[1]"

        return self.__pssh.resolve(query).strip()

    def prepare_compute_node(self, compute_node):
        if compute_node in self.__compute_nodes:
            return

        r = self.__pssh.run("sudo modprobe nbd", compute_node)
        logging.info(f"Enable NBD: {r}")

        if not self.__args.no_auth:
            iam_token = self.__ycp.create_iam_token()
            self.__pssh.run(
                " && ".join([
                    "mkdir -p .nbs-client",
                    "echo 'ClientConfig { Host: \"localhost\" SecurePort: 9768 }' > .nbs-client/config.txt",
                    f"echo '{iam_token}' > .nbs-client/iam-token"]),
                compute_node)

        if not self.__args.skip_tools:
            arcadia_root = find_root.detect_root(os.getcwd())
            assert(arcadia_root)
            blockstore_path = os.path.join(arcadia_root, 'cloud/blockstore')

            if self.__args.remote_folder != '':
                self.__pssh.run(f'mkdir -p {self.__args.remote_folder}', compute_node)

            tools = [
                os.path.join(
                    blockstore_path, 'support/CLOUDINC-1800/tools',
                    name, name) for name in [
                        'cleanup-fs',
                        'cleanup-ext4-meta',
                        'cleanup-xfs',
                        'copy_dev']
            ]

            if self.__args.xfsprogs_path:
                for p in ['db/xfs_db']:  # , 'repair/xfs_repair']:
                    tools.append(os.path.join(self.__args.xfsprogs_path, p))

            if self.__args.e2fsprogs_path:
                for p in ['e2fsck/e2fsck', 'misc/dumpe2fs']:
                    tools.append(os.path.join(self.__args.e2fsprogs_path, p))

            for path in tools:
                self.__pssh.scp(path, self.__args.remote_folder, compute_node)

        self.__compute_nodes.add(compute_node)

    def cleanup_fs(self, disk_id, compute_node):
        bkp = f'--backup-volume {self.__args.backup_volume}'

        auth = ''
        if self.__args.no_auth:
            auth = '--no-auth'

        r = self.__pssh.run(
            f"cd {self.__args.remote_folder} && \
                sudo ./cleanup-fs \
                    --disk-id {disk_id} \
                    {auth} {bkp} \
                    --device {self.__args.device} \
                    -vvv",
            compute_node)

        logging.info(f"cleanup-fs: {r}")

        return r and r.strip().endswith("SUCCESS")


def cleanup_vm(env, instance_id, disks):
    vm = env.get_instance(instance_id)

    logging.debug(vm)

    compute_node = env.resolve_compute_node(vm['zone_id'])
    logging.debug(f"compute_node: {compute_node}")
    env.prepare_compute_node(compute_node)

    start_on_exit = vm['status'] == 'RUNNING'

    if start_on_exit:
        env.stop_instance(instance_id)

    for disk in disks:
        if not env.cleanup_fs(disk, compute_node):
            logging.error(f"can't cleanup: {disk}")
            # TODO: confirmation
            sys.exit(1)

    if start_on_exit:
        env.start_instance(instance_id)


def read_tasks(tasks_file):
    tasks = []
    with open(tasks_file) as f:
        for line in f:
            tasks.append(line.strip().split(' '))
    return tasks


def main(args):
    env = Env(args)

    for task in read_tasks(args.tasks):
        cleanup_vm(env, task[0], task[1:])

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--tasks", type=str, required=True)
    parser.add_argument("--cluster", type=str, required=True)
    parser.add_argument("--ycp-profile", type=str, required=True)
    parser.add_argument("--pssh-path", type=str, default="yc-pssh")
    parser.add_argument("--remote-folder", type=str, default="CLOUDINC-1800")

    # tools
    parser.add_argument("--xfsprogs-path", type=str, default="")
    parser.add_argument("--e2fsprogs-path", type=str, default="")

    parser.add_argument("-s", '--silent', help="silent mode", default=0, action='count')
    parser.add_argument("-v", '--verbose', help="verbose mode", default=0, action='count')

    parser.add_argument("--device", type=str, default="/dev/nbd0")
    parser.add_argument("--backup-volume", choices=["none", "copy_dev", "nbs"], default="copy_dev")

    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False)

    parser.add_argument(
        "--no-auth",
        action="store_true",
        default=False)

    parser.add_argument(
        "--skip-tools",
        action="store_true",
        default=False)

    args = parser.parse_args()

    prepare_logging(args)

    sys.exit(main(args))
