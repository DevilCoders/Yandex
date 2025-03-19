import pytest
import uuid

import yatest.common as common

from cloud.blockstore.config.client_pb2 import TClientConfig
from cloud.blockstore.config.server_pb2 import TServerAppConfig, TServerConfig, TKikimrServiceConfig
from cloud.blockstore.config.storage_pb2 import TStorageServiceConfig, CT_LOAD
from cloud.blockstore.config.features_pb2 import TFeaturesConfig
from cloud.blockstore.tests.python.lib.loadtest_env import LocalLoadTest
from cloud.blockstore.tests.python.lib.test_base import thread_count, run_test
from cloud.storage.core.protos.endpoints_pb2 import EEndpointStorageType


def default_storage_config():
    bw = 1 << 7     # 128 MB/s
    iops = 1 << 16

    storage = TStorageServiceConfig()

    storage.SSDCompactionType = CT_LOAD
    storage.HDDCompactionType = CT_LOAD
    storage.V1GarbageCompactionEnabled = True

    storage.ThrottlingEnabled = True
    storage.ThrottlingEnabledSSD = True

    storage.SSDUnitReadBandwidth = bw
    storage.SSDUnitWriteBandwidth = bw
    storage.SSDMaxReadBandwidth = bw
    storage.SSDMaxWriteBandwidth = bw
    storage.SSDUnitReadIops = iops
    storage.SSDUnitWriteIops = iops
    storage.SSDMaxReadIops = iops
    storage.SSDMaxWriteIops = iops

    storage.HDDUnitReadBandwidth = bw
    storage.HDDUnitWriteBandwidth = bw
    storage.HDDMaxReadBandwidth = bw
    storage.HDDMaxWriteBandwidth = bw
    storage.HDDUnitReadIops = iops
    storage.HDDUnitWriteIops = iops
    storage.HDDMaxReadIops = iops
    storage.HDDMaxWriteIops = iops

    return storage


def storage_config_with_incremental_compaction():
    storage = default_storage_config()
    storage.IncrementalCompactionEnabled = True

    return storage


def storage_config_with_incremental_compaction_and_compaction_batching():
    storage = default_storage_config()
    storage.IncrementalCompactionEnabled = True
    storage.CompactionRangeCountPerRun = 5
    storage.SSDMaxBlobsPerRange = 5
    storage.HDDMaxBlobsPerRange = 5

    return storage


def storage_config_with_multiple_partitions(max_partitions):
    storage = default_storage_config()
    storage.BytesPerPartition = 4096
    storage.BytesPerPartitionSSD = 4096
    storage.MultipartitionVolumesEnabled = True
    storage.MaxPartitionsPerVolume = max_partitions

    return storage


def features_config_with_sparse_merged_blobs_and_incremental_compaction():
    config = TFeaturesConfig()

    config.Features.add()
    config.Features[0].Name = 'UseSparseMergedBlobs'
    config.Features[0].Whitelist.CloudIds.append("test_cloud")

    config.Features.add()
    config.Features[1].Name = 'IncrementalCompaction'
    config.Features[1].Whitelist.CloudIds.append("test_cloud")

    return config


def storage_config_with_fresh_channel_writes_enabled(enabled):
    storage = default_storage_config()
    storage.FreshChannelCount = 1
    storage.FreshChannelWriteRequestsEnabled = enabled

    return storage


def storage_config_with_logical_used_blocks_calculation_enabled(enabled):
    storage = default_storage_config()
    storage.LogicalUsedBlocksCalculationEnabled = enabled

    return storage


def storage_config_with_mixed_index_cache_enabled():
    storage = default_storage_config()
    storage.MixedIndexCacheV1Enabled = True

    return storage


class TestCase(object):

    def __init__(
            self,
            name,
            config_path,
            storage_config_patches,
            features_config_patch,
            restart_interval,
    ):
        self.name = name
        self.config_path = config_path
        self.storage_config_patches = storage_config_patches
        self.features_config_patch = features_config_patch
        self.restart_interval = restart_interval


TESTS = [
    TestCase(
        "version1-two-partitions",
        "cloud/blockstore/tests/loadtest/local-newfeatures/local-tablet-version-1-two-partitions.txt",
        [
            storage_config_with_multiple_partitions(2),
            storage_config_with_multiple_partitions(1),
        ],
        None,
        15,
    ),
    TestCase(
        "version1-two-partitions-and-checkpoints",
        "cloud/blockstore/tests/loadtest/local-newfeatures/local-tablet-version-1-two-partitions-and-checkpoints.txt",
        [
            storage_config_with_multiple_partitions(2),
            storage_config_with_multiple_partitions(1),
        ],
        None,
        15,
    ),
    TestCase(
        "version1-incremental-compaction",
        "cloud/blockstore/tests/loadtest/local-newfeatures/local-tablet-version-1-incremental-compaction.txt",
        [
            storage_config_with_incremental_compaction(),
            default_storage_config(),
        ],
        None,
        15,
    ),
    TestCase(
        "version1-incremental-compaction-and-compaction-batching",
        "cloud/blockstore/tests/loadtest/local-newfeatures/local-tablet-version-1-incremental-compaction.txt",
        [
            storage_config_with_incremental_compaction_and_compaction_batching(),
            default_storage_config(),
        ],
        None,
        15,
    ),
    TestCase(
        "version1-large-multipartition-disk",
        "cloud/blockstore/tests/loadtest/local-newfeatures/local-tablet-version-1-large-multipartition-disk.txt",
        [
            storage_config_with_multiple_partitions(16),
            storage_config_with_multiple_partitions(1),
        ],
        None,
        15,
    ),
    TestCase(
        "version1-incremental-compaction-cloudid",
        "cloud/blockstore/tests/loadtest/local-newfeatures/local-tablet-version-1-incremental-compaction.txt",
        None,
        features_config_with_sparse_merged_blobs_and_incremental_compaction(),
        15,
    ),
    TestCase(
        "version1-fresh-channel-writes-enabled",
        "cloud/blockstore/tests/loadtest/local-newfeatures/local-tablet-version-1-fresh-channel-writes.txt",
        [
            storage_config_with_fresh_channel_writes_enabled(False),
            storage_config_with_fresh_channel_writes_enabled(True),
        ],
        None,
        15,
    ),
    TestCase(
        "version1-fresh-channel-writes-enabled-no-restarts",
        "cloud/blockstore/tests/loadtest/local-newfeatures/local-tablet-version-1-fresh-channel-writes.txt",
        [
            storage_config_with_fresh_channel_writes_enabled(True),
        ],
        None,
        None,
    ),
    TestCase(
        "version1-logical-used-blocks-enabling",
        "cloud/blockstore/tests/loadtest/local-newfeatures/local-tablet-version-1-logical-used-blocks.txt",
        [
            storage_config_with_logical_used_blocks_calculation_enabled(False),
            storage_config_with_logical_used_blocks_calculation_enabled(True),
        ],
        None,
        60,
    ),
    TestCase(
        "version1-mixed-index-cache",
        "cloud/blockstore/tests/loadtest/local-newfeatures/local-tablet-version-1-mixed-index-cache.txt",
        [
            storage_config_with_mixed_index_cache_enabled(),
        ],
        None,
        15,
    ),
]


def __run_test(test_case):
    endpoint_storage_dir = common.output_path() + '/endpoints-' + str(uuid.uuid4())
    nbd_socket_suffix = "_nbd"

    server = TServerAppConfig()
    server.ServerConfig.CopyFrom(TServerConfig())
    server.ServerConfig.ThreadsCount = thread_count()
    server.ServerConfig.StrictContractValidation = True
    server.ServerConfig.NbdEnabled = True
    server.ServerConfig.NbdSocketSuffix = nbd_socket_suffix
    server.ServerConfig.EndpointStorageType = EEndpointStorageType.ENDPOINT_STORAGE_FILE
    server.ServerConfig.EndpointStorageDir = endpoint_storage_dir
    server.KikimrServiceConfig.CopyFrom(TKikimrServiceConfig())

    env = LocalLoadTest(
        "",
        server_app_config=server,
        storage_config_patches=test_case.storage_config_patches,
        static_pdisk_size=64 * 1024 * 1024 * 1024,
        use_in_memory_pdisks=True,
        features_config_patch=test_case.features_config_patch,
        restart_interval=test_case.restart_interval,
    )

    client = TClientConfig()
    client.NbdSocketSuffix = nbd_socket_suffix

    ret = run_test(
        test_case.name,
        test_case.config_path,
        env.nbs_port,
        env.mon_port,
        client_config=client,
        endpoint_storage_dir=endpoint_storage_dir,
        env_processes=[env.nbs],
    )

    env.tear_down()

    return ret


@pytest.mark.parametrize("test_case", TESTS, ids=[x.name for x in TESTS])
def test_load(test_case):
    config = common.get_param("config")
    if config is None:
        test_case.config_path = common.source_path(test_case.config_path)
        return __run_test(test_case)
