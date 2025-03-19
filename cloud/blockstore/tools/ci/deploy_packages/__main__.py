import argparse
import json
import sys

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.clusters import get_cluster_update_config

from .lib import Z2Helper, Error


def _parse_args():
    parser = argparse.ArgumentParser()

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument('-v', '--verbose', action='store_true')
    verbose_quite_group.add_argument('-q', '--quite', action='store_true')

    parser.add_argument('--teamcity', action='store_true', help='use teamcity logging format')

    deploy_group = parser.add_argument_group('deploy arguments')
    deploy_group.add_argument('-c', '--cluster', type=str, required=True, help='deploy packages on specified cluster')
    deploy_group.add_argument('-s', '--service', type=str, required=True, help='deploy packages for specified service')
    deploy_group.add_argument('-p', '--packages', type=str, required=True, action='append', help='file containing json list of packages to deploy')

    args = parser.parse_args()

    return args


def _deploy_packages(args, logger):
    cluster = get_cluster_update_config(args.cluster, args.service)
    z2 = Z2Helper(cluster, logger)

    for path in args.packages:
        with open(path, 'r') as f:
            z2.edit_package_versions(json.load(f), args.service)
            z2.run_update()


def main():
    args = _parse_args()
    logger = common.create_logger('yc-nbs-ci-deploy-packages', args)

    try:
        _deploy_packages(args, logger)
    except Error as e:
        logger.fatal(f'Failed to deploy packages: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()
