import argparse
import json
import random
import sys
import time

from cloud.blockstore.public.sdk.python.client import ClientError, ClientCredentials
import cloud.blockstore.public.sdk.python.protos as protos
from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.clusters import get_cluster_test_config
from cloud.blockstore.pylibs.ycp import YcpWrapper, make_ycp_engine
from cloud.blockstore.pylibs.sdk import make_client


class Error(Exception):
    pass


_NBS_PORT = {
    'preprod': 9768,
    'hw-nbs-stable-lab': 9766
}

_DEFAULT_KILL_PERIOD = 600
_DEFAULT_MIGRATION_TIMEOUT = 28800  # 8h


class Migration:

    def __init__(self, runner, volume):
        self.__runner = runner
        self.__volume = volume
        self.__agent_id = None

    def __enter__(self):
        self.__agent_id = self.__runner.start_migration(self.__volume)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        message = 'finish migration test'
        if exc_type is not None:
            message = 'halt migration test'
        self.__runner.change_agent_state(self.__agent_id, 0, message)


class TestRunner:
    __DEFAULT_POSTPONED_WEIGHT = 128 * 1024 ** 2  # Bytes
    __DEFAULT_ZONE_ID = 'ru-central1-a'

    def __init__(self, args, logger):
        self.args = args
        self.logger = logger
        self.helpers = common.make_helpers(self.args.dry_run)
        self.port = _NBS_PORT[args.cluster]
        self.need_kill_tablet = args.kill_tablet
        self.sleep_timeout = args.kill_period
        self.kills = 0
        self.max_kills = -1  # inf
        self.tablet_id = None
        self.instance = None
        self.disk_id = None

        if args.dry_run:
            self.sleep_timeout = 0
            self.max_kills = 6

        cluster = get_cluster_test_config(args.cluster, self.__DEFAULT_ZONE_ID)

        ycp = YcpWrapper(
            cluster.name,
            cluster.ipc_type_to_folder_desc[args.ipc_type],
            logger,
            make_ycp_engine(args.dry_run))

        for disk in ycp.list_disks():
            if disk.name == self.args.disk_name:
                self.disk_id = disk.id
                self.instance = ycp.get_instance(disk.instance_ids[0])
                break

        if self.disk_id is None:
            raise Error(f"Can't find disk with name {self.args.disk_name}")

        if cluster.name == 'preprod':
            token = ycp.create_iam_token(args.service_account_id)
            self.credentials = ClientCredentials(auth_token=token.iam_token)
        elif cluster.name == 'hw-nbs-stable-lab':
            self.credentials = None
        else:
            raise Error(f'Cluster with name {cluster.name} not supported in the test')

        try:
            self.client = make_client(
                self.args.dry_run,
                endpoint='{}:{}'.format(self.instance.compute_node, self.port),
                credentials=self.credentials,
                log=logger
            )
        except ClientError as e:
            raise Error(f'Failed to create grpc client:\n{e}')

        if self.need_kill_tablet:
            self.tablet_id = self.get_volume_tablet_id()

    @common.retry(5, 10, Error)
    def change_agent_state(self, agent_id: str, state: int, message):
        self.logger.info(f'Changing state of agent_id=<{agent_id}> to state=<{state}>')
        try:
            data = json.dumps({
                'Message': message + f' ({self.args.disk_name})',
                'ChangeAgentState': {
                    'AgentId': agent_id,
                    'State': state
                }
            })

            self.client.execute_action(
                action='diskregistrychangestate',
                input_bytes=str.encode(data))
        except ClientError as e:
            raise Error(f'Failed to change agent state <agent_id={agent_id}> <state={state}>:\n{e}')

    @common.retry(5, 10, Error)
    def get_volume_tablet_id(self):
        try:
            d = self.client.execute_action(
                action='DescribeVolume',
                input_bytes=str.encode('{"DiskId":"%s"}' % self.disk_id))
            return json.loads(d)['VolumeTabletId']
        except ClientError as e:
            raise Error(f'Failed to execute DescribeVolume action for disk <disk_id={self.disk_id}>:\n{e}')

    @common.retry(5, 10, Error)
    def get_volume_description(self):
        try:
            volume = self.client.describe_volume(disk_id=self.disk_id)
            return volume
        except ClientError as e:
            raise Error(f'Failed to execute describe volume for disk <disk_id={self.disk_id}>:\n{e}')

    @common.retry(5, 10, Error)
    def check_load(self):
        self.logger.info(f'check if eternal load running on instance with ip=<{self.instance.ip}>')
        with common.make_ssh_client(self.args.dry_run, self.instance.ip) as ssh:
            _, stdout, _ = ssh.exec_command('pgrep eternal-load')
            stdout.channel.exit_status_ready()
            out = ''.join(stdout.readlines())
            if not out:
                raise Error(f'Eternal load not running on instance with ip=<{self.instance.ip}>')

    def start_migration(self, volume) -> str:
        devices = volume.Devices
        agent_id = random.choice(devices).AgentId
        self.change_agent_state(agent_id, 1, 'start migration test')
        return agent_id

    @common.retry(5, 10, Error)
    def check_migration(self):
        try:
            volume = self.client.describe_volume(disk_id=self.disk_id)
        except ClientError as e:
            raise Error(f'Failed to check migration status for disk:\n{e}')
        return getattr(volume, 'Migrations', False)

    @common.retry(5, 10, Error)
    def check_migration_started(self):
        self.logger.info('Check if migration started')
        if not self.check_migration():
            raise Error('Migration not started')

    @common.retry(5, 10, Error)
    def kill_tablet(self):
        if self.need_kill_tablet:
            self.logger.info(f'Killing tablet <tablet_id={self.tablet_id}>')
            try:
                self.client.execute_action(
                    action='killtablet',
                    input_bytes=str.encode('{"TabletId": %s}' % self.tablet_id))
            except ClientError as e:
                raise Error(f'Failed to kill tablet <tablet_id={self.tablet_id}>:\n{e}>')

        self.kills += 1
        time.sleep(self.sleep_timeout)

    @common.retry(5, 10, Error)
    def change_postponed_weight(self, weight: int, blocks: int):
        self.logger.info(f'Changing max postponed weight for disk <disk_id={self.disk_id}> to <weight={weight}>')
        try:
            self.client.resize_volume(
                disk_id=self.disk_id,
                blocks_count=blocks,
                channels_count=0,
                config_version=None,
                performance_profile=protos.TVolumePerformanceProfile(MaxPostponedWeight=weight))
        except ClientError as e:
            self.logger(e)
            raise Error(f'Failed to change postponed weight to <weight={weight}>:\n{e}')

    def check_timeout(self, start) -> bool:
        if self.args.timeout == -1:
            return True
        return time.time() - start < self.args.timeout

    def run(self):
        self.logger.info('Test started')

        if self.args.check_load:
            self.check_load()

        volume = self.get_volume_description()

        postponed_weight = volume.PerformanceProfile.MaxPostponedWeight
        blocks_count = volume.BlocksCount

        with Migration(self, volume):
            self.check_migration_started()

            start = time.time()
            while self.check_migration() and (self.max_kills == -1 or self.kills < self.max_kills):
                if not self.check_timeout(start):
                    raise Error('Timeout. Stopping test')

                self.kill_tablet()
                if self.kills == 5:
                    self.change_postponed_weight(postponed_weight + 1, blocks_count)

            self.change_postponed_weight(self.__DEFAULT_POSTPONED_WEIGHT, blocks_count)

        if self.args.check_load:
            self.check_load()


def _parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('--teamcity', action='store_true', help='use teamcity logging format')
    parser.add_argument('--dry-run', action='store_true', help='dry run')

    test_arguments_group = parser.add_argument_group('test arguments')
    test_arguments_group.add_argument(
        '--cluster',
        type=str,
        required=True,
        help='run migration test on specified cluster')
    test_arguments_group.add_argument(
        '--ipc-type',
        type=str,
        default='grpc',
        help='run migration test with specified ipc-type')
    test_arguments_group.add_argument(
        '--disk-name',
        type=str,
        required=True,
        help='run migration test for specified non-repl disk')
    test_arguments_group.add_argument(
        '--check-load',
        action='store_true',
        help='if set test will check that eternal load on specified instance not fails after migration'
    )
    test_arguments_group.add_argument(
        '--kill-tablet',
        action='store_true',
        help='if set, test will kill disk tablet during migration'
    )
    test_arguments_group.add_argument(
        '--timeout',
        type=int,
        default=_DEFAULT_MIGRATION_TIMEOUT,
        help='specify test timeout'
    )
    test_arguments_group.add_argument(
        '--kill-period',
        type=int,
        required='--kill-tablet' in sys.argv,
        default=_DEFAULT_KILL_PERIOD,
        help='specify sleep time between kills'
    )
    test_arguments_group.add_argument(
        '--service-account-id',
        type=str,
        required='--cluster preprod' in sys.argv,
        help='specify service-account-id for authorization to nbs server'
    )
    return parser.parse_args()


def main():
    args = _parse_args()
    logger = common.create_logger('yc-nbs-ci-migration-test', args)

    try:
        TestRunner(args, logger).run()
    except Error as e:
        logger.fatal(f'Failed to run test: \n {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()
