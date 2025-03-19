from dataclasses import dataclass

from cloud.blockstore.pylibs.clusters import FolderDesc, get_cluster_test_config


@dataclass
class YcpConfig:
    folder: FolderDesc
    placement_group_name: str
    image_name: str


@dataclass
class DiskCreateConfig:
    size: int
    bs: int
    type: str
    placement_group_name: str
    image_name: str

    def __init__(self, size, bs, type, placement_group_name=None, image_name=None):
        self.size = size
        self.bs = bs
        self.type = type
        self.placement_group_name = placement_group_name
        self.image_name = image_name


@dataclass
class FsCreateConfig:
    size: int
    bs: int
    type: str
    device_name: str
    mount_path: str

    def __init__(self, size, bs, type, device_name='nfs', mount_path='/test'):
        self.size = size
        self.bs = bs
        self.type = type
        self.device_name = device_name
        self.mount_path = mount_path


@dataclass
class LoadConfig:
    use_requests_with_different_sizes: bool
    need_filling: bool
    io_depth: int
    write_rate: int
    bs: int
    write_parts: int

    def __init__(self, use_requests_with_different_sizes, need_filling, io_depth, write_rate, bs, write_parts=1):
        self.use_requests_with_different_sizes = use_requests_with_different_sizes
        self.need_filling = need_filling
        self.io_depth = io_depth
        self.write_rate = write_rate
        self.bs = bs
        self.write_parts = write_parts


@dataclass
class TestConfig:
    test_file: str
    ycp_config: YcpConfig
    disk_config: DiskCreateConfig
    fs_config: FsCreateConfig
    load_config: LoadConfig


@dataclass
class DBTestConfig:
    ycp_config: YcpConfig
    disk_config: DiskCreateConfig
    db: str


_DISK_CONFIGS = {
    'eternal-640gb-verify-checkpoint': DiskCreateConfig(640, 4096, 'network-ssd'),
    'eternal-320gb': DiskCreateConfig(320, 4096, 'network-ssd'),
    'eternal-4tb': DiskCreateConfig(4096, 4096, 'network-ssd'),
    'eternal-4tb-one-partition': DiskCreateConfig(4096, 4096, 'network-ssd'),

    'eternal-1023gb-nonrepl':
        DiskCreateConfig(
            size=93 * 11,
            bs=4096,
            type='network-ssd-nonreplicated',
            placement_group_name='eternal-pg'),
    'eternal-1023gb-nonrepl-vhost':
        DiskCreateConfig(
            size=93 * 11,
            bs=4096,
            type='network-ssd-nonreplicated',
            placement_group_name='eternal-pg'),
    'eternal-1023gb-nonrepl-rdma':
        DiskCreateConfig(
            size=1023,
            bs=4096,
            type='network-ssd-nonreplicated',
            placement_group_name='eternal-pg'),
    'eternal-1023gb-nonrepl-rdma-vhost':
        DiskCreateConfig(
            size=1023,
            bs=4096,
            type='network-ssd-nonreplicated',
            placement_group_name='eternal-pg'),

    'eternal-512gb-different-size-requests': DiskCreateConfig(512, 4096, 'network-ssd'),
    'eternal-1tb-different-size-requests': DiskCreateConfig(1024, 4096, 'network-ssd'),

    'eternal-1tb-mysql': DiskCreateConfig(1024, 4096, 'network-ssd'),
    'eternal-1tb-postgresql': DiskCreateConfig(1024, 4096, 'network-ssd'),

    'eternal-1023gb-nonrepl-mysql':
        DiskCreateConfig(
            size=93 * 11,
            bs=4096,
            type='network-ssd-nonreplicated',
            placement_group_name='eternal-pg'),
    'eternal-1023gb-nonrepl-postgresql':
        DiskCreateConfig(
            size=93 * 11,
            bs=4096,
            type='network-ssd-nonreplicated',
            placement_group_name='eternal-pg'),

    'eternal-320gb-overlay':
        DiskCreateConfig(
            size=320,
            bs=4096,
            type='network-ssd',
            image_name='ubuntu1604-stable'),
}

_FS_CONFIGS = {
    'eternal-100gb-nfs-different-size-requests': FsCreateConfig(100, 4096, 'network-ssd'),
    'eternal-100gb-nfs': FsCreateConfig(100, 4096, 'network-ssd'),
    'eternal-100gb-nfs-multipart-write': FsCreateConfig(100, 4096, 'network-ssd'),
}

_LOAD_CONFIGS = {
    'eternal-640gb-verify-checkpoint': LoadConfig(False, True, 32, 50, 4096),
    'eternal-320gb': LoadConfig(False, True, 32, 50, 4096),
    'eternal-4tb': LoadConfig(False, True, 32, 50, 4096),
    'eternal-4tb-one-partition': LoadConfig(False, True, 32, 50, 4096),

    'eternal-1023gb-nonrepl': LoadConfig(False, False, 32, 50, 4096),
    'eternal-1023gb-nonrepl-vhost': LoadConfig(False, False, 32, 50, 4096),
    'eternal-1023gb-nonrepl-rdma': LoadConfig(False, False, 32, 50, 4096),
    'eternal-1023gb-nonrepl-rdma-vhost': LoadConfig(False, False, 32, 50, 4096),

    'eternal-512gb-different-size-requests': LoadConfig(True, True, 32, 50, 4096),
    'eternal-1tb-different-size-requests': LoadConfig(True, True, 32, 50, 4096),

    'eternal-100gb-nfs-different-size-requests': LoadConfig(True, False, 32, 10, 4096),
    'eternal-100gb-nfs': LoadConfig(False, False, 32, 10, 1048576),
    'eternal-100gb-nfs-multipart-write': LoadConfig(False, False, 32, 10, 4096, 4),

    'eternal-320gb-overlay': LoadConfig(True, False, 32, 25, 4096),
}

_IPC_TYPE = {
    'eternal-1023gb-nonrepl-vhost': 'vhost',
    'eternal-1023gb-nonrepl-rdma-vhost': 'vhost',
}

_DB = {
    'eternal-1tb-mysql': 'mysql',
    'eternal-1tb-postgresql': 'postgresql',
    'eternal-1023gb-nonrepl-mysql': 'mysql',
    'eternal-1023gb-nonrepl-postgresql': 'postgresql'
}


def generate_test_config(args, test_case: str) -> TestConfig:
    if test_case not in _LOAD_CONFIGS:
        return None

    ipc_type = _IPC_TYPE.get(test_case, 'grpc')
    image_name = None
    disk_config = _DISK_CONFIGS.get(test_case)
    fs_config = _FS_CONFIGS.get(test_case)
    load_config = _LOAD_CONFIGS.get(test_case)
    if disk_config is not None:
        file_path = args.file_path or '/dev/vdb'
    elif fs_config is not None:
        image_name = 'ubuntu2004'
        file_path = args.file_path or '/test/test.txt'
    else:
        raise Exception('Unknown test case')

    folder_id = get_cluster_test_config(args.cluster, args.zone_id).ipc_type_to_folder_desc.get(ipc_type)
    if folder_id is None:
        return None

    return TestConfig(
        file_path,
        YcpConfig(
            folder=folder_id,
            placement_group_name=getattr(args, 'placement_group_name', None) or "nbs-eternal-tests",
            image_name=image_name
        ),
        disk_config,
        fs_config,
        load_config)


def generate_db_test_config(args, test_case: str) -> DBTestConfig:
    if test_case not in _DB:
        return None

    ipc_type = _IPC_TYPE.get(test_case, 'grpc')
    return DBTestConfig(
        YcpConfig(
            folder=get_cluster_test_config(args.cluster, args.zone_id).ipc_type_to_folder_desc[ipc_type],
            placement_group_name=getattr(args, 'placement_group_name', None) or "nbs-eternal-tests",
            image_name='eternal-db-test'
        ),
        _DISK_CONFIGS[args.test_case],
        _DB[args.test_case]
    )


def generate_all_test_configs(args) -> [(str, TestConfig)]:
    configs = []
    for test_case in _LOAD_CONFIGS.keys():
        generated_config = generate_test_config(args, test_case)
        if generated_config is not None:
            configs.append((test_case, generate_test_config(args, test_case)))

    return configs


def generate_all_db_test_configs(args) -> [(str, DBTestConfig)]:
    configs = []
    for test_case in _DB.keys():
        configs.append((test_case, generate_db_test_config(args, test_case)))

    return configs


def get_test_config(args, db: bool):
    if db:
        if args.test_case == 'all':
            return generate_all_db_test_configs(args, args.test_case)
        else:
            return generate_db_test_config(args, args.test_case)
    else:
        if args.test_case == 'all':
            return generate_all_test_configs(args)
        return generate_test_config(args, args.test_case)
