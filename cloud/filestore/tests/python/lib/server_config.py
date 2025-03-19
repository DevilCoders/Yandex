import os

import yatest.common as common

import ydb.tests.library.common.yatest_common as yatest_common

from google.protobuf.text_format import MessageToString

from ydb.tests.library.harness.kikimr_runner import \
    get_unique_path_for_current_test, ensure_path_exists

from ydb.core.protos.config_pb2 import \
    TDomainsConfig, TStaticNameserviceConfig, TLogConfig, \
    TActorSystemConfig, TDynamicNameserviceConfig

from cloud.filestore.config.diagnostics_pb2 import TDiagnosticsConfig

from cloud.filestore.config.server_pb2 import \
    TServerAppConfig, TServerConfig, \
    TKikimrServiceConfig, TLocalServiceConfig, TNullServiceConfig

from cloud.filestore.config.storage_pb2 import TStorageConfig

LOG_WARN = 4
LOG_INFO = 6
LOG_DEBUG = 7
LOG_TRACE = 8


class NfsServerConfigGenerator():

    def __init__(self, binary_path, working_dir=None,
                 service_type=None, verbose=False,
                 domains_txt=None, names_txt=None, kikimr_port=0,
                 restart_interval=None):

        self.__binary_path = binary_path

        if working_dir is not None:
            self.__working_dir = working_dir
            self.__configs_dir = os.path.join(working_dir, "nfs_configs")
        else:
            output_path = yatest_common.output_path()
            self.__working_dir = get_unique_path_for_current_test(
                output_path=output_path,
                sub_folder="")
            self.__configs_dir = get_unique_path_for_current_test(
                output_path=output_path,
                sub_folder="nfs_configs")

        ensure_path_exists(self.__working_dir)
        ensure_path_exists(self.__configs_dir)

        self.__service_type = service_type or "null"
        self.__verbose = verbose

        self.__port_manager = yatest_common.PortManager()
        self.__port = self.__port_manager.get_port()
        self.__mon_port = self.__port_manager.get_port()
        self.__ic_port = self.__port_manager.get_port()
        self.__kikimr_port = kikimr_port

        self.__restart_interval=restart_interval

        self.__generate_configs(domains_txt, names_txt)

        self.__output_path = yatest_common.output_path()
        self.__cwd = get_unique_path_for_current_test(
            output_path=self.__output_path,
            sub_folder="")

    @property
    def binary_path(self):
        return self.__binary_path

    @property
    def working_dir(self):
        return self.__working_dir

    @property
    def configs_dir(self):
        return self.__configs_dir

    @property
    def service_type(self):
        return self.__service_type

    @property
    def verbose(self):
        return self.__verbose

    @property
    def port(self):
        return self.__port

    @property
    def mon_port(self):
        return self.__mon_port

    @property
    def ic_port(self):
        return self.__ic_port

    @property
    def domains_txt(self):
        return self.__proto_configs["domains.txt"]

    @property
    def names_txt(self):
        return self.__proto_configs["names.txt"]

    @property
    def log_txt(self):
        return self.__proto_configs["log.txt"]

    @property
    def sys_txt(self):
        return self.__proto_configs["sys.txt"]

    @property
    def storage_txt(self):
        return self.__proto_configs["storage.txt"]

    @property
    def diag_txt(self):
        return self.__proto_configs["diag.txt"]

    @property
    def dyn_ns_txt(self):
        return self.__proto_configs["dyn_ns.txt"]

    def __generate_domains_txt(self, domains_txt):
        config = TDomainsConfig()
        if domains_txt is not None:
            config.CopyFrom(domains_txt)

        # TODO
        return config

    def __generate_names_txt(self, names_txt):
        config = TStaticNameserviceConfig()
        if names_txt is not None:
            config.CopyFrom(names_txt)

        # TODO
        return config

    def __generate_log_txt(self):
        kikimr_services = [
            "BS_NODE",
            "BS_PROXY",
            "BS_PROXY_COLLECT",
            "BS_PROXY_DISCOVER",
            "BS_PROXY_PUT",
            "BS_QUEUE",
            "INTERCONNECT",
            "INTERCONNECT_NETWORK",
            "INTERCONNECT_SESSION",
            "INTERCONNECT_STATUS",
            "LABELS_MAINTAINER",
            "OPS_COMPACT",
            "SCHEME_BOARD_SUBSCRIBER",
            "TABLET_EXECUTOR",
            "TABLET_FLATBOOT",
            "TABLET_MAIN",
            "TABLET_OPS_HOST",
            "TENANT_POOL",
            "TX_PROXY",
        ]

        nfs_services = [
            "NFS_SERVER",
            "NFS_SERVICE",
            "NFS_SERVICE_WORKER",
            "NFS_SS_PROXY",
            "NFS_TABLET",
            "NFS_TABLET_PROXY",
            "NFS_TABLET_WORKER",
        ]

        nfs_level = LOG_DEBUG if self.verbose else LOG_INFO
        kikimr_level = LOG_INFO if self.verbose else LOG_WARN

        config = TLogConfig()
        for service_name in nfs_services:
            config.Entry.add(Component=service_name.encode(), Level=nfs_level)

        for service_name in kikimr_services:
            config.Entry.add(Component=service_name.encode(), Level=kikimr_level)
        config.DefaultLevel = kikimr_level

        return config

    def __generate_sys_txt(self):
        config = TActorSystemConfig()
        config.Scheduler.Resolution = 2048
        config.Scheduler.SpinThreshold = 0
        config.Scheduler.ProgressThreshold = 10000

        config.SysExecutor = 0
        config.Executor.add(
            Type=config.TExecutor.EType.Value("BASIC"),
            Threads=4,
            SpinThreshold=20,
            Name="System"
        )

        config.UserExecutor = 1
        config.Executor.add(
            Type=config.TExecutor.EType.Value("BASIC"),
            Threads=4,
            SpinThreshold=20,
            Name="User"
        )

        config.BatchExecutor = 2
        config.Executor.add(
            Type=config.TExecutor.EType.Value("BASIC"),
            Threads=4,
            SpinThreshold=20,
            Name="Batch"
        )

        config.IoExecutor = 3
        config.Executor.add(
            Type=config.TExecutor.EType.Value("IO"),
            Threads=1,
            Name="IO"
        )

        config.Executor.add(
            Type=config.TExecutor.EType.Value("BASIC"),
            Threads=1,
            SpinThreshold=10,
            Name="IC",
            TimePerMailboxMicroSecs=100
        )
        config.ServiceExecutor.add(
            ServiceName="Interconnect",
            ExecutorId=4
        )

        return config

    def __generate_server_txt(self):
        config = TServerAppConfig()
        config.ServerConfig.CopyFrom(TServerConfig())
        config.ServerConfig.Port = self.__port

        if self.__service_type == "local":
            config.LocalServiceConfig.CopyFrom(TLocalServiceConfig())

            path = common.ram_drive_path()
            if path:
                config.LocalServiceConfig.RootPath = path

        elif self.__service_type == "kikimr":
            config.KikimrServiceConfig.CopyFrom(TKikimrServiceConfig())
        else:
            config.NullServiceConfig.CopyFrom(TNullServiceConfig())

        return config

    def __generate_storage_txt(self):
        config = TStorageConfig()

        config.HDDSystemChannelPoolKind = "hdd"
        config.HDDLogChannelPoolKind = "hdd"
        config.HDDIndexChannelPoolKind = "hdd"
        config.HDDFreshChannelPoolKind = "hdd"
        config.HDDMixedChannelPoolKind = "hdd"

        # FIXME: no ssd in the recipe
        config.SSDSystemChannelPoolKind = "hdd"
        config.SSDLogChannelPoolKind = "hdd"
        config.SSDIndexChannelPoolKind = "hdd"
        config.SSDFreshChannelPoolKind = "hdd"
        config.SSDMixedChannelPoolKind = "hdd"

        config.HybridSystemChannelPoolKind = "hdd"
        config.HybridLogChannelPoolKind = "hdd"
        config.HybridIndexChannelPoolKind = "hdd"
        config.HybridFreshChannelPoolKind = "hdd"
        config.HybridMixedChannelPoolKind = "hdd"

        return config

    def __generate_diag_txt(self):
        config = TDiagnosticsConfig()

        config.ProfileLogTimeThreshold = 100
        config.SamplingRate = 10000

        return config

    def __generate_dyn_ns_txt(self):
        config = TDynamicNameserviceConfig()
        config.MaxStaticNodeId = 1000
        config.MaxDynamicNodeId = 1064
        return config

    def __generate_configs(self, domains_txt, names_txt):
        self.__proto_configs = {
            "server.txt": self.__generate_server_txt(),
        }

        if self.__service_type == "kikimr":
            self.__proto_configs.update({
                "domains.txt": self.__generate_domains_txt(domains_txt),
                "names.txt": self.__generate_names_txt(names_txt),
                "log.txt": self.__generate_log_txt(),
                "sys.txt": self.__generate_sys_txt(),
                "storage.txt": self.__generate_storage_txt(),
                "diag.txt": self.__generate_diag_txt(),
                "dyn_ns.txt": self.__generate_dyn_ns_txt(),
            })

        pass

    def __config_file_path(self, name):
        return os.path.join(self.__configs_dir, name)

    def __write_configs(self):
        for name, proto in self.__proto_configs.items():
            path = self.__config_file_path(name)
            with open(path, "w") as config_file:
                config_file.write(MessageToString(proto))

        pass

    def generate_command(self):
        self.__write_configs()

        command = [
            self.binary_path,
            "--service", self.service_type,
            "--mon-port", str(self.mon_port),
            "--server-file", self.__config_file_path("server.txt"),
            "--verbose", "debug" if self.verbose else "info",
            "--profile-file", os.path.join(self.__cwd, "profile-%s.log" % self.__ic_port),
            "--suppress-version-check",
        ]

        if self.__service_type == "kikimr":
            command += [
                "--domain", self.domains_txt.Domain[0].Name,
                "--ic-port", str(self.ic_port),
                "--node-broker", "localhost:" + str(self.__kikimr_port),
                "--domains-file", self.__config_file_path("domains.txt"),
                "--naming-file", self.__config_file_path("names.txt"),
                "--log-file", self.__config_file_path("log.txt"),
                "--sys-file", self.__config_file_path("sys.txt"),
                "--storage-file", self.__config_file_path("storage.txt"),
                "--dynamic-naming-file", self.__config_file_path("dyn_ns.txt"),
            ]

        if self.__restart_interval:
            launcher_path = common.binary_path(
                "cloud/storage/core/tools/testing/unstable-process/storage-unstable-process")

            command = [
                launcher_path,
                "--ping-port", str(self.mon_port),
                "--ping-timeout", str(2),
                "--restart-interval", str(self.__restart_interval),
                "--cmdline", " ".join(command),
            ]

        return command
