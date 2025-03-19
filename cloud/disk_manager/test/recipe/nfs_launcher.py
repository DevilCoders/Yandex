import os
import signal

from cloud.filestore.tests.python.lib.server import NfsServer, wait_for_nfs_server
from cloud.filestore.tests.python.lib.server_config import NfsServerConfigGenerator

PID_FILE_NAME = "disk_manager_recipe_nfs.pid"


class NfsLauncher:

    def __init__(self, kikimr_port, domains_txt, names_txt, nfs_binary_path):
        self.__nfs_configurator = NfsServerConfigGenerator(
            binary_path=nfs_binary_path,
            # FIXME: use kikimr service, resolve tenant config issues
            service_type="local",
            verbose=True,
            domains_txt=domains_txt,
            names_txt=names_txt,
            kikimr_port=kikimr_port)

        self.__nfs_server = NfsServer(configurator=self.__nfs_configurator)

    def start(self):
        self.__nfs_server.start()
        wait_for_nfs_server(self.__nfs_server, self.__nfs_configurator.port)

        with open(PID_FILE_NAME, "w") as f:
            f.write(str(self.__nfs_server.pid))

    @staticmethod
    def stop():
        if not os.path.exists(PID_FILE_NAME):
            return
        with open(PID_FILE_NAME) as f:
            pid = int(f.read())
            os.kill(pid, signal.SIGTERM)

    @property
    def port(self):
        return self.__nfs_configurator.port
