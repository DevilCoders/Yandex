#!/skynet/python/bin/python
"""Script to manipulate with cached nanny services (RX-354)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
import urllib2
import json
import requests
import time

import gencfg
import config
import gaux.aux_abc
import gaux.aux_decorators
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.card.node import CardNode
from core.settings import SETTINGS
from gaux.aux_utils import retry_urlopen
from gaux.aux_colortext import red_text


class EActions(object):
    """All action to perform on ipv4 tunnels"""
    DOWNLOAD = 'download'  # download nanny services info to specified file
    SYNC_GROUPS = 'sync_groups'  # update group nanny sevices info (list of nanny services, corresponding to group)
    ALL = [DOWNLOAD, SYNC_GROUPS]


def get_parser():
    parser = ArgumentParserExt(description='Manipulate with nanny services')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument('-o', '--output-file', type=str, default=None,
                        help='Optional. File to write nanny services info (for <{}> action)'.format(EActions.DOWNLOAD))
    parser.add_argument('--abc-oauth-token', type=str, default=None,
                        help='Optional. Oauth token to access abc (obligatory for <{}> action)'.format(EActions.DOWNLOAD))
    parser.add_argument('-g', '--groups', type=argparse_types.groups, default='ALL',
                        help='Optional. Sync nanny services for specified groups (for action <{}>'.format(EActions.SYNC_GROUPS))
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")

    return parser


def normalize(options):
    if options.action == EActions.DOWNLOAD:
        options.abc_oauth_token = options.abc_oauth_token or os.environ.get('GENCFG_DEFAULT_OAUTH')
        if options.abc_oauth_token is None:
            raise Exception('You must specify <--abc-oauth-token> param for action <{}>'.format(options.action))


def request_nanny(url):
    headers = { 'Authorization': 'OAuth {}'.format(config.get_default_oauth()) }
    request = urllib2.Request(url, None, headers)

    return json.loads(retry_urlopen(5, request))


@gaux.aux_decorators.memoize
def resolve_nanny_group(group_name, abc_oauth_token):
    """Owners of nanny groups could be staff groups or abc groups"""

    try:
        headers = {
            'Authorization': 'OAuth {}'.format(abc_oauth_token),
            'Content-Type': 'application/json',
            'Accept': 'application/json',
        }

        # check for abc group owners
        response = requests.get('https://staff-api.yandex-team.ru/v3/groups', headers=headers, params=dict(id=group_name))
        if response.status_code == 200:
            response_json = response.json()
            if len(response_json['result']):
                group_details = response_json['result'][0]
                if group_details.get('service', {}).get('id'):
                    aa = gaux.aux_abc.resolve_abc_service(group_details['service']['id'], abc_oauth_token)
                    return aa

        # check for staff group owners
        params = {'department_group.id': group_name}
        response = requests.get('https://staff-api.yandex-team.ru/v3/persons', headers=headers, params=params)
        if response.status_code == 200:
            return [x['login'] for x in response.json()['result']]
        elif response.status_code == 400:
            return []
        else:
            raise Exception('Got code <{}> when requesting <{}> with params <{}>'.format(response.status_code, 'https://staff-api.yandex-team.ru/v3/groups', params))
    except Exception as e:
        return []


#def update_nanny_service(options, nanny_service):
def get_nanny_service_info(service_name, abc_oauth_token):
    # get list of gencfg groups
    used_groups = []
    instances_url = '{}{}{}/runtime_attrs/instances/'.format(SETTINGS.services.nanny.rest.url, SETTINGS.services.nanny.rest.path.services, service_name)
    data = request_nanny(instances_url)
    # Non-gencfg hosts owners must not be exported to CAuth to avoid dom0 users access: https://st.yandex-team.ru/YP-403
    if data['content']['chosen_type'] != 'EXTENDED_GENCFG_GROUPS':
        return dict(owners=[], hosts=[], groups=[])

    for group_info in data['content']['extended_gencfg_groups']['groups']:
        used_groups.append(group_info['name'])
    used_groups = sorted(set(used_groups))

    # calculate new user list
    new_users = {'ekilimchuk', 'osol', 'sivanichkin'}
    users_url = '{}{}{}/auth_attrs/'.format(SETTINGS.services.nanny.rest.url, SETTINGS.services.nanny.rest.path.services, service_name)
    auth_data = request_nanny(users_url)['content']
    for role in ('owners', 'conf_managers', 'ops_managers'):
        for login in auth_data[role]['logins']:
            new_users.add(login)
        for group in auth_data[role]['groups']:
            new_users.update(resolve_nanny_group(group, abc_oauth_token))

    # remove non-assciii users
    ascii_users = []
    for user in new_users:
        try:
            ascii_users.append(user.decode('ascii'))
        except UnicodeEncodeError:
            pass

    # calculate new host list
    hosts_url = '{}{}{}/current_state/hosts/'.format(SETTINGS.services.nanny.rest.url, SETTINGS.services.nanny.rest.path.services, service_name)
    hosts = sorted([x['hostname'] for x in request_nanny(hosts_url)['result']])

    return dict(owners=sorted(ascii_users), hosts=hosts, groups=used_groups)


def update_nanny_service(options, nanny_service):
    info = get_nanny_service_info(nanny_service.name, options.abc_oauth_token)
    nanny_service.owners = info['owners']
    nanny_service.hosts = info['hosts']
    nanny_service.parent.mark_as_modified()


def get_nanny_services_names(options):
    result = set()

    tpl_url = '{}{}?limit={{limit}}&skip={{skip}}'.format(SETTINGS.services.nanny.rest.url, SETTINGS.services.nanny.rest.path.services)
    print()
    limit, skip = 200, 0
    while True:
        url = tpl_url.format(limit=limit, skip=skip)

        data = request_nanny(url)

        page_result = set()
        for item in data['result']:
            try:
                if item['current_state']['content']['summary']['value'] != 'OFFLINE':
                    page_result.add(item['_id'])
            except Exception as e:
                print('Invalid format for item `{}`: {}: {}'.format(item.get('_id'), type(e), e))
        result |= page_result

        skip += len(data['result'])
        if not len(data['result']):
            break

    return result


def main_download(options):
    nanny_services_names = get_nanny_services_names(options)
    nanny_services_names = sorted(nanny_services_names)

    result = []
    failed_count = 0
    for index, service_name in enumerate(nanny_services_names):
        print 'Processing service {} ({} of {} total)'.format(service_name, index, len(nanny_services_names))
        try:
            service_info = get_nanny_service_info(service_name, options.abc_oauth_token)
        except Exception as e:
            print '    Got exception {}'.format(e)
            failed_count += 1
            if failed_count > 50:
                raise Exception('Too many failures when requesting nanny services owners')
        else:
            result.append(dict(name=service_name, owners=service_info['owners'], hosts=service_info['hosts'], groups=service_info['groups']))

    with open(options.output_file, 'w') as f:
        f.write(json.dumps(result))


def main_sync_groups(options):
    # request and get all nanny services
    services_info = []
    services_url_tpl = '{}{}'.format(SETTINGS.services.nanny.rest.url, SETTINGS.services.nanny.rest.path.services)
    while True:
        url = '{}?skip={}'.format(services_url_tpl, len(services_info))
        response_services = request_nanny(url)
        if response_services['result']:
            services_info.extend(response_services['result'])
        else:
            break

    # get info for groups
    by_group_data = defaultdict(list)
    for service_info in services_info:
        service_instance_type = service_info['runtime_attrs']['content']['instances']['chosen_type']
        if service_instance_type != 'EXTENDED_GENCFG_GROUPS':  # not a service with gencfg groups
            continue

        service_name = service_info['_id']
        service_status = service_info['current_state']['content']['summary']['value']
        ts = time.localtime(service_info['current_state']['content']['summary']['entered'] / 1000)
        service_last_modified = time.strftime('%Y-%m-%d %H:%M:%S', ts)

        # cycle through groups
        for nanny_group_info in service_info['runtime_attrs']['content']['instances']['extended_gencfg_groups']['groups']:
            group_name = nanny_group_info['name']
            group_tag = nanny_group_info['release']
            by_group_data[group_name].append(dict(name=service_name, status=service_status, last_modified=service_last_modified, tag=group_tag))

    # update groups
    for group in options.groups:
        group.card.nanny.services = []
        for service_info in by_group_data[group.card.name]:
            group.card.nanny.services.append(CardNode.create_from_dict(service_info))
        group.mark_as_modified()

    CURDB.update(smart=True)


def main(options):
    if options.action == EActions.DOWNLOAD:
        return main_download(options)
    elif options.action == EActions.SYNC_GROUPS:
        return main_sync_groups(options)
    else:
        raise Exception('Unknown action {}'.format(options.action))


def print_result(result, options):
    print result


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result, options)
