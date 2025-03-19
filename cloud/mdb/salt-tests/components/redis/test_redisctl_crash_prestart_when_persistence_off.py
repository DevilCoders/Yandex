import yatest.common

from cloud.mdb.internal.python.pytest.utils import parametrize
from test_utils import DeadConn, main_test, get_conn_sequence, SUCCESS_RESULT_CODE, SUCCESS_RESULT_PRINT
from utils.command import CommandResult, ErrorResult


def _ping_shard_fail_and_drop_flag(config, *args, **kwargs):
    config.config['args'].redis_data_path = yatest.common.test_source_path('test_data/info.json_non_infinite_wait')
    return ErrorResult()


def _ping_shard_fail_and_make_single_noded(config, *args, **kwargs):
    config.config['dbaas_conf_path'] = yatest.common.test_source_path('test_data/dbaas.conf_single_noded')
    return ErrorResult()


IP1 = "some.ip"
IP2 = "another.ip"
REDIS_CONFIGS_PERS_OFF = (
    yatest.common.test_source_path('test_data/redis.conf_main'),
    yatest.common.test_source_path('test_data/redis-main.conf_pers_off'),
)
REDIS_CONFIGS_PERS_ON = (
    yatest.common.test_source_path('test_data/redis.conf_main'),
    yatest.common.test_source_path('test_data/redis-main.conf_main'),
)
DBAAS_SHARDED = yatest.common.test_source_path('test_data/dbaas.conf_sharded')
DBAAS_SINGLE_NODED = yatest.common.test_source_path('test_data/dbaas.conf_single_noded')
TEST_DATA = [
    {
        'id': 'AOF on, skipping',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--redis-conf-path',
                REDIS_CONFIGS_PERS_ON,
                '--action',
                'crash_prestart_when_persistence_off',
            ],
            'custom_patch_dict': {},
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': ['AOF or RDB is on, skipping'],
            'expected_err_parts': [],
        },
    },
    {
        'id': '1-noded, skipping',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--redis-conf-path',
                REDIS_CONFIGS_PERS_OFF,
                '--dbaas-conf-path',
                DBAAS_SINGLE_NODED,
                '--action',
                'crash_prestart_when_persistence_off',
            ],
            'custom_patch_dict': {},
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': ['single node in shard, skipping'],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Persistence off, not a master in sentinel',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--dbaas-conf-path',
                yatest.common.test_source_path('test_data/dbaas.conf_main'),
                '--sentinel-conf-path',
                yatest.common.test_source_path('test_data/sentinel.conf_main'),
                '--cluster-conf-path',
                yatest.common.test_source_path('test_data/cluster.conf_main'),
                '--redis-conf-path',
                REDIS_CONFIGS_PERS_OFF,
                '--action',
                'crash_prestart_when_persistence_off',
            ],
            'custom_patch_dict': {
                'utils.redis_server.get_sentinel_config_option': lambda *args, **kwargs: IP1,
                'utils.redis_server.get_ip': lambda *args, **kwargs: IP2,
            },
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': ["not a master according to config, skipping"],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Persistence off, not a master in sharded',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--dbaas-conf-path',
                DBAAS_SHARDED,
                '--sentinel-conf-path',
                yatest.common.test_source_path('test_data/sentinel.conf_main'),
                '--cluster-conf-path',
                yatest.common.test_source_path('test_data/cluster.conf_main'),
                '--redis-conf-path',
                REDIS_CONFIGS_PERS_OFF,
                '--action',
                'crash_prestart_when_persistence_off',
            ],
            'custom_patch_dict': {
                'utils.redis_server.is_master_in_sharded_config': lambda *args, **kwargs: False,
            },
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': ["not a master according to config, skipping"],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Persistence off, master in sentinel - wait',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--dbaas-conf-path',
                yatest.common.test_source_path('test_data/dbaas.conf_main'),
                '--sentinel-conf-path',
                yatest.common.test_source_path('test_data/sentinel.conf_main'),
                '--cluster-conf-path',
                yatest.common.test_source_path('test_data/cluster.conf_main'),
                '--redis-conf-path',
                REDIS_CONFIGS_PERS_OFF,
                '--action',
                'crash_prestart_when_persistence_off',
            ],
            'custom_patch_dict': {
                'utils.redis_server.get_sentinel_config_option': lambda *args, **kwargs: IP1,
                'utils.redis_server.get_ip': lambda *args, **kwargs: IP1,
                'commands.crash_prestart_when_persistence_off.ping_shard': lambda *args, **kwargs: CommandResult(),
            },
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Persistence off, master in sharded - wait',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--dbaas-conf-path',
                DBAAS_SHARDED,
                '--sentinel-conf-path',
                yatest.common.test_source_path('test_data/sentinel.conf_main'),
                '--cluster-conf-path',
                yatest.common.test_source_path('test_data/cluster.conf_main'),
                '--redis-conf-path',
                REDIS_CONFIGS_PERS_OFF,
                '--action',
                'crash_prestart_when_persistence_off',
            ],
            'custom_patch_dict': {
                'utils.redis_server.get_sentinel_config_option': lambda *args, **kwargs: IP1,
                'utils.redis_server.get_ip': lambda *args, **kwargs: IP1,
                'commands.crash_prestart_when_persistence_off.ping_shard': lambda *args, **kwargs: CommandResult(),
            },
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Persistence off, master in sentinel - wait infinite',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--dbaas-conf-path',
                yatest.common.test_source_path('test_data/dbaas.conf_main'),
                '--sentinel-conf-path',
                yatest.common.test_source_path('test_data/sentinel.conf_main'),
                '--cluster-conf-path',
                yatest.common.test_source_path('test_data/cluster.conf_main'),
                '--redis-conf-path',
                REDIS_CONFIGS_PERS_OFF,
                '--action',
                'crash_prestart_when_persistence_off',
            ],
            'custom_patch_dict': {
                'utils.redis_server.get_sentinel_config_option': lambda *args, **kwargs: IP1,
                'utils.redis_server.get_ip': lambda *args, **kwargs: IP1,
                'commands.crash_prestart_when_persistence_off.ping_shard': _ping_shard_fail_and_drop_flag,
            },
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Persistence off, master in sharded - wait infinite',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--dbaas-conf-path',
                DBAAS_SHARDED,
                '--sentinel-conf-path',
                yatest.common.test_source_path('test_data/sentinel.conf_main'),
                '--cluster-conf-path',
                yatest.common.test_source_path('test_data/cluster.conf_main'),
                '--redis-conf-path',
                REDIS_CONFIGS_PERS_OFF,
                '--action',
                'crash_prestart_when_persistence_off',
            ],
            'custom_patch_dict': {
                'utils.redis_server.get_sentinel_config_option': lambda *args, **kwargs: IP1,
                'utils.redis_server.get_ip': lambda *args, **kwargs: IP1,
                'commands.crash_prestart_when_persistence_off.ping_shard': _ping_shard_fail_and_drop_flag,
            },
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Persistence off, master in sentinel - wait infinite, then 1noded - skipping',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--dbaas-conf-path',
                yatest.common.test_source_path('test_data/dbaas.conf_main'),
                '--sentinel-conf-path',
                yatest.common.test_source_path('test_data/sentinel.conf_main'),
                '--cluster-conf-path',
                yatest.common.test_source_path('test_data/cluster.conf_main'),
                '--redis-conf-path',
                REDIS_CONFIGS_PERS_OFF,
                '--action',
                'crash_prestart_when_persistence_off',
            ],
            'custom_patch_dict': {
                'utils.redis_server.get_sentinel_config_option': lambda *args, **kwargs: IP1,
                'utils.redis_server.get_ip': lambda *args, **kwargs: IP1,
                'commands.crash_prestart_when_persistence_off.ping_shard': _ping_shard_fail_and_make_single_noded,
            },
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': ['single node in shard, skipping'],
            'expected_err_parts': [],
        },
    },
]


@parametrize(*TEST_DATA)
def test_redisctl_crash_prestart_when_persistence_off(
    capsys, argv, custom_patch_dict, conn_func, expected_code, expected_out_parts, expected_err_parts
):
    main_test(
        capsys,
        argv,
        conn_func,
        expected_code,
        expected_out_parts,
        expected_err_parts,
        custom_patch_dict=custom_patch_dict,
    )
