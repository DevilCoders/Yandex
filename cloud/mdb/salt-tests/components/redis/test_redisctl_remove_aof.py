import yatest.common
from hamcrest import assert_that, equal_to
import os.path

from cloud.mdb.internal.python.pytest.utils import parametrize
from test_utils import (
    main_test,
    get_conn_sequence,
    ERROR_RESULT_CODE,
    MasterConn,
    AofFilledDiskConn,
    OkConn,
    SlaveConn,
    SUCCESS_RESULT_CODE,
    SUCCESS_RESULT_PRINT,
    AofDisabledConn,
)

DISK_IS_NOT_FULL_PARTS = ['Disk usage', 'is less than limit 95% provided, nothing done']
AOF_FILE = '/var/lib/redis/appendonly.aof'
AOF_DIR = '/var/lib/redis/appendonlydir'
AOF_REDIS_7_BASE = 'appendonly.aof.1.base.rdb'
AOF_REDIS_7_AOF = 'appendonly.aof.1.incr.aof'


class Flag(object):
    def __init__(self):
        self.on = False

    def get(self):
        return self.on

    def set(self):
        self.on = True

    def drop(self):
        self.on = False


AOF_REMOVED_FLAG = Flag()


class FileMock(object):
    def __init__(self, size, avail):
        self.f_bsize = size
        self.f_blocks = 1
        self.f_bavail = avail
        self.f_frsize = 1


def os_path_exists_provider(data):
    def os_path_exists(path):
        if path not in data:
            return False
        values = data[path]
        return values.pop(0)

    return os_path_exists


def os_listdir_provider(data):
    def os_listdir(path):
        if path not in data:
            return []
        values = data[path]
        return values.pop(0)

    return os_listdir


def disk_usage_provider(data_dict):
    def os_statvfs(path):
        usage = data_dict[path]
        return FileMock(usage, 100 - usage)

    return os_statvfs


def file_size_provider(data_dict):
    def getsize(path):
        usage = data_dict[path]
        return usage

    return getsize


def aof_removed(flag):
    def os_unlink(path):
        if path == AOF_FILE:
            flag.set()

    return os_unlink


def aof_removed_redis_7(flag):
    def os_unlink(path):
        if path == os.path.join(AOF_DIR, AOF_REDIS_7_BASE):
            flag.set()

    return os_unlink


TEST_DATA = [
    {
        'id': 'No AOF file',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'remove_aof',
            ],
            'conn_func': get_conn_sequence(OkConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': ["No /var/lib/redis/appendonly.aof - skip other steps"],
            'custom_patch_dict': {},
            'aof_removed': False,
        },
    },
    {
        'id': 'No AOF file, Redis 7',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main_redis_7'),
                '--action',
                'remove_aof',
            ],
            'conn_func': get_conn_sequence(OkConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': ["No base AOF file - skip other steps"],
            'custom_patch_dict': {},
            'aof_removed': False,
        },
    },
    {
        'id': 'AOF file exists, master, no --force',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'remove_aof',
            ],
            'conn_func': get_conn_sequence(MasterConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': ["Can't proceed on master without --force flag"],
            'custom_patch_dict': {
                'os.path.exists': os_path_exists_provider(
                    {
                        AOF_FILE: [True],
                    }
                )
            },
            'aof_removed': False,
        },
    },
    {
        'id': 'AOF file exists, disk full, dead conn',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'remove_aof',
            ],
            'conn_func': get_conn_sequence(AofFilledDiskConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': [
                "Disk is full but we can't proceed as we failed to connect to Redis to check "
                + "if it's master (to skip that use --force flag)"
            ],
            'custom_patch_dict': {
                'os.path.exists': os_path_exists_provider(
                    {
                        AOF_FILE: [True],
                    }
                ),
            },
            'aof_removed': False,
        },
    },
    {
        'id': 'AOF file exists, slave, disk is not full enough, alive conn',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'remove_aof',
            ],
            'conn_func': get_conn_sequence(SlaveConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': DISK_IS_NOT_FULL_PARTS,
            'expected_err_parts': [],
            'custom_patch_dict': {
                'os.path.exists': os_path_exists_provider(
                    {
                        AOF_FILE: [True],
                    }
                ),
                'os.statvfs': disk_usage_provider({AOF_FILE: 94}),
            },
            'aof_removed': False,
        },
    },
    {
        'id': 'AOF file exists, slave, disk is not full enough, alive conn, Redis 7',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main_redis_7'),
                '--action',
                'remove_aof',
            ],
            'conn_func': get_conn_sequence(SlaveConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': DISK_IS_NOT_FULL_PARTS,
            'expected_err_parts': [],
            'custom_patch_dict': {
                'os.path.exists': os_path_exists_provider(
                    {
                        AOF_DIR: [True],
                    }
                ),
                'os.listdir': os_listdir_provider(
                    {
                        AOF_DIR: [[AOF_REDIS_7_BASE, AOF_REDIS_7_AOF]],
                    }
                ),
                'os.statvfs': disk_usage_provider({AOF_DIR: 94}),
            },
            'aof_removed': False,
        },
    },
    {
        'id': 'AOF file exists, slave, disk is full, alive conn, AOF off -> removed',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'remove_aof',
            ],
            'conn_func': get_conn_sequence(SlaveConn, AofDisabledConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
            'custom_patch_dict': {
                'os.path.exists': os_path_exists_provider(
                    {
                        AOF_FILE: [True, True, True, True, False],
                    }
                ),
                'os.statvfs': disk_usage_provider({AOF_FILE: 96}),
                'os.path.getsize': file_size_provider({AOF_FILE: 96}),
                'os.unlink': aof_removed(AOF_REMOVED_FLAG),
            },
            'aof_removed': True,
        },
    },
    {
        'id': 'AOF file exists, slave, disk is full, alive conn, AOF off, Redis 7 -> removed',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main_redis_7'),
                '--action',
                'remove_aof',
            ],
            'conn_func': get_conn_sequence(SlaveConn, AofDisabledConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
            'custom_patch_dict': {
                'os.path.exists': os_path_exists_provider(
                    {
                        AOF_DIR: [True],
                        os.path.join(AOF_DIR, AOF_REDIS_7_BASE): [True, True, True, False],
                    }
                ),
                'os.listdir': os_listdir_provider(
                    {
                        AOF_DIR: [
                            [AOF_REDIS_7_BASE, AOF_REDIS_7_AOF],
                            [AOF_REDIS_7_BASE, AOF_REDIS_7_AOF],
                            [AOF_REDIS_7_BASE],
                        ],
                    }
                ),
                'os.statvfs': disk_usage_provider({AOF_DIR: 96}),
                'os.path.getsize': file_size_provider({AOF_DIR: 96, os.path.join(AOF_DIR, AOF_REDIS_7_BASE): 95}),
                'os.unlink': aof_removed_redis_7(AOF_REMOVED_FLAG),
            },
            'aof_removed': True,
        },
    },
]


@parametrize(*TEST_DATA)
def test_redisctl_stop_and_wait(
    capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts, custom_patch_dict, aof_removed
):
    AOF_REMOVED_FLAG.drop()
    main_test(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts, custom_patch_dict)
    assert_that(AOF_REMOVED_FLAG.get(), equal_to(aof_removed))
