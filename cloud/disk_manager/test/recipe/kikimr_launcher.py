import os
import signal

from ydb.tests.library.harness.kikimr_cluster import kikimr_cluster_factory
from ydb.tests.library.harness.kikimr_config import KikimrConfigGenerator


PID_FILE_NAME = "disk_manager_recipe_kikimr.pid"


class KikimrLauncher:

    def __init__(self, kikimr_binary_path):
        dynamic_storage_pools = [
            dict(name="dynamic_storage_pool:1", kind="rot", pdisk_user_kind=0),
            dict(name="dynamic_storage_pool:2", kind="ssd", pdisk_user_kind=0),
            dict(name="dynamic_storage_pool:3", kind="rotencrypted", pdisk_user_kind=0),
            dict(name="dynamic_storage_pool:4", kind="ssdencrypted", pdisk_user_kind=0),
        ]
        configurator = KikimrConfigGenerator(
            binary_path=kikimr_binary_path,
            erasure=None,
            has_cluster_uuid=False,
            static_pdisk_size=64 * 2**30,
            dynamic_storage_pools=dynamic_storage_pools,
            use_log_files=False,
            enable_public_api_external_blobs=True)

        self.__cluster = kikimr_cluster_factory(configurator=configurator)
        self.__dynamic_storage_pools = dynamic_storage_pools

    def start(self):
        self.__cluster.start()
        with open(PID_FILE_NAME, "w") as f:
            f.write(str(list(self.__cluster.nodes.values())[0].pid))

    @staticmethod
    def stop():
        if not os.path.exists(PID_FILE_NAME):
            return
        with open(PID_FILE_NAME) as f:
            pid = int(f.read())
            os.kill(pid, signal.SIGTERM)

    @property
    def port(self):
        return list(self.__cluster.nodes.values())[0].port

    @property
    def domains_txt(self):
        return self.__cluster.config.domains_txt

    @property
    def names_txt(self):
        return self.__cluster.config.names_txt

    @property
    def dynamic_storage_pools(self):
        return self.__dynamic_storage_pools
