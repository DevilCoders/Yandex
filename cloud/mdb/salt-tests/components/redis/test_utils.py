import time
from contextlib import ExitStack
from unittest.mock import patch
from redis.exceptions import BusyLoadingError, ConnectionError, ResponseError
import pytest
from hamcrest import assert_that, contains_string, equal_to

from utils import config, command, state
from redisctl import redisctl


WAIT_CMD_ERROR_PARTS = [
    'FAILED: wait command: time left = ',
    '; restarts left = 0; last pids seen = []; last result seen: ',
]
DEFAULT_CMD_RESULT = command.CommandResult()
SUCCESS_RESULT_CODE = DEFAULT_CMD_RESULT.result_code
SUCCESS_RESULT_PRINT = DEFAULT_CMD_RESULT.result_print
ERROR_RESULT_CODE = command.ResultCode.ERROR
UNEXPECTED_RESULT = state.RedisStatus.UNDEFINED.value
TEST_DUMP_DATA = b'test_dump_data'
BGSAVE_NAME = 'bgsave'
LASTSAVE_NAME = 'lastsave'
CONFIG_NAME = 'config'
RDB_FULL_PATH = '/tmp/dump.rdb'


class OkConn(object):
    def ping(self):
        return 'PONG'


class DeadConn(object):
    def ping(self):
        raise Exception("test dead conn")


class ReplicaSyncedConn(OkConn):
    def info(self):
        return {
            'master_failover_state': 'no-failover',
            'master_sync_in_progress': 0,
            'role': 'replica',
            'master_link_status': 'up',
        }


class ReplicaSyncInProgressConn(OkConn):
    def info(self):
        return {
            'master_sync_in_progress': 1,
        }


class FailoverInProgressConn(OkConn):
    def info(self):
        return {
            'master_failover_state': "failover-in-progress",
        }


class MasterConn(OkConn):
    def info(self):
        return {
            'role': 'master',
        }


class ReplicaUnexpectedStateConn(OkConn):
    def info(self):
        return {
            'master_failover_state': 'no-failover',
            'master_sync_in_progress': 0,
            'role': 'replica',
            'master_link_status': 'down',
        }


class Config(config.Config):
    def __init__(self, args=None):
        super().__init__(args)

    def get_pass_from_file(self):
        return "test_password"

    def read_dbaas_config(self):
        if self.config['args'].dbaas_conf_path == '/etc/dbaas.conf':
            return {}
        return super().read_dbaas_config()


class ShutdownSuccessConn(OkConn):
    def execute_command(self, *args, **kwargs):
        raise ConnectionError


class ShutdownFailedConn(OkConn):
    def execute_command(self, *args, **kwargs):
        raise ResponseError


class ShutdownUnexpectedConn(OkConn):
    def execute_command(self, *args, **kwargs):
        pass


class AofDisabledConn(OkConn):
    def execute_command(self, *args, **kwargs):
        return 'appendonly', 'no'


class AofEnabledConn(OkConn):
    def execute_command(self, *args, **kwargs):
        return 'appendonly', 'yes'


class RDBEnabledConn(OkConn):
    def execute_command(self, *args, **kwargs):
        return 'save', '900 1'


class PersOffConn(OkConn):
    def execute_command(self, _, arg):
        dct = {'save': '', 'appendonly': 'no'}
        return arg, dct[arg]


def bgsave():
    with open(RDB_FULL_PATH, 'wb') as fobj:
        fobj.write(TEST_DUMP_DATA)


def bgsave_in_progress():
    bgsave()
    raise ConnectionError('already in progress')


def bgsave_error():
    bgsave()
    raise ConnectionError('unexpected_bgsave_error')


class DumpConn(OkConn):
    def execute_command(self, cmd):
        dct = {
            LASTSAVE_NAME: time.time() - 5,
            BGSAVE_NAME: bgsave(),
        }
        return cmd, dct[cmd]


class DumpErrorConn(OkConn):
    def execute_command(self, cmd):
        if cmd == LASTSAVE_NAME:
            return cmd, time.time() - 5
        if cmd == BGSAVE_NAME:
            bgsave_error()


class DumpInProgressConn(OkConn):
    def execute_command(self, cmd):
        if cmd == LASTSAVE_NAME:
            return cmd, time.time() - 5
        if cmd == BGSAVE_NAME:
            bgsave_in_progress()


class BrokenLastsaveDumpConn(OkConn):
    def execute_command(self, cmd):
        if cmd == LASTSAVE_NAME:
            raise ResponseError('broken lastsave')
        if cmd == BGSAVE_NAME:
            return cmd, bgsave()


class SlaveConn(OkConn):
    def info(self):
        return {'role': 'slave'}


class AofFilledDiskConn(object):
    def ping(self):
        exc = ResponseError()
        exc.message = "MISCONF Errors writing to the AOF file: No space left on device"
        raise exc


class LoadingConn(object):
    def ping(self):
        raise BusyLoadingError


class ShutdownSuccessLoadingConn(LoadingConn, ShutdownSuccessConn):
    pass


class ShutdownSuccessAofFIlledConn(AofFilledDiskConn, ShutdownSuccessConn):
    pass


def get_config_option(option, config_path=None, index=1):
    OPTIONS = {
        ('rename-command BGSAVE', 2): BGSAVE_NAME,
        ('rename-command LASTSAVE', 2): LASTSAVE_NAME,
        ('rename-command CONFIG', 2): CONFIG_NAME,
        ('rename-command SHUTDOWN', 2): 'stub',
        ('rename-command BGREWRITEAOF', 2): 'stub',
    }
    return OPTIONS[(option, index)]


def get_conn_sequence(*args):
    for conn in args:
        yield conn()


def main_test(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts, custom_patch_dict=None):
    args = redisctl.get_args(argv)
    patch_paths_funcs = {
        'utils.connection.Redis': conn_func,
        'utils.config._get_config_option_from_files': get_config_option,
    }
    if custom_patch_dict:
        patch_paths_funcs.update(custom_patch_dict)

    with ExitStack() as stack:
        for path, func in patch_paths_funcs.items():
            obj = stack.enter_context(patch(path))
            obj.side_effect = func
        config = Config(args)
        with pytest.raises(SystemExit) as e:
            redisctl.run(args, config)

    assert_that(e.type, equal_to(SystemExit))
    assert_that(e.value.code, equal_to(expected_code))
    out, err = capsys.readouterr()
    for expected_out in expected_out_parts:
        assert_that(out, contains_string(expected_out))
    for expected_err in expected_err_parts:
        assert_that(err, contains_string(expected_err))
