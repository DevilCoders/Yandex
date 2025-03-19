import argparse
import os
import requests
import sys

from cloud.blockstore.pylibs import common


class Error(Exception):
    pass


def _parse_args():
    parser = argparse.ArgumentParser()

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument('-v', '--verbose', action='store_true')
    verbose_quite_group.add_argument('-q', '--quite', action='store_true')

    parser.add_argument('--teamcity', action='store_true', help='use teamcity logging format')

    sign_group = parser.add_argument_group('sign arguments')
    sign_group.add_argument('-t', '--task-id', type=str, required=True, help='teamcity SignPackages task id')
    sign_group.add_argument('-p', '--project-id', type=str, required=True, help='teamcity SignPackages project id')

    args = parser.parse_args()

    return args


def _sign_packages_via_teamcity(args):
    token = os.getenv('TEAMCITY_OAUTH_TOKEN')
    if token is None:
        raise Error('no TEAMCITY_OAUTH_TOKEN specified')

    data = {
        'buildType': {
            'id': args.task_id,
            'projectId': args.project_id,
        }
    }
    headers = {
        'Authorization': 'Bearer %s' % token,
        'Content-Type': 'application/json',
        'Accept': 'application/json',
    }

    response = requests.post(
        'https://teamcity.yandex-team.ru/app/rest/buildQueue',
        json=data,
        headers=headers,
    )
    response.raise_for_status()


def main():
    args = _parse_args()
    logger = common.create_logger('yc-nbs-ci-sign-packages', args)

    try:
        _sign_packages_via_teamcity(args)
    except Error as e:
        logger.fatal(f'Failed to sign packages: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()
