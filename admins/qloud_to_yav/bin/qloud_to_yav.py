#!/usr/bin/env python3

import os
import re
import argparse
import subprocess
import requests
try:  # Try arcadia lib
    from library.python.vault_client.instances import Testing, Production
except:  # Try local lib
    from vault_client.instances import Testing, Production


QLOUD_API_PATH = 'https://qloud-ext.yandex-team.ru/api/v1'
QLOUD_API_ENDPOINT_PATH = '/environment/dump'
QLOUD_API_TOKEN = os.environ.get('QLOUD_API_TOKEN', None)
YAV_TOKEN = os.environ.get('YAV_TOKEN', None)

VaultClient = None
suppress_resolv_fail = None
force_create = None
yav_url = ''
testing_arg = ''

secret_data = {}


class Printer(object):
    """
    Colorful printer class.
    Call method before printing desired text to print it in color.
    After, call reset.
    """
    @staticmethod
    def green():
        print('\033[0;32m', end='')

    @staticmethod
    def red():
        print('\033[0;31m', end='')

    @staticmethod
    def yellow():
        print('\033[0;33m', end='')

    @staticmethod
    def reset():
        print('\033[0m', end='')


def run_remote_cmd_on_instance(instance: str, cmd: str) -> str:
    sub_command = ['ssh', instance, cmd]
    try:
        output = subprocess.check_output(sub_command, stderr=subprocess.DEVNULL)
    except Exception as e:
        raise Exception(
            'Error at ssh call. Please check that instance is active and accessible from current working machine (check Puncher rules). Error: %s' % e,
        )
    return output


def get_hostname_by_component(component: str) -> str:
    try:
        global suppress_resolv_fail
        qloud_tree = {}
        qloud_list = component.split('.')
        assert len(qloud_list) == 4
        qloud_tree['component'] = qloud_list[-1]
        qloud_tree['environment'] = qloud_list[-2]
        qloud_tree['app'] = qloud_list[-3]
        qloud_tree['project'] = qloud_list[-4]
        host = '{}-1.{}.{}.{}.{}.{}'.format(qloud_tree['component'],
                                            qloud_tree['component'],
                                            qloud_tree['environment'],
                                            qloud_tree['app'],
                                            qloud_tree['project'],
                                            'stable.qloud-d.yandex.net')
        rc = subprocess.call(['host', host], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if suppress_resolv_fail and rc == 1:
            return None
        assert rc == 0
        return host
    except Exception as e:
        raise Exception('cannot found running instance for %s component\n %s' % (component, e))


def get_env_vars(hostname: str, component):
    env_output = run_remote_cmd_on_instance(hostname, 'cat /proc/$(pidof qloud-init -o 1)/environ')
    env_lst = env_output.split(b'\0')
    env_map = {}
    result = {}
    for env_item in env_lst:
        if env_item:
            k, v = env_item.decode().split('=', 1)
            env_map[k] = v
    for k, v in secret_data.items():
        if v['component'] == component:
            v['value'] = env_map.get(v['target'], None)  # .get because if can also be a file path


def assign_secret_meta_from_qloud(component: str):
    assert len(component.split('.')) == 4
    global secret_data
    environment = '.'.join(component.split('.')[:-1])
    headers = {'Authorization': 'OAuth {}'.format(QLOUD_API_TOKEN)}
    r = requests.get('{}{}/{}'.format(QLOUD_API_PATH, QLOUD_API_ENDPOINT_PATH, environment), headers=headers)
    for c in r.json()['components']:
        if c['componentName'] == component.split('.')[-1]:
            secrets = c['secrets']
            for s in secrets:
                id = s['objectId']
                if not secret_data.get(id) and s['used']:
                    if id.startswith('secret.'):
                        id = re.sub(r"^secret\.", "", id)
                    secret_data[id] = {}
                    secret_data[id]['target'] = s['target']
                    secret_data[id]['value'] = ''
                    secret_data[id]['component'] = component


def check_if_file(hostname, filename) -> bool:
    return 'File' in run_remote_cmd_on_instance(hostname, 'stat {}'.format(filename)).decode()


def get_secret_files(hostname, c):
    global secret_data
    for k, v in secret_data.items():
        if not v['value'] and check_if_file(hostname, v['target']):
            v['value'] = run_remote_cmd_on_instance(hostname, 'cat {}'.format(v['target'])).decode()
            v['type'] = 'file'
            with open('/tmp/{}'.format(k), 'w') as __fd__:
                __fd__.write(v['value'])


def link_secret_data(component: str):
    hostname = get_hostname_by_component(component)
    if hostname:
        assign_secret_meta_from_qloud(component)
        get_env_vars(hostname, component)
        get_secret_files(hostname, component)


def link_secret_data_for_environment(environment: str):
    headers = {'Authorization': 'OAuth {}'.format(QLOUD_API_TOKEN)}
    r = requests.get('{}{}/{}'.format(QLOUD_API_PATH, QLOUD_API_ENDPOINT_PATH, environment), headers=headers)
    components = []
    for c in r.json()['components']:
        components.append('{}.{}'.format(environment, c['componentName']))
    for c in components:
        link_secret_data(c)


def add_role(client, secret_uuid, who, abc_id=None, abc_scope=None, login=None) -> None:
    client.add_user_role_to_secret(
        secret_uuid,
        who,
        abc_scope=abc_scope,
        abc_id=abc_id,
        login=login
    )


def create_yav_secret(name: str, yav_args: dict) -> str:
    if YAV_TOKEN:
        client = VaultClient(decode_files=True, authorization='OAuth {}'.format(YAV_TOKEN))
    else:
        client = VaultClient(decode_files=True)
    if secret_data or force_create:
        secret_uuid = client.create_secret(name)
        version_secret_data = {}
        for k, v in secret_data.items():
            if v.get('type') != 'file':
                version_secret_data[k] = v['value']
        if version_secret_data:
            version_uuid = client.create_secret_version(
                secret_uuid,
                version_secret_data
            )
        for k, v in secret_data.items():
            if v.get('type') == 'file':
                version_secret_data[k] = v['value']
                subprocess.check_call('ya vault create version {} -u -f /tmp/{} {}'.format(secret_uuid, k, testing_arg),
                                      shell=True,
                                      stdout=subprocess.DEVNULL,
                                      stderr=subprocess.DEVNULL)
        for k, v in yav_args.items():
            if 'ro_users' in k:
                for user in v:
                    add_role(client, secret_uuid, 'reader', login=user)
            if 'rw_users' in k:
                for user in v:
                    add_role(client, secret_uuid, 'owner', login=user)
            if 'ro_acl_abc' in k:
                for abc_scope in v:
                    add_role(client, secret_uuid, 'reader', abc_id=abc_scope['id'], abc_scope=abc_scope['scope'])
            if 'rw_acl_abc' in k:
                for abc_scope in v:
                    add_role(client, secret_uuid, 'owner', abc_id=abc_scope['id'], abc_scope=abc_scope['scope'])
        Printer.green()
        print('{}/secret/{}'.format(yav_url, secret_uuid))
        Printer.reset()
    else:
        Printer.yellow()
        print('Seems like {} has no secrets used in it\'s component config. Not creating this secret.'.format(name))
        Printer.reset()


def switch_yav_env(use_prod_yav: bool):
    global VaultClient, yav_url, testing_arg
    if use_prod_yav:
        VaultClient = Production
        yav_url = 'https://yav.yandex-team.ru'
        testing_arg = ''
    else:
        VaultClient = Testing
        yav_url = 'https://yav-test.yandex-team.ru'
        testing_arg = '--testing'


def parse_yav_args(args) -> dict:
    res = {}
    if args.ro_acl_users:
        res['ro_users'] = list(args.ro_acl_users)
    if args.ro_acl_abc:
        res['ro_acl_abc'] = []
        all_abc_lsts = [a.split(',') for a in args.ro_acl_abc]
        for lst in all_abc_lsts:
            res['ro_acl_abc'].append({
                'id': int(lst[0]),
                'scope': lst[1]
                })
    if args.rw_acl_users:
        res['rw_users'] = list(args.rw_acl_users)
    if args.rw_acl_abc:
        res['rw_acl_abc'] = []
        all_abc_lsts = [a.split(',') for a in args.rw_acl_abc]
        for lst in all_abc_lsts:
            res['rw_acl_abc'].append({
                'id': int(lst[0]),
                'scope': lst[1]
                })
    if not args.ro_acl_users and not args.ro_acl_abc and not args.rw_acl_users\
            and not args.rw_acl_abc and not args.use_prod_yav:
        Printer.red()
        print('Error parsing arguments for Yav, so only printing secrets itself..')
        Printer.reset()
        return False
    return res


def parse_args():
    parser = argparse.ArgumentParser(description='Qloud app secret to yav with proper acl')
    # parser.add_argument('-v', '--verbose', action='store_true', dest='verbose_output')
    sub_parsers = parser.add_subparsers(dest='action')
    create_parser = sub_parsers.add_parser('create')

    create_parser.add_argument(
        '--use-prod-yav', action='store_true', dest='use_prod_yav',
        help='Use production yav (https://yav.yandex-team.ru/) instad of testing (https://yav-test.yandex-team.ru/)'
    )
    create_parser.add_argument(
        '--suppress-resolv-fail', action='store_true', dest='suppress_resolv_fail',
        help='If for example component is empty (no hosts), resolv will fail. This option allows to ignore it'
    )
    create_parser.add_argument(
        '--force-create', action='store_true', dest='force_create',
        help='If component contains no secrets, create empty secret.'
    )
    create_parser.add_argument(
        '--sec-name', action='store', type=str, dest='sec_name',
        help='Name of the new secret'
    )
    create_parser.add_argument(
        '--allow-ro-users', action='store', type=str, dest='ro_acl_users', nargs='+',
        help='To who allow read permissions. List of user[s]'
    )
    create_parser.add_argument(
        '--allow-ro-abc', action='store', type=str, dest='ro_acl_abc', nargs='+',
        help='To which ABC_ID(int) ABC_SCOPE(str) allow read permissions List like this: 645 administration'
    )
    create_parser.add_argument(
        '--allow-rw-users', action='store', type=str, dest='rw_acl_users', nargs='+',
        help='To who allow read + write permissions'
    )
    create_parser.add_argument(
        '--allow-rw-abc', action='store', type=str, dest='rw_acl_abc', nargs='+',
        help='To which ABC_ID(int) ABC_SCOPE(str) allow read+write permissions List like this: 645 administration'
    )
    create_parser.add_argument(
        '--qloud-component', action='store', type=str, dest='component',
        help='Qloud component name. e.g. `kinopoisk.quiz-and-questionnaire.testing.quiz-and-questionnaire-api`'
    )
    create_parser.add_argument(
        '--qloud-environment', action='store', type=str, dest='environment',
        help='Qloud environment name. e.g. `kinopoisk.quiz-and-questionnaire.production`'
    )

    return parser, parser.parse_args()


def main():
    global suppress_resolv_fail
    global force_create
    parser, args = parse_args()
    try:
        suppress_resolv_fail = args.suppress_resolv_fail
        force_create = args.force_create
        switch_yav_env(args.use_prod_yav)
    except Exception:
        pass
    if args.action == 'create':
        if (args.component is not None) ^ (args.environment is not None):
            if args.component is not None:
                link_secret_data(args.component)
                # print(secret_data)
                # print()
                yav_args_dict = parse_yav_args(args)
                if yav_args_dict:
                    create_yav_secret(args.sec_name, yav_args_dict)
                else:
                    print(secret_data)
            elif args.environment is not None:
                link_secret_data_for_environment(args.environment)
                # print(secret_data)
                # print()
                yav_args_dict = parse_yav_args(args)
                if yav_args_dict:
                    create_yav_secret(args.sec_name, yav_args_dict)
                else:
                    print(secret_data)
    else:
        Printer.red()
        print('Error in arguments')
        Printer.reset()


main()


# END
