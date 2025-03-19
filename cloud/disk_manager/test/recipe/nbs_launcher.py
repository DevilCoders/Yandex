import os
import signal

import ydb.tests.library.common.yatest_common as yatest_common

from cloud.blockstore.config.discovery_pb2 import TDiscoveryServiceConfig
from cloud.blockstore.config.server_pb2 import TServerConfig, TServerAppConfig, TKikimrServiceConfig
from cloud.blockstore.config.storage_pb2 import TStorageServiceConfig
from cloud.blockstore.tests.python.lib.nbs_runner import LocalNbs
from cloud.blockstore.tests.python.lib.test_base import thread_count, wait_for_nbs_server

PID_FILE_NAME = "disk_manager_recipe_nbs.pid"


class NbsLauncher:

    def __init__(self, kikimr_port, domains_txt, dynamic_storage_pools, root_certs_file, cert_file, cert_key_file, kikimr_binary_path, nbs_binary_path):
        self.__port_manager = yatest_common.PortManager()
        nbs_port = self.__port_manager.get_port()
        nbs_secure_port = self.__port_manager.get_port()

        server_app_config = TServerAppConfig()
        server_app_config.ServerConfig.CopyFrom(TServerConfig())
        server_app_config.ServerConfig.ThreadsCount = thread_count()
        server_app_config.ServerConfig.StrictContractValidation = False
        server_app_config.KikimrServiceConfig.CopyFrom(TKikimrServiceConfig())
        server_app_config.ServerConfig.RootCertsFile = root_certs_file

        cert = server_app_config.ServerConfig.Certs.add()
        cert.CertFile = cert_file
        cert.CertPrivateKeyFile = cert_key_file

        storage_config_patch = TStorageServiceConfig()
        storage_config_patch.MultipleMountAllowed = True
        storage_config_patch.MountRequired = True
        storage_config_patch.InactiveClientsTimeout = 10000
        storage_config_patch.LogicalUsedBlocksCalculationEnabled = True

        instance_list_file = os.path.join(yatest_common.output_path(), "static_instances.txt")
        with open(instance_list_file, "w") as f:
            print("localhost\t%s\t%s" % (nbs_port, nbs_secure_port), file=f)
        discovery_config = TDiscoveryServiceConfig()
        discovery_config.InstanceListFile = instance_list_file

        self.__nbs = LocalNbs(
            kikimr_port,
            domains_txt,
            server_app_config=server_app_config,
            storage_config_patches=[storage_config_patch],
            discovery_config=discovery_config,
            enable_tls=True,
            dynamic_storage_pools=dynamic_storage_pools,
            nbs_secure_port=nbs_secure_port,
            nbs_port=nbs_port,
            kikimr_binary_path=kikimr_binary_path,
            nbs_binary_path=nbs_binary_path,
            grpc_trace=False)

    def start(self):
        self.__nbs.start()
        wait_for_nbs_server(self.__nbs.nbs_port)
        with open(PID_FILE_NAME, "w") as f:
            f.write(str(self.__nbs.pid))

    @staticmethod
    def stop():
        if not os.path.exists(PID_FILE_NAME):
            return
        with open(PID_FILE_NAME) as f:
            pid = int(f.read())
            os.kill(pid, signal.SIGTERM)

    @property
    def port(self):
        return self.__nbs.nbs_secure_port
