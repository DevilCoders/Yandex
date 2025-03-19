import logging
import os
import signal
import time

from yatest.common import process

from ydb.tests.library.harness.daemon import Daemon
from ydb.tests.library.harness.kikimr_runner import get_unique_path_for_current_test, ensure_path_exists
import ydb.tests.library.common.yatest_common as yatest_common

logger = logging.getLogger()


CONTROLPLANE_CONFIG_TEMPLATE = """
GrpcConfig: <
    Port: {port}
    Certs: <
        CertFile: "{cert_file}"
        PrivateKeyFile: "{private_key_file}"
    >
    Hostname: "{hostname}"
    KeepAlive: <>
>
TasksConfig: <
    TaskTypeBlacklist: [
        "dataplane.CollectSnapshots",
        "dataplane.CreateSnapshotFromDisk",
        "dataplane.CreateSnapshotFromSnapshot",
        "dataplane.CreateSnapshotFromURL",
        "dataplane.CreateSnapshotFromLegacySnapshot",
        "dataplane.TransferFromDiskToDisk",
        "dataplane.TransferFromSnapshotToDisk",
        "dataplane.TransferFromLegacySnapshotToDisk",
        "dataplane.DeleteSnapshot",
        "dataplane.DeleteSnapshotData"
    ]
    TaskPingPeriod: "1s"
    PollForTaskUpdatesPeriod: "1s"
    PollForTasksPeriodMin: "1s"
    PollForTasksPeriodMax: "2s"
    PollForStallingTasksPeriodMin: "2s"
    PollForStallingTasksPeriodMax: "4s"
    TaskStallingTimeout: "5s"
    TaskWaitingTimeout: "3s"
    ScheduleRegularTasksPeriodMin: "2s"
    ScheduleRegularTasksPeriodMax: "4s"
    RunnersCount: 30
    StalkingRunnersCount: 10
    EndedTaskExpirationTimeout: "2000s"
    ClearEndedTasksTaskScheduleInterval: "11s"
    ClearEndedTasksLimit: 10
    MaxRetriableErrorCount: 1000
    HangingTaskTimeout: "100s"
>
NfsConfig: <
    Zones: <
        key: "zone"
        value: <
            Endpoints: [
                "localhost:{nfs_port}",
                "localhost:{nfs_port}"
            ]
        >
    >
    Zones: <
        key: "other"
        value: <
            Endpoints: [
                "localhost:{nfs_port}",
                "localhost:{nfs_port}"
            ]
        >
    >
    Insecure: true
>
FilesystemConfig: <
    DeletedFilesystemExpirationTimeout: "1s"
    ClearDeletedFilesystemsTaskScheduleInterval: "2s"
>
NbsConfig: <
    Zones: <
        key: "zone"
        value: <
            Endpoints: [
                "localhost:{nbs_port}",
                "localhost:{nbs_port}"
            ]
        >
    >
    Zones: <
        key: "other"
        value: <
            Endpoints: [
                "localhost:{nbs_port}",
                "localhost:{nbs_port}"
            ]
        >
    >
    RootCertsFile: "{root_certs_file}"
    GrpcKeepAlive: <>
>
DisksConfig: <
    DeletedDiskExpirationTimeout: "1s"
    ClearDeletedDisksTaskScheduleInterval: "2s"
    UseDataplaneTasksForLegacySnapshots: true
>
SnapshotConfig: <
    DefaultZoneId: "zone"
    Zones: <
        key: "zone"
        value: <
            Endpoints: [
                "localhost:{snapshot_port}",
                "localhost:{snapshot_port}"
            ]
        >
    >
    Zones: <
        key: "other"
        value: <
            Endpoints: [
                "localhost:{snapshot_port}",
                "localhost:{snapshot_port}"
            ]
        >
    >
    Secure: true
    RootCertsFile: "{root_certs_file}"
>
PoolsConfig: <
    MaxActiveSlots: 10
    MaxBaseDisksInflight: 3
    MaxBaseDiskUnits: 100
    TakeBaseDisksToScheduleParallelism: 10
    ScheduleBaseDisksTaskScheduleInterval: "10s"
    DeleteBaseDisksTaskScheduleInterval: "10s"
    CloudId: "cloud"
    FolderId: "pools"
    DeleteBaseDisksLimit: 100
    DeletedBaseDiskExpirationTimeout: "1s"
    ClearDeletedBaseDisksTaskScheduleInterval: "1s"
    ReleasedSlotExpirationTimeout: "1s"
    ClearReleasedSlotsTaskScheduleInterval: "1s"
    UseDataplaneTasksForLegacySnapshots: true
    ConvertToImageSizedBaseDiskThreshold: 10
    ConvertToDefaultSizedBaseDiskThreshold: 30
    OptimizeBaseDisksTaskScheduleInterval: "10s"
    RegularBaseDiskOptimizationEnabled: true
    MinOptimizedPoolAge: "1s"
>
TransferConfig: <
    SnapshotTaskPingPeriod: "1s"
    SnapshotTaskHeartbeatTimeout: "5s"
    SnapshotTaskBackoffTimeout: "11s"
>
ImagesConfig: <
    SnapshotTaskPingPeriod: "1s"
    SnapshotTaskHeartbeatTimeout: "5s"
    SnapshotTaskBackoffTimeout: "11s"
    DeletedImageExpirationTimeout: "1s"
    ClearDeletedImagesTaskScheduleInterval: "2s"
    DefaultDiskPoolConfigs: [
        <
            ZoneId: "zone"
            Capacity: 0
        >,
        <
            ZoneId: "other"
            Capacity: 1
        >
    ]
    UseDataplaneTasksForFolder: ["dataplaneFolder"]
    UseDataplaneTasksForLegacySnapshots: true
    UseDataplaneFromURLTasksForFolder: ["dataplaneFromURLFolder"]
>
SnapshotsConfig: <
    SnapshotTaskPingPeriod: "1s"
    SnapshotTaskHeartbeatTimeout: "5s"
    SnapshotTaskBackoffTimeout: "11s"
    DeletedSnapshotExpirationTimeout: "1s"
    ClearDeletedSnapshotsTaskScheduleInterval: "2s"
    UseDataplaneTasksForFolder: ["dataplaneFolder"]
>
LoggingConfig: <
    LoggingStderr: <>
    Level: LEVEL_DEBUG
>
MonitoringConfig: <
    Port: {monitoring_port}
    RestartsCountFile: "{restarts_count_file}"
>
AuthConfig: <
    DisableAuthorization: false
    MetadataUrl: "{metadata_url}"
    AccessServiceEndpoint: "localhost:{access_service_port}"
    CertFile: "{root_certs_file}"
    ConnectionTimeout: "2s"
    RetryCount: 7
    PerRetryTimeout: "2s"
    FolderId: "DiskManagerFolderId"
>
PersistenceConfig: <
    Endpoint: "localhost:{kikimr_port}"
    Database: "/Root"
    RootPath: "disk_manager/recipe"
>
PlacementGroupConfig: <
    DeletedPlacementGroupExpirationTimeout: "1s"
    ClearDeletedPlacementGroupsTaskScheduleInterval: "2s"
>
"""


DATAPLANE_CONFIG_TEMPLATE = """
TasksConfig: <
    ZoneId: "zone"
    TaskTypeWhitelist: [
        "dataplane.CollectSnapshots",
        "dataplane.CreateSnapshotFromDisk",
        "dataplane.CreateSnapshotFromSnapshot",
        "dataplane.CreateSnapshotFromURL",
        "dataplane.CreateSnapshotFromLegacySnapshot",
        "dataplane.TransferFromDiskToDisk",
        "dataplane.TransferFromSnapshotToDisk",
        "dataplane.TransferFromLegacySnapshotToDisk",
        "dataplane.DeleteSnapshot",
        "dataplane.DeleteSnapshotData"
    ]
    TaskPingPeriod: "1s"
    PollForTaskUpdatesPeriod: "1s"
    PollForTasksPeriodMin: "1s"
    PollForTasksPeriodMax: "2s"
    PollForStallingTasksPeriodMin: "2s"
    PollForStallingTasksPeriodMax: "4s"
    TaskStallingTimeout: "5s"
    TaskWaitingTimeout: "3s"
    ScheduleRegularTasksPeriodMin: "2s"
    ScheduleRegularTasksPeriodMax: "4s"
    RunnersCount: 30
    StalkingRunnersCount: 10
    EndedTaskExpirationTimeout: "2000s"
    ClearEndedTasksTaskScheduleInterval: "11s"
    ClearEndedTasksLimit: 10
    MaxRetriableErrorCount: 1000
    HangingTaskTimeout: "100s"
>
NbsConfig: <
    Zones: <
        key: "zone"
        value: <
            Endpoints: [
                "localhost:{nbs_port}",
                "localhost:{nbs_port}"
            ]
        >
    >
    Zones: <
        key: "other"
        value: <
            Endpoints: [
                "localhost:{nbs_port}",
                "localhost:{nbs_port}"
            ]
        >
    >
    RootCertsFile: "{root_certs_file}"
    GrpcKeepAlive: <
        PermitWithoutStream: true
    >
>
LoggingConfig: <
    LoggingStderr: <>
    Level: LEVEL_DEBUG
>
MonitoringConfig: <
    Port: {monitoring_port}
    RestartsCountFile: "{restarts_count_file}"
>
AuthConfig: <
    MetadataUrl: "{metadata_url}"
>
PersistenceConfig: <
    Endpoint: "localhost:{kikimr_port}"
    Database: "/Root"
    RootPath: "disk_manager/recipe"
>
DataplaneConfig: <
    SnapshotConfig: <
        LegacyStorageFolder: "legacy_snapshot"
        PersistenceConfig: <
            Endpoint: "localhost:{kikimr_port}"
            Database: "/Root"
        >
        ChunkBlobsTableShardCount: 1
        ChunkMapTableShardCount: 1
        ExternalBlobsMediaKind: "ssd"
        DeleteWorkerCount: 10
        ShallowCopyWorkerCount: 10
        ShallowCopyInflightLimit: 100
        ChunkCompression: "lz4"
    >
    ReaderCount: 50
    WriterCount: 50
    ChunksInflightLimit: 100
    SnapshotCollectionTimeout: "1s"
    CollectSnapshotsTaskScheduleInterval: "2s"
    SnapshotCollectionInflightLimit: 10
    UseGetChangedBlocks: true
>
"""


def get_pid_file_name(idx):
    return "disk_manager_recipe_disk_manager_{}.pid".format(idx)


class DiskManagerServer(Daemon):

    def __init__(self, config_file, working_dir, with_nemesis):
        disk_manager_path = yatest_common.binary_path(
            "cloud/disk_manager/cmd/yc-disk-manager/yc-disk-manager"
        )
        nemesis_path = yatest_common.binary_path(
            "cloud/disk_manager/test/nemesis/nemesis"
        )

        if with_nemesis:
            internal_command = disk_manager_path + " --config " + config_file
            command = [nemesis_path]
            command += ["--cmd", internal_command]
        else:
            command = [disk_manager_path]
            command += ["--config", config_file]

        super(DiskManagerServer, self).__init__(
            command=command,
            cwd=working_dir,
            timeout=180)


class DiskManagerLauncher:

    def __init__(
        self,
        hostname,
        kikimr_port,
        nbs_port,
        metadata_url,
        root_certs_file,
        idx,
        is_dataplane,
        with_nemesis,
        nfs_port=None,
        snapshot_port=None,
        access_service_port=None,
        cert_file=None,
        cert_key_file=None,
    ):
        self.__idx = idx

        self.__port_manager = yatest_common.PortManager()
        self.__port = self.__port_manager.get_port()
        self.__monitoring_port = self.__port_manager.get_port()

        working_dir = get_unique_path_for_current_test(
            output_path=yatest_common.output_path(),
            sub_folder=""
        )
        ensure_path_exists(working_dir)

        self.__restarts_count_file = os.path.join(working_dir, 'restarts_count_{}.txt'.format(idx))
        with open(self.__restarts_count_file, 'w') as f:
            if idx % 2 == 0:
                f.write(str(idx))
            else:
                # test empty restarts count file
                pass

        config_file = os.path.join(
            working_dir,
            'disk_manager_config_{}.txt'.format(idx)
        )

        self.__server_config = None
        if is_dataplane:
            with open(config_file, "w") as f:
                f.write(DATAPLANE_CONFIG_TEMPLATE.format(
                    root_certs_file=root_certs_file,
                    nbs_port=nbs_port,
                    monitoring_port=self.__monitoring_port,
                    restarts_count_file=self.__restarts_count_file,
                    metadata_url=metadata_url,
                    kikimr_port=kikimr_port
                ))
        else:
            with open(config_file, "w") as f:
                self.__server_config = CONTROLPLANE_CONFIG_TEMPLATE.format(
                    port=self.__port,
                    hostname=hostname,
                    cert_file=cert_file,
                    private_key_file=cert_key_file,
                    root_certs_file=root_certs_file,
                    nfs_port=nfs_port,
                    nbs_port=nbs_port,
                    snapshot_port=snapshot_port,
                    monitoring_port=self.__monitoring_port,
                    restarts_count_file=self.__restarts_count_file,
                    metadata_url=metadata_url,
                    access_service_port=access_service_port,
                    kikimr_port=kikimr_port
                )
                f.write(self.__server_config)

        init_database_command = [
            yatest_common.binary_path(
                "cloud/disk_manager/cmd/yc-disk-manager-init-db/yc-disk-manager-init-db"
            ),
            "--config",
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

        self.__daemon = DiskManagerServer(config_file, working_dir, with_nemesis)

    def start(self):
        self.__daemon.start()
        with open(get_pid_file_name(self.__idx), "w") as f:
            f.write(str(self.__daemon.daemon.process.pid))

    @staticmethod
    def stop(idx):
        pid_file_name = get_pid_file_name(idx)
        if not os.path.exists(pid_file_name):
            return
        with open(pid_file_name) as f:
            pid = int(f.read())
            os.kill(pid, signal.SIGTERM)

    @property
    def port(self):
        return self.__port

    @property
    def server_config(self):
        return self.__server_config
