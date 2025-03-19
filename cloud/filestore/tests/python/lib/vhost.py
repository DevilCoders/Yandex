import logging
import os
import retrying

import yatest.common as common

import cloud.filestore.public.sdk.python.protos as protos
import ydb.tests.library.common.yatest_common as yatest_common

from cloud.filestore.public.sdk.python.client.grpc_client import CreateGrpcEndpointClient
from cloud.filestore.tests.python.lib.common import daemon_log_files, is_grpc_error
from google.protobuf.text_format import MessageToString
from ydb.tests.library.harness.daemon import Daemon
from ydb.tests.library.harness.kikimr_runner import get_unique_path_for_current_test, ensure_path_exists

logger = logging.getLogger(__name__)


class NfsVhost(Daemon):
    def __init__(self, binary_path, server_port=None,
                 cwd=".", verbose=False, service=None,
                 vhost_app_config=None,
                 restart_interval=None,
                 restart_flag=None):

        self.__binary_path = binary_path

        self.__port_manager = yatest_common.PortManager()
        self.__port = self.__port_manager.get_port()
        self.__mon_port = self.__port_manager.get_port()

        self.__server_port = os.getenv("NFS_SERVER_PORT")

        command = [
            self.__binary_path,
            "--server-port", str(self.__port),
            "--connect-port", str(self.__server_port),
            "--mon-port", str(self.__mon_port),
        ]

        if service is not None:
            command += [
                "--service", service,
            ]

        if vhost_app_config:
            config_file_path = os.path.join(self.config_path(), "vhost.txt")
            with open(config_file_path, "w") as config_file:
                config_file.write(MessageToString(vhost_app_config))

            command += [
                "--vhost-file", config_file_path,
            ]

        if verbose:
            command += [
                "--verbose", "debug",
            ]

        if restart_interval:
            launcher_path = common.binary_path(
                "cloud/storage/core/tools/testing/unstable-process/storage-unstable-process")

            command = [
                launcher_path,
                "--ping-port", str(self.__mon_port),
                "--ping-timeout", str(2),
                "--restart-interval", str(restart_interval),
                "--cmdline", " ".join(command),
            ] + ["--allow-restart-flag", restart_flag] if restart_flag else []

        super(NfsVhost, self).__init__(
            command=command,
            cwd=cwd,
            timeout=180,
            **daemon_log_files(prefix="filestore-vhost", cwd=cwd))

    @property
    def pid(self):
        return super(NfsVhost, self).daemon.process.pid

    @property
    def port(self):
        return self.__port

    def config_path(self):
        config_path = get_unique_path_for_current_test(
            output_path=yatest_common.output_path(),
            sub_folder="nfs_configs"
        )
        ensure_path_exists(config_path)
        return config_path


@retrying.retry(stop_max_delay=60000, wait_fixed=1000, retry_on_exception=is_grpc_error)
def wait_for_nfs_vhost(daemon, port):
    '''
    Ping NFS vhost with delay between attempts to ensure
    it is running and listening by the moment the actual test execution begins
    '''
    if not daemon.is_alive():
        raise RuntimeError("vhost server is dead")

    with CreateGrpcEndpointClient(str("localhost:%d" % port)) as grpc_client:
        grpc_client.ping(protos.TPingRequest())
