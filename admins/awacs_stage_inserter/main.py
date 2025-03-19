#!/usr/bin/env python

import os
import argparse
import time
import getpass
import json
import library.python.oauth as lpo

from src.AwacsBackend import AwacsBackend
from src.AwacsUpstream import AwacsUpstream
from src.AwacsDomain import AwacsDomain
from src.logging_config import LoggingConfig

from nanny_rpc_client.exceptions import NotFoundError

AWACS_URL = os.getenv('AWACS_URL', 'https://awacs.yandex-team.ru/api/')
AWACS_TOKEN = os.getenv('AWACS_TOKEN',
                        lpo.get_token('1f6ac4e0d6a8431fae44b513c84439ca',
                                      '082c95f2392947ca9155f33af4d298fe', login=getpass.getuser()))


logging_config = LoggingConfig(__name__)
logger = logging_config.get_logger()


class UnstablesConfig(object):
    def __init__(self, **kwargs):
        if kwargs.get('path') is not None and os.path.exists(kwargs['path']):
            self.read_config(kwargs['path'])
        del kwargs['path']
        self.token = AWACS_TOKEN
        self.url = AWACS_URL
        self.merge_config_with_args(kwargs['args'])

    def read_config(self, path):
        with open(path, 'r') as __fd__:
            self.__dict__.update(json.load(__fd__))

    def merge_config_with_args(self, args):
        for param_key, param_value in args.items():
            if param_value:
                setattr(self, param_key, param_value)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--action', required=False, type=str, help='Action to do:'
                        '`insert_unstable`, OR `remove_unstable`. Note the domain will not be deleted! It sould be created beforehand.'
                        'If it already exists, an upstream will be inserted into its\' upstream_list')
    parser.add_argument('-e', '--endpoint-set', required=False, type=str, help='Endpoint set id: stage_name.deploy_unit')
    parser.add_argument('-n', '--namespace', required=False, type=str, help='AWACS namespace_id')
    parser.add_argument('-d', '--domain', required=False, type=str, help='Domain in which upstream will be inserted')
    parser.add_argument('-c', '--config', required=False, type=str, help='Path to json config')
    parser.add_argument('--upstream-config', required=False, type=str, help='Path to custom YAML upstream config')
    parser.add_argument('-abc', '--abc-id', required=False, type=str, help='NOT NEEDED for unstables. Used only for insert_stage. ABC ID (645 for kp) for certificate order.')
    parser.add_argument('-dc', '--datacenters', required=False, type=str, nargs='+', help='Datacetners where deploy_unit\'s pods are e.g.: sas man iva')
    parser.add_argument('-f', '--fqdns', required=False, type=str, nargs='+', help='FQDNs for service, e.g.: voting-system-api.kp.yandex.net voting-system.kp.yandex.net')
    parser.add_argument('-g', '--guids', required=False, type=int, nargs='+', help='Groups ids from staff-api (e.g. 61458,31277 (https://staff-api.yandex-team.ru/v3/groups?id=61458,31277))')
    parser.add_argument('-u', '--users', required=False, type=str, nargs='+', help='Usernames from staff to extend ACL attributes (e.g. `robot-kp-java`)')
    parser.add_argument('--skip-domain-upstream-actions', required=False, action='store_true', default=False, help='Do not add/remove upstream to/from the domain explicitly'
                        ' (for "include all upstreams" awacs domain option)')
    # parser.add_argument('-w', '--wait', required=False, action='store_true', help='Should I wait untill full balancer config apply?'
    #                     'This can take a while. Without this flag, just send command to awacs and do not wait untill config apply.')
    args = parser.parse_args()

    return args


def create_backends(config):
    for dc in config.datacenters:
        backend_id = '{}_{}'.format(config.endpoint_set.replace('.', '_'), dc)
        if config.__dict__.get('backends_by_dc') is None:
            config.backends_by_dc = {}
        config.backends_by_dc[dc] = backend_id
        logger.info('Generated backend ID: {}'.format(backend_id))
        awacs_backend = AwacsBackend(config, backend_id, dc)
        try:
            resp_pb = awacs_backend.get_backend()
            logger.info('Backend {} already exists'.format(backend_id))
        except NotFoundError:
            logger.info('Backend {} not exists, creating...'.format(backend_id))
            awacs_backend.create_backend()
            # Api slow
            time.sleep(2)
            resp_pb = awacs_backend.get_backend()
            logger.info('Created backend:\n{}'.format(resp_pb.backend))


def create_upstream(config):
    logger.info(config.backends_by_dc)
    upstream_id = config.endpoint_set.replace('.', '_')
    logger.info(upstream_id)
    config.upstream_id = upstream_id
    awacs_upstream = AwacsUpstream(config)
    try:
        resp_pb = awacs_upstream.get_upstream()
        logger.info('Upstream {} already exists'.format(upstream_id))
    except NotFoundError:
        logger.info('Upstream {} not exists, creating...'.format(upstream_id))
        awacs_upstream.create_upstream()
        # Api slow
        time.sleep(2)
        resp_pb = awacs_upstream.get_upstream()
        logger.info('Created upstream:\n{}'.format(resp_pb.upstream))


def insert_upstream_in_domain(config):
    skip_domain_upstream_actions = config.__dict__.get('skip_domain_upstream_actions')
    if skip_domain_upstream_actions is True or skip_domain_upstream_actions == 'True':
        logger.info('Skipping inserting upstream in domain')
        return
    logger.info('Start work with domain.')
    awacs_domain = AwacsDomain(config)
    current_revision = awacs_domain.get_domain().domain.meta.version
    rc = awacs_domain.insert_upstream_in_domain()
    if rc:
        logger.info('Inserting upstream in domain is IN PROGRESS.')
        logger.info('You can watch the process here: https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/{}/show/'.format(config.namespace))
        new_revision = awacs_domain.get_domain().domain.meta.version
        while current_revision == new_revision:
            new_revision = awacs_domain.get_domain().domain.meta.version
            logger.debug('Waiting for domain changes to initialize balancer deploy')
            time.sleep(1)
        logger.debug('New domain revision is {}. Started validating config, and soon the balancer will start deploying'.format(new_revision))
        return new_revision


def create_domain(config):
    logger.info('Start work with domain.')
    config.upstream_id = config.endpoint_set.replace('.', '_')
    awacs_domain = AwacsDomain(config)
    try:
        resp_pb = awacs_domain.get_domain()
        logger.info('Domain {} already exists'.format(config.domain))
    except NotFoundError:
        logger.info('Domain {} not exists, creating...'.format(config.domain))
        awacs_domain.create_domain()
        # Api slow
        time.sleep(2)
        resp_pb = awacs_domain.get_domain()
        logger.info('Created domain:\n{}'.format(resp_pb.domain))
        logger.info('APPLYING CHANGES in balancer is IN PROGRESS.')
        logger.info('You can watch the process here: https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/{}/show/'.format(config.namespace))


def remove_backends(config):
    for dc in config.datacenters:
        backend_id = '{}_{}'.format(config.endpoint_set.replace('.', '_'), dc)
        logger.info('Going to remove backend with id {}'.format(backend_id))
        awacs_backend = AwacsBackend(config, backend_id, dc)
        try:
            resp_pb = awacs_backend.get_backend()
            awacs_backend.remove_backend()
            # Api slow
            time.sleep(2)
            logger.info('Removed backend: {}'.format(resp_pb.backend.meta.id))
        except NotFoundError:
            logger.info('Backend {} not exists, not deleting!...'.format(backend_id))


def remove_upstream(config):
    upstream_id = config.endpoint_set.replace('.', '_')
    logger.info('Going to remove upstream {}'.format(upstream_id))
    config.upstream_id = upstream_id
    awacs_upstream = AwacsUpstream(config)
    try:
        _ = awacs_upstream.get_upstream()
        logger.info('Upstream {} exists, removing...'.format(upstream_id))
        awacs_upstream.remove_upstream()
        # Api slow
        time.sleep(2)
        logger.info('Removed upstream:\n{}'.format(upstream_id))
    except NotFoundError:
        logger.info('Upstream {} not exists, not removing...'.format(upstream_id))


def remove_upstream_from_domain(config):
    if config.__dict__.get('skip_domain_upstream_actions') is True:
        logger.info('Skipping removing upstream from domain')
        return
    logger.info('Start work with domain.')
    config.upstream_id = config.endpoint_set.replace('.', '_')
    awacs_domain = AwacsDomain(config)
    current_revision = awacs_domain.get_domain().domain.meta.version
    rc = awacs_domain.remove_upstream_from_domain()
    if rc:
        logger.info('Removing upstream {} from this domain is IN PROGRESS.'.format(config.upstream_id))
        logger.info('You can watch the process here: https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/{}/show/'.format(config.namespace))
        new_revision = awacs_domain.get_domain().domain.meta.version
        while current_revision == new_revision:
            new_revision = awacs_domain.get_domain().domain.meta.version
            logger.debug('Waiting for domain changes to initialize balancer deploy')
            time.sleep(1)
        logger.debug('New domain revision is {}. Started validating config, and soon the balancer will start deploying'.format(new_revision))
        return new_revision


def get_latest_domain_revision(config):
    awacs_domain = AwacsDomain(config)
    current_revision = awacs_domain.get_domain().domain.meta.version
    return current_revision


def wait_for_balancer_deploy(config, revision, force=False):
    balancer_deploy_finished = False
    domain_revision = revision
    was_in_progress_at_least_once = force
    while not balancer_deploy_finished:
        logger.debug('....Getting status update from awacs....')
        awacs_domain = AwacsDomain(config)
        domain = awacs_domain.get_domain().domain
        all_domain_revisions = []
        for d in domain.statuses:
            all_domain_revisions.append(d.id)
            if d.id == domain_revision:
                validated = {}
                in_progress = {}
                active = {}
                for k, v in dict(d.validated).items():
                    validated[k] = v.status
                for k, v in dict(d.in_progress).items():
                    in_progress[k] = v.status
                for k, v in dict(d.active).items():
                    active[k] = v.status

                total_validated = len(active.keys())
                total_in_progress = len(active.keys())
                total_active = len(active.keys())

                logger.debug('Need to have 0 in IN_PROGRESS; AND {} of {} in ACTIVE!'.format(total_active, total_active))

                current_total_validated = sum(value == 'True' for value in validated.values())
                current_total_in_progress = sum(value == 'True' for value in in_progress.values())
                current_total_active = sum(value == 'True' for value in active.values())

                if not was_in_progress_at_least_once:
                    if current_total_in_progress > 0:
                        was_in_progress_at_least_once = True

                logger.debug('VALIDATED: {} out of {}'.format(current_total_validated, total_validated))
                logger.debug('IN_PROGRESS: {} out of {}'.format(current_total_in_progress, total_in_progress))
                logger.debug('ACTIVE: {} out of {}'.format(current_total_active, total_active))

                time.sleep(2)

                if was_in_progress_at_least_once and current_total_active == total_active and current_total_in_progress == 0 and current_total_active != 0:
                    logger.debug('Finished deploying this revision!')
                    balancer_deploy_finished = True
                else:
                    logger.debug('...Balancer config is still deploying...')
        # If too many changes (multiple PRs at once)
        #
        if domain_revision not in all_domain_revisions:
            logger.debug('Finished deploying this revision.')
            balancer_deploy_finished = True
            wait_for_balancer_deploy(config, get_latest_domain_revision(config), force=True)


def main():
    args = parse_args()
    if args:
        logger.info('Program started')
        unstables_config = UnstablesConfig(path=args.config, args=args.__dict__)
        if unstables_config.action and unstables_config.action not in ['insert_unstable', 'remove_unstable', 'insert_stage']:
            logger.error('Action should be either `insert_unstable` or `remove_unstable`. Or maybe `insert_stage`')
            return

        logger.debug('Current config:')
        tmp_token = unstables_config.token
        unstables_config.token = 'AQAD-***************************'
        logger.debug(unstables_config.__dict__)
        unstables_config.token = tmp_token

        # Depending on action, remove or insert unstable.
        #
        if unstables_config.action == 'insert_unstable':
            create_backends(unstables_config)
            create_upstream(unstables_config)
            _ = insert_upstream_in_domain(unstables_config)
            # domain_revision = insert_upstream_in_domain(unstables_config)
        elif unstables_config.action == 'remove_unstable':
            # domain_revision = remove_upstream_from_domain(unstables_config)
            _ = remove_upstream_from_domain(unstables_config)
            remove_upstream(unstables_config)
            remove_backends(unstables_config)
        elif unstables_config.action == 'insert_stage':
            create_backends(unstables_config)
            create_upstream(unstables_config)
            create_domain(unstables_config)

        # if unstables_config.__dict__.get('wait') is True or unstables_config.__dict__.get('wait') == 'True':
        #     if domain_revision is None:
        #         domain_revision = get_latest_domain_revision(unstables_config)
        #         wait_for_balancer_deploy(unstables_config, domain_revision, force=True)
        #     else:
        #         wait_for_balancer_deploy(unstables_config, domain_revision)


if __name__ == '__main__':
    main()

# EOF
