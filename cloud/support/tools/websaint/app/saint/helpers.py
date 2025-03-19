from datetime import datetime
import requests
import configparser
import json
import os
import logging
from os.path import expanduser
from app.saint.profiles import Profile, WellKnownProfiles
from app.saint.endpoints import Endpoints
from app.saint.printers import Color, get_bytes_size_string
from app import Config


def timestamp_resolve(timestamp):
    return datetime.fromtimestamp(timestamp).isoformat()


def timestamp_resolve_grpc(timestamp):
    return datetime.utcfromtimestamp(int(f'{timestamp}'.split()[1]
                                         .strip())).strftime('%Y-%m-%d %H:%M:%S')


def get_nda_link(link):
    url = 'https://nda.ya.ru/--'
    payload = {'url': link}
    r = requests.post(url, data=payload, verify=False)
    return r.text


def load_profiles_from_config(config: configparser.RawConfigParser):
    try:
        profile_names = config.get('PROFILES', 'names').strip().split()
    except configparser.NoSectionError:
        # For backward compatibility
        profile_names = [WellKnownProfiles.prod.name]

    profiles = {}
    for profile_name in profile_names:
        endpoints_section_name = profile_name.upper() + '.ENDPOINTS'
        solomon_section_name = profile_name.upper() + '.SOLOMON'
        keyauth_section_name = profile_name.upper() + '.AUTHKEYS'

        if not config.has_section(endpoints_section_name) and profile_name == WellKnownProfiles.prod.name:
            # For backward compatibility load profile endpoints from ENDPOINTS section, not from PROD.ENDPOINTS one
            endpoints_section_name = 'ENDPOINTS'

        endpoints = Endpoints(
            priv_iaas_api=config.get(endpoints_section_name, 'priv_iaas_api'),
            priv_identity_api=config.get(endpoints_section_name, 'priv_identity_api'),
            priv_s3_api=config.get(endpoints_section_name, 'priv_s3_api'),
            priv_mdb_api=config.get(endpoints_section_name, 'priv_mdb_api'),
            priv_billing_api=config.get(endpoints_section_name, 'priv_billing_api'),
            pub_mdb_api=config.get(endpoints_section_name, 'pub_mdb_api'),
            pub_compute_api=config.get(endpoints_section_name, 'pub_compute_api'),
        )

        if config.has_section(solomon_section_name):
            instance_usage_link = config.get(solomon_section_name, 'instance_usage_link')
            cluster_usage_link = config.get(solomon_section_name, 'cluster_usage_link')
            disk_dashbord = config.get(solomon_section_name, 'prod_disk_dashbord')
        else:
            instance_usage_link = WellKnownProfiles.prod.solomon_instance_usage_url
            cluster_usage_link = WellKnownProfiles.prod.solomon_cluster_usage_url
            disk_dashbord = WellKnownProfiles.prod.prod_disk_dashbord

        if config.has_section(keyauth_section_name):
            profile_static_keys = {
                'id': Config.key_id,
                'acc_id': Config.account_id,
                'private_key': Config.private_key
            }
        else:
            profile_static_keys = {}
            print('В конфиге нет ключей. Удалите ~/.rei/rei.cfg и инициализируйте файл заново')
            quit()

        profiles[profile_name] = Profile(profile_name,
                                         endpoints,
                                         instance_usage_link,
                                         cluster_usage_link,
                                         disk_dashbord,
                                         profile_static_keys)

    return profiles


# get key for user account
def get_user_input(environment: str):
    home_dir = expanduser('~')

    pem_path = f'{environment}-account-key.json'
    while not os.path.exists(f'{home_dir}/{pem_path}'):
        print(f"""
        Прежде чем начать работу с saint, создай с помощью утилиты yc статический ключ доступа.
        Для этого
        - убедись что у тебя установлен yc и в нём настроен профиль для {Color.red}{environment}{Color.END} окружения
        - выполни следующие команды:

        yc --profile=<твой {Color.red}{environment}{Color.END}-профиль> iam create-token
        yc iam key create --user-account --output ~/{Color.red}{environment}{Color.END}-account-key.json

        """)
        pem_path = str(input(
            f'Где в {Color.green}{home_dir}{Color.END} лежит файл с ключом? [{Color.red}{environment}{Color.END} -account-key.json]: ')).strip() \
                   or f'{environment}-account-key.json'

    try:
        with open(f'{home_dir}/{pem_path}', 'r') as keyfile:
            thekey = json.load(keyfile)
            acc_id = thekey['user_account_id']
            id = thekey['id']
            priv_key = thekey['private_key']
            print(f'{home_dir}/{pem_path} ok')
    except BaseException as e:
        print(e)
        quit()

    return acc_id, id, priv_key


# Checkout dirs and generate config file
def init_config_setup():
    home_dir = expanduser('~')
    config = configparser.RawConfigParser()
    config.read('./rei.cfg'.format(home_dir))

    print('Checkout working directory...')

    if not os.path.exists('{home}/.rei'.format(home=home_dir)):
        print('Creating directory ~/.rei/')
        os.system('mkdir -p {home}/.rei'.format(home=home_dir))
    if not os.path.isfile('{home}/.rei/allCAs.pem'.format(home=home_dir)):
        print('\nTrying download allCAs.pem')
        os.system('wget -q --show-progress https://crls.yandex.net/allCAs.pem -O {home}/.rei/allCAs.pem'.format(
            home=home_dir))
        cert_path = '{home}/.rei/allCAs.pem'.format(home=home_dir)

        if not os.path.isfile('{home}/.rei/allCAs.pem'.format(home=home_dir)):
            print('\nPlease download CA cert from here: https://crls.yandex.net/allCAs.pem')
            cert_path = input('And enter /path/to/allCAs.pem: ')
    else:
        cert_path = '{home}/.rei/allCAs.pem'.format(home=home_dir)

    for endpoints_section_name in ['CA', 'PROFILES', 'AUTHKEYS']:
        try:
            config.add_section(endpoints_section_name)
        except configparser.DuplicateSectionError as e:
            logging.debug(e)

    config.set('CA', 'cert', cert_path)
    config.set('PROFILES', 'names', ' '.join(profile.name for profile in WellKnownProfiles.__all__))

    for profile in WellKnownProfiles.__all__:
        endpoints_section_name = profile.name.upper() + '.ENDPOINTS'
        if not config.has_section(endpoints_section_name):
            config.add_section(endpoints_section_name)

        config.set(endpoints_section_name, 'priv_iaas_api', profile.endpoints.priv_iaas_api)
        config.set(endpoints_section_name, 'priv_identity_api', profile.endpoints.priv_identity_api)
        config.set(endpoints_section_name, 'priv_s3_api', profile.endpoints.priv_s3_api)
        config.set(endpoints_section_name, 'priv_mdb_api', profile.endpoints.priv_mdb_api)
        config.set(endpoints_section_name, 'priv_billing_api', profile.endpoints.priv_billing_api)
        config.set(endpoints_section_name, 'pub_mdb_api', profile.endpoints.pub_mdb_api)
        config.set(endpoints_section_name, 'pub_compute_api', profile.endpoints.pub_compute_api)

        solomon_section_name = profile.name.upper() + '.SOLOMON'
        if not config.has_section(solomon_section_name):
            config.add_section(solomon_section_name)

        config.set(solomon_section_name, 'instance_usage_link', profile.solomon_instance_usage_url)
        config.set(solomon_section_name, 'cluster_usage_link', profile.solomon_cluster_usage_url)
        config.set(solomon_section_name, 'prod_disk_dashbord',
                   profile.prod_disk_dashbord)  # the-nans quickfix solomon issue

        keyauth_section_name = profile.name.upper() + '.AUTHKEYS'
        if not config.has_section(keyauth_section_name):
            config.add_section(keyauth_section_name)
            user_input_account_id, user_input_key_id, user_input_pem = get_user_input(profile.name.lower())
            config.set(keyauth_section_name, 'key_id', user_input_key_id)
            config.set(keyauth_section_name, 'account_id', user_input_account_id)
            config.set(keyauth_section_name, 'account_private_key', user_input_pem)

    with open('{home}/.rei/rei.cfg'.format(home=home_dir), 'w') as cfgfile:
        config.write(cfgfile)

    print('\nDone. You can check your config:\ncat ~/.rei/rei.cfg\n')


def calc_iops(disk_size, disk_type, vcpu=0):
    """
    :param disk_size: gb
    :param disk_type: str: nrd, ssd or hdd
    :param vcpu: int, required for disk_type = nrd only
    :return: result_disk = {
                    max_w_iops,
                    max_r_iops,
                    max_w_bwidth,
                    max_r_bwidth
                    }
    """

    def count_value(blk_size, val, maxval):
        blocks = disk_size // blk_size if disk_size % blk_size == 0 else disk_size // blk_size + 1
        return min(blocks * val, maxval)

    d_size = {
        'blk_size': [32, 256, 93],
        'wmax_iops': [40000, 11000, 50000],
        'rmax_iops': [12000, 300, 50000],
        'wblk_iops': [1000, 300, 5600],
        'rblk_iops': [400, 100, 28000],
        'wbwidth': [450, 240, 1024],
        'wbwidth_blk': [15, 30, 82],
        'rbwidth': [450, 240, 1024],
        'rbwidth_blk': [15, 30, 110],
    }
    d_type = {
        'network-ssd': 0,
        'network-hdd': 1,
        'network-ssd-nonreplicated': 2
    }

    result_disk = {
        'WIOPS': count_value(d_size['blk_size'][d_type[disk_type]],
                             d_size['wblk_iops'][d_type[disk_type]],
                             d_size['wmax_iops'][d_type[disk_type]]),
        'RIOPS': count_value(d_size['blk_size'][d_type[disk_type]],
                             d_size['rblk_iops'][d_type[disk_type]],
                             d_size['rmax_iops'][d_type[disk_type]]),
        'WBW': count_value(d_size['blk_size'][d_type[disk_type]],
                           d_size['wbwidth_blk'][d_type[disk_type]],
                           d_size['wbwidth'][d_type[disk_type]]),
        'RBW': count_value(d_size['blk_size'][d_type[disk_type]],
                           d_size['rbwidth_blk'][d_type[disk_type]],
                           d_size['rbwidth'][d_type[disk_type]]),
    }

    if disk_type == 'network-ssd-nonreplicated':
        if vcpu > 0:
            max_iops = min(100000, vcpu * 10000)
            max_bwidth = min(1024, vcpu * 100)
            result_disk['WIOPS'] = min(result_disk['WIOPS'], max_iops)
            result_disk['WBW'] = min(result_disk['WBW'], max_bwidth)
            result_disk['RIOPS'] = min(result_disk['RIOPS'], max_iops)
            result_disk['RBW'] = min(result_disk['RBW'], max_bwidth)
        else:
            raise LookupError
    return result_disk


def get_saml_user_attributes(attributes_raw):
    attributes = []
    for attr in list(attributes_raw):
        attributes.append(attr)
        attributes.append(attributes_raw[attr])
    return attributes


def format_account(account_raw, is_human):
    account = {}
    if account_raw:
        if is_human:
            account = account_raw
            try:
                account = {
                    'id': account_raw.sub,
                    'type': 'Organization Account',
                    'login': account_raw.name,
                    'default_email': account_raw.email,
                    'organization_id': account_raw.federation.id or ' ',
                    'organization name': account_raw.federation.name or ' '
                }
            except AttributeError:
                if account_raw.oauth_user_account.federation_id != '':
                    account = {
                        'id': account_raw.id,
                        'type': 'Oauth User Account',
                        'federation_id': account_raw.oauth_user_account.federation_id,
                        'claims': get_saml_user_attributes(account_raw.oauth_user_account.claims)
                    }
                if account_raw.saml_user_account.federation_id != '':
                    account = {
                        'id': account_raw.id,
                        'type': 'SAML user account',
                        'name_id': account_raw.saml_user_account.name_id,
                        'email': account_raw.saml_user_account.attributes.get(
                            'EmailAdress') or account_raw.saml_user_account.name_id,
                        'federation_id': account_raw.saml_user_account.federation_id,
                        'attributes': format_attributes(
                            get_saml_user_attributes(account_raw.saml_user_account.attributes))
                    }
                if account_raw.yandex_passport_user_account.login != '':
                    account = {
                        'id': account_raw.id,
                        'type': 'Yandex Passport User Account',
                        'login': account_raw.yandex_passport_user_account.login,
                        'default_email': account_raw.yandex_passport_user_account.default_email
                    }
        else:
            account = {
                'account_id': account_raw.id,
                'name': account_raw.name,
                'type': 'Service Account',
                'folder_id': account_raw.folder_id,
                'created_at': timestamp_resolve(account_raw.created_at.seconds)
            }
    return account


def format_attributes(attributes: list):
    result = []

    for attr in attributes:
        if type(attr) is str:
            result.append(f"{attr}: ")
        else:
            short = f"{attr.value}"[0:128]
            if len(short) > 127:
                result.append(f" {short[2:]}.. \n")
            else:
                result.append(f" {short[2:-2]} \n")
    return ''.join(result)


def clean_metadata(metadata: str):
    metas = metadata.split('\n')
    clean_metas = []
    for meta in metas:
        if 'net user' in meta:
            meta = f'net user ****'
            print(meta)

        clean_metas.append(meta)
    return '\n'.join(clean_metas)


if __name__ == "__main__":
    get_user_input('preprod')
