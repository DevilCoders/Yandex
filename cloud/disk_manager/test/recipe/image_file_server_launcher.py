import binascii
import os
import signal

from ydb.tests.library.harness.daemon import Daemon
from ydb.tests.library.harness.kikimr_runner import get_unique_path_for_current_test, ensure_path_exists
import ydb.tests.library.common.yatest_common as yatest_common

PID_FILE_NAME = "disk_manager_recipe_image_file_server.pid"


class ImageFileServer(Daemon):

    def __init__(self, port, working_dir, image_file_path):
        command = [yatest_common.binary_path(
            "cloud/disk_manager/test/images/server/server")]
        command += [
            "start",
            "--image-file", image_file_path,
            "--port", str(port),
        ]
        super(ImageFileServer, self).__init__(
            command=command,
            cwd=working_dir,
            timeout=180)


class ImageFileServerLauncher:

    def __init__(self, image_file_path):
        self.__image_file_path = image_file_path

        self.__port_manager = yatest_common.PortManager()
        self.__port = self.__port_manager.get_port()

        working_dir = get_unique_path_for_current_test(
            output_path=yatest_common.output_path(),
            sub_folder=""
        )
        ensure_path_exists(working_dir)

        self.__daemon = ImageFileServer(self.__port, working_dir, self.__image_file_path)

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

    @property
    def image_file_size(self):
        return os.path.getsize(self.__image_file_path)

    @property
    def image_file_crc32(self):
        content = open(self.__image_file_path, 'rb').read()
        crc32 = (binascii.crc32(content) & 0xFFFFFFFF)
        return crc32
