import os
import signal

from ydb.tests.library.harness.daemon import Daemon
from ydb.tests.library.harness.kikimr_runner import get_unique_path_for_current_test, ensure_path_exists
import ydb.tests.library.common.yatest_common as yatest_common

DEFAULT_CONFIG_TEMPLATE = """
Port: {port}
CertFile: "{cert_file}"
PrivateKeyFile: "{private_key_file}"
IamTokenToUserId: <
    key: "TestToken"
    value: "UserAllowAll"
>
IamTokenToUserId: <
    key: "TestTokenDisksOnly"
    value: "UserAllowDisksOnly"
>
Rules: <
    IdPattern: "UserAllowAll"
    PermissionPattern: ".*"
>
Rules: <
    IdPattern: "UserAllowDisksOnly"
    PermissionPattern: "disk-manager\\\\.disks\\\\..*"
>
"""

PID_FILE_NAME = "disk_manager_recipe_access_service.pid"


class AccessServiceServer(Daemon):

    def __init__(self, config_file, working_dir):
        command = [yatest_common.binary_path(
            "cloud/disk_manager/test/mocks/accessservice/accessservice-mock")]
        command += [
            "--config", config_file
        ]
        super(AccessServiceServer, self).__init__(
            command=command,
            cwd=working_dir,
            timeout=180)


class AccessServiceLauncher:

    def __init__(self, cert_file, private_key_file):
        self.__port_manager = yatest_common.PortManager()
        self.__port = self.__port_manager.get_port()

        working_dir = get_unique_path_for_current_test(
            output_path=yatest_common.output_path(),
            sub_folder=""
        )
        ensure_path_exists(working_dir)

        config_file = os.path.join(working_dir, 'access_service_config.txt')
        with open(config_file, "w") as f:
            f.write(DEFAULT_CONFIG_TEMPLATE.format(
                port=self.__port,
                cert_file=cert_file,
                private_key_file=private_key_file
            ))
        self.__daemon = AccessServiceServer(config_file, working_dir)

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
        return self.__port
