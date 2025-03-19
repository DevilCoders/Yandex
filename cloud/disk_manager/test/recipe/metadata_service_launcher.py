import os
import signal

from ydb.tests.library.harness.daemon import Daemon
from ydb.tests.library.harness.kikimr_runner import get_unique_path_for_current_test, ensure_path_exists
import ydb.tests.library.common.yatest_common as yatest_common

DEFAULT_CONFIG_TEMPLATE = """
Port: {port}
AccessToken: "TestToken"
"""

PID_FILE_NAME = "disk_manager_recipe_metadata_service.pid"


class MetadataServiceServer(Daemon):

    def __init__(self, config_file, working_dir):
        command = [yatest_common.binary_path(
            "cloud/disk_manager/test/mocks/metadata/metadata-mock")]
        command += [
            "--config", config_file
        ]
        super(MetadataServiceServer, self).__init__(
            command=command,
            cwd=working_dir,
            timeout=180)


class MetadataServiceLauncher:

    def __init__(self):
        self.__port_manager = yatest_common.PortManager()
        self.__port = self.__port_manager.get_port()

        working_dir = get_unique_path_for_current_test(
            output_path=yatest_common.output_path(),
            sub_folder=""
        )
        ensure_path_exists(working_dir)

        config_file = os.path.join(working_dir, 'metadata_service_config.txt')
        with open(config_file, "w") as f:
            f.write(DEFAULT_CONFIG_TEMPLATE.format(
                port=self.__port,
            ))
        self.__daemon = MetadataServiceServer(config_file, working_dir)

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
    def url(self):
        return "http://localhost:{port}".format(port=self.__port)
