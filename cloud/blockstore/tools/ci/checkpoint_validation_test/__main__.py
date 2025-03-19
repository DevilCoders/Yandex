import argparse
import jinja2
import sys

from .lib import (
    get_test_config,
    Error
)

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.sdk.client import make_client
from cloud.blockstore.public.sdk.python.client import ClientCredentials, ClientError
from cloud.blockstore.pylibs.ycp import Ycp, make_ycp_engine

from library.python import resource


def get_config_template(name: str) -> jinja2.Template:
    return jinja2.Template(resource.find(name).decode('utf8'))


class TestRunner:
    _VALIDATE_CHECKPOINT_CMD = ('%s %s --disk-id=%s --checkpoint-id=%s'
                                ' --read-all --io-depth=32 --config %s --throttling-disabled'
                                ' --client-performance-profile %s')
    _CLIENT_CONFIG_FILE = 'nbs-client.txt'
    _THROTTLER_CONFIG_FILE = 'throttler-profile.txt'
    _NBS_PORT = {
        'preprod': 9767,
        'hw-nbs-stable-lab': 9766
    }
    _NBS_SECURE_PORT = {
        'preprod': 9768,
        'hw-nbs-stable-lab': None
    }

    def __init__(self, args, logger):
        self.args = args
        self.logger = logger
        self.ycp = Ycp(args.cluster, logger, make_ycp_engine(args.dry_run))
        self.helpers = common.make_helpers(args.dry_run)
        self.test_config = get_test_config(self.ycp, self.helpers, args, logger)

        port = self._NBS_PORT[args.cluster]
        credentials = None
        self.token = None
        if args.cluster == 'preprod':
            self.token = self.ycp.create_iam_token(args.service_account_id)
            credentials = ClientCredentials(auth_token=self.token.iam_token)
            port = self._NBS_SECURE_PORT[args.cluster]

        try:
            self.logger.info(f'Creating connection to compute_node=<{self.test_config.instance.compute_node}>')
            self.client = make_client(
                args.dry_run,
                endpoint='{}:{}'.format(self.test_config.instance.compute_node, port),
                credentials=credentials)
        except ClientError as e:
            raise Error('Failed to create grpc client' + e.message)

    def create_file(self, out: str):
        tmp_file = self.helpers.create_tmp_file()
        tmp_file.write(bytes(out, 'utf8'))
        tmp_file.flush()
        return tmp_file

    def create_checkpoint(self):
        self.logger.info(f'Creating checkpoint <id={self.test_config.checkpoint_id}>'
                         f' on disk <id={self.test_config.disk.id}>')
        self.client.create_checkpoint(self.test_config.disk.id, self.test_config.checkpoint_id)
        self.logger.info('Checkpoint successfully created')

    def delete_checkpoint(self, checkpoint_id: str):
        self.logger.info(f'Deleting checkpoint <id={checkpoint_id}>'
                         f' on disk <id={self.test_config.disk.id}>')
        self.client.delete_checkpoint(self.test_config.disk.id, checkpoint_id)
        self.logger.info('Checkpoint successfully deleted')

    def validate_checkpoint(self, eternal_config: str, nbs_client_config: str, throttler_config: str):
        self.logger.info('Validating checkpoint')
        cmd = self._VALIDATE_CHECKPOINT_CMD % (
            self.args.validator_path,
            eternal_config,
            self.test_config.disk.id,
            self.test_config.checkpoint_id,
            nbs_client_config,
            throttler_config
        )
        result = self.helpers.make_subprocess_run(cmd)
        if result.returncode:
            raise Error(f'Validator failed with exit_code=<{result.returncode}>.\n'
                        f'Stderr: {result.stderr}')
        self.logger.info('Validation finished')

    def find_checkpoints(self) -> [str]:
        stat = self.client.stat_volume(disk_id=self.test_config.disk.id)
        return stat['Checkpoints']

    def run_test(self):
        checkpoints = self.find_checkpoints()
        self.logger.info(f'Found {len(checkpoints)} checkpoints')
        self.logger.info('Deleting existing checkpoints')
        for c in checkpoints:
            self.delete_checkpoint(c)

        eternal_config = self.create_file(self.test_config.config_json)
        token_file = self.create_file(self.token.iam_token) if self.token is not None else None
        nbs_client_config = self.create_file(get_config_template(self._CLIENT_CONFIG_FILE).render(
            host=self.test_config.instance.compute_node,
            port=self._NBS_PORT[self.args.cluster],
            securePort=self._NBS_SECURE_PORT[self.args.cluster],
            tokenPath=token_file.name if self.token else None,
        ))
        throttler_config = self.create_file(get_config_template(self._THROTTLER_CONFIG_FILE).render(
            maxReadBandwidth=self.args.max_read_bandwidth * 1024 ** 2
        ))

        self.logger.info(f'Running checkpoint validation test on cluster <{self.args.cluster}>')
        try:
            self.create_checkpoint()
            self.validate_checkpoint(eternal_config.name, nbs_client_config.name, throttler_config.name)
        finally:
            self.delete_checkpoint(self.test_config.checkpoint_id)


def parse_args():
    parser = argparse.ArgumentParser()

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument('-v', '--verbose', action='store_true')
    verbose_quite_group.add_argument('-q', '--quite', action='store_true')

    parser.add_argument('--teamcity', action='store_true', help='use teamcity logging format')
    parser.add_argument('--dry-run', action='store_true', help='dry run')

    test_arguments_group = parser.add_argument_group('test arguments')
    test_arguments_group.add_argument(
        '-c',
        '--cluster',
        type=str,
        required=True,
        help='run corruption test on specified cluster')
    test_arguments_group.add_argument(
        '--validator-path',
        type=str,
        required=True,
        help='specify path to checkpoint validator')
    test_arguments_group.add_argument(
        '--max-read-bandwidth',
        type=int,
        default=300,
        help='specify max read bandwidth in MB/s')
    test_arguments_group.add_argument(
        '--service-account-id',
        type=str,
        required='--cluster preprod' in sys.argv,
        help='specify path to checkpoint validator')

    return parser.parse_args()


def main():
    args = parse_args()
    logger = common.create_logger('yc-nbs-ci-validate-checkpoint-test', args)

    try:
        TestRunner(args, logger).run_test()
    except Exception as e:
        logger.fatal(f'Test failed: {e}')
        sys.exit(1)
    logger.info('Test finished successfully')


if __name__ == '__main__':
    main()
