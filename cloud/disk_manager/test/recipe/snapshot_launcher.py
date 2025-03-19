import logging
import os
import signal
import tempfile
import time
import uuid

from yatest.common import process

from ydb.tests.library.harness.daemon import Daemon
from ydb.tests.library.harness.kikimr_runner import get_unique_path_for_current_test, ensure_path_exists
import ydb.tests.library.common.yatest_common as yatest_common

logger = logging.getLogger()


DEFAULT_CONFIG_TEMPLATE = """
[General]
Tmpdir = "{tmp_dir}"
Storage = "kikimr+grpc"
CacheSize = 1024
DummyFsRoot = ""
StartSplit = 10
ClearChunkBatchSize = 50

[S3]
Dummy = true

[Kikimr]
[Kikimr.default]
DBHost = "localhost:{kikimr_port}"
Root = "/Root/legacy_snapshot"
DBName = "/Root"

[Nbs]
[Nbs.zone]
Host = "localhost:{nbs_port}"
SupportsChangedBlocks = true
SSL = true
# TODO: Only RootCertsFile needs to be available
KeyFile = "{cert_key_file}"
CertFile = "{cert_file}"
RootCertsFile = "{root_certs_file}"
[Nbs.other]
Host = "localhost:{nbs_port}"
SupportsChangedBlocks = true
SSL = true
# TODO: Only RootCertsFile needs to be available
KeyFile = "{cert_key_file}"
CertFile = "{cert_file}"
RootCertsFile = "{root_certs_file}"

[Nbd]
DockerImage="snapshot_qemu_nbd"
SecurityOptions=["no-new-privileges"]

[Server]
SSL = true
KeyFile = "{cert_key_file}"
CertFile = "{cert_file}"
GRPCEndpoint = "tcp4://[::]:{grpc_port}"
HTTPEndpoint = "tcp6://[::]:{http_port}"

[Logging]
Level = "debug"
Output = "stdout"
EnableKikimr = true
EnableSpans = true
EnableGrpc = true
EnableNbs = true

[GC]
Enabled = true
BatchSize = 100
Interval = "24h"
FailedCreation = "1s"
FailedConversion = "1s"
FailedDeletion = "1s"
Tombstone = "1h"

[Performance]
MoveWorkers = 16
ConvertWorkers = 16
GCWorkers = 4
ClearChunkWorkers = 16
VerifyWorkers = 64

[QemuDockerProxy]
SocketPath = "{proxy_unix_socket}"
HostForDocker = "127.0.0.1:1234"
IDLengthBytes = 20
"""

PID_FILE_NAME = "disk_manager_recipe_snapshot.pid"


def create_unix_socket():
    file_name = str(uuid.uuid4())
    unix_socket_path = tempfile.gettempdir() + "/" + file_name
    # Chop path because it's limited in some environments.
    if len(unix_socket_path) > 100:
        to_chop = min(len(unix_socket_path) - 100, len(file_name) - 1)
        unix_socket_path = unix_socket_path[:-to_chop]
    assert len(unix_socket_path) <= 100

    if os.path.exists(unix_socket_path):
        os.remove(unix_socket_path)

    os.mknod(unix_socket_path)

    return unix_socket_path


class SnapshotServer(Daemon):

    def __init__(self, config_file, working_dir):
        command = [yatest_common.binary_path(
            "cloud/compute/snapshot/cmd/yc-snapshot/yc-snapshot")]
        command += [
            "-config", config_file
        ]
        super(SnapshotServer, self).__init__(
            command=command,
            cwd=working_dir,
            timeout=180)


class SnapshotLauncher:

    def __init__(self, kikimr_port, nbs_port, root_certs_file, cert_file, cert_key_file):
        self.__port_manager = yatest_common.PortManager()
        self.__grpc_port = self.__port_manager.get_port()
        self.__http_port = self.__port_manager.get_port()
        self.__proxy_unix_socket = create_unix_socket()

        working_dir = get_unique_path_for_current_test(
            output_path=yatest_common.output_path(),
            sub_folder=""
        )
        ensure_path_exists(working_dir)

        config_file = os.path.join(working_dir, 'snapshot_config.toml')
        with open(config_file, "w") as f:
            f.write(DEFAULT_CONFIG_TEMPLATE.format(
                tmp_dir=working_dir,
                kikimr_port=kikimr_port,
                nbs_port=nbs_port,
                root_certs_file=root_certs_file,
                cert_file=cert_file,
                cert_key_file=cert_key_file,
                grpc_port=self.__grpc_port,
                http_port=self.__http_port,
                proxy_unix_socket=self.__proxy_unix_socket
            ))

        init_database_command = [
            yatest_common.binary_path(
                "cloud/compute/snapshot/cmd/yc-snapshot-populate-database/yc-snapshot-populate-database"
            ),
            "-config",
            config_file,
        ]

        attempts_left = 20
        while True:
            try:
                process.execute(init_database_command)
                break
            except yatest_common.ExecutionError as e:
                logger.error("init_database_command=%s failed with error=%s", init_database_command, e)

                attempts_left -= 1
                if attempts_left == 0:
                    raise e

                time.sleep(1)
                continue

        self.__daemon = SnapshotServer(config_file, working_dir)

    def start(self):
        self.__daemon.start()
        with open(PID_FILE_NAME, "w") as f:
            f.write(str(self.__daemon.daemon.process.pid))

    @staticmethod
    def stop():
        if not os.path.exists(PID_FILE_NAME):
            return
        with open(PID_FILE_NAME) as f:
            pid = int(f.read())
            os.kill(pid, signal.SIGTERM)

    @property
    def port(self):
        return self.__grpc_port
