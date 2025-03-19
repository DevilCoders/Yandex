#!/usr/bin/env python3

import os
import re
import json
import time
import logging
import requests
import argparse
import configparser
from functools import wraps
from os.path import expanduser
import http.client as http_client
from prettytable import PrettyTable
from requests.exceptions import ConnectionError, Timeout


# Config

EXAMPLE_CONFIG = '[REI_AUTH]\noAuth_token = YOUR_TOKEN_HERE\n\n[CA]\ncert = /path/to/allCAs.pem\n'
home_dir = expanduser('~')
config = configparser.RawConfigParser()
config.read('{home}/.rei/rei.cfg'.format(home=home_dir))
try:
    oauth_token = config.get('REI_AUTH', 'oAuth_token')
except (FileNotFoundError, ValueError, configparser.NoSectionError):
    print('\nCorrupted config or no config file present.\nPlease create or check config: ~/.rei/rei.cfg\n')
    print('Example rei.cfg:\n\n{pattern}'.format(pattern=EXAMPLE_CONFIG))
    quit()

try:
    ca_cert = config.get('CA', 'cert')
except (configparser.NoSectionError, ValueError):
    msg = '\nPlease add to /home/username/.rei/rei.cfg section: \n\n' \
          '[CA]\ncert = /path/to/allCAs.pem\n\n' \
          'For download CA cert use: wget https://crls.yandex.net/allCAs.pem\n'
    print(msg)

try:
    os.environ['REQUESTS_CA_BUNDLE'] = ca_cert
except NameError:
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem'  # CA Certificate for HTTPS (https://crls.yandex.net/allCAs.pem)

# Parser config

parser = argparse.ArgumentParser(description='Yandex.Cloud Quota Manager for support team (YCLOUD-1460)', usage='''
    quotas.py                                       interactive mode (prod)
    quotas.py -pre                                  preprod env
    quotas.py -t <token>                            set new quota limits from quotacalc
    quotas.py -c <cloud>                            show all quotas
    quotas.py -c <cloud> -pre                       show all quotas (preprod)
    quotas.py -c <cloud> -s                         set compute quota (interactive mode)
    quotas.py -c <cloud> -s -n <name> -l <limit>    set compute quota (manual, target)
    quotas.pt -c <cloud> -s -fj <file.json>         set compute quota from json (YCLOUD-1548)
    quotas.py -c <cloud> -s3                        set s3 quota (interactive mode)
    quotas.py -c <cloud> -m                         set mdb quota (interactive mode)
    quotas.py -c <cloud> -z                         set all quotas to zero values (for LLC Yandex clouds)
    quotas.py -b <cloud>                            get billing info
    quotas.py -f <folder>                           show folder quota
    quotas.py -f <folder> -l <limit>                set folder quota (target: active-operation-count)
    quotas.py -fl <file> -l <limit>                 set folder quota for folders from file (new line separated)
    quotas.py -fc <cloud>                           show folder quota for all folders (100 max) in cloud id
    quotas.py -fc <cloud> -l <limit>                set folder quota for all folders (100 max) in cloud id
    \ntotal-disk-size/memory/etc values must be B, KB, MB, GB, TB. Example: 1B, 10K, 100M, 100G, 2T ''')
parser.add_argument('-v', '--version', action='version', version='quota-manager 0.3.9')
parser.add_argument('--debug', action='store_true', help='print debug messages')
parser.add_argument('-pre', '--preprod', action='store_true', help='preprod mode')
parser.add_argument('-c', '--cloud', required=False, type=str, help='show all quotas')
parser.add_argument('-t', '--token', required=False, type=str, help='token from quotacalc')
parser.add_argument('-z', '--zero', required=False, action='store_true', help='set all quotas to zero (-c <cloud> required)')
parser.add_argument('-b', '--billing', required=False, type=str, help='show billing info')
parser.add_argument('-n', '--name', default=False, type=str, help='a name of limit')
parser.add_argument('-l', '--limit', default=False, type=str, help='a value of limit')
comp_group = parser.add_argument_group('Compute quota set')
comp_group.add_argument('-s', '--set', required=False, action='store_true', help='set compute quota (-c <cloud> required)')
comp_group.add_argument('-fj', '--from-json', type=str, metavar='FILE', help='path to json file with values')
comp_group.add_argument('-r', '--resources', action='store_true', help='resources list')
mdb_group = parser.add_argument_group('MDB quota set')
mdb_group.add_argument('-m', '--mdb', required=False, action='store_true', help='interactive set mdb quota (-c <cloud> required)')
s3_group = parser.add_argument_group('S3 quota set')
s3_group.add_argument('-s3', '--storage', required=False, action='store_true', help='set s3 quota (-c <cloud> required)')
folder_group = parser.add_argument_group('Folder quota set')
folder_group.add_argument('-f', '--folder', required=False, type=str, help='target folder id')
folder_group.add_argument('-fl', '--folders-list', type=str, metavar='FILE', help='path to folders list file (new line separated folder_ids)')
folder_group.add_argument('-fc', '--folders-by-cloud', type=str, metavar='CLOUD', help='target cloud id (for all folders in cloud)')
args = parser.parse_args()


# Wrappers

if args.preprod:
    ENV_MODE = '\033[91mPREPROD\033[0m'
    FOLDER_URL = 'https://iaas.private-api.cloud-preprod.yandex.net/compute/external/v1/folderQuota/'
    ALL_FOLDER_URL = 'https://identity.private-api.cloud-preprod.yandex.net:14336/v1/allFolders?cloudId='
    COMPUTE_URL = 'https://iaas.private-api.cloud-preprod.yandex.net/compute/external/v1/quota/'
    IAM_URL = 'http://identity.private-api.cloud-preprod.yandex.net:4336/v1/tokens'
    S3_URL = 'https://storage-idm.private-api.cloud-preprod.yandex.net:1443/management/cloud/'
    S3_MGMT_URL = 'https://storage-idm.private-api.cloud-preprod.yandex.net:1443/management/cloud/'
    MDB_URL = 'https://mdb.private-api.cloud-preprod.yandex.net/mdb/v1/support/quota/'
    QUOTA_CALC = 'https://quotacalc.cloud-testing.yandex.net/qfiles/'
    BILLING_URL = 'https://billing.private-api.cloud-preprod.yandex.net:16465/billing/v1/private/'
else:
    ENV_MODE = '\033[92mPROD\033[0m'
    FOLDER_URL = 'https://iaas.private-api.cloud.yandex.net/compute/external/v1/folderQuota/'
    ALL_FOLDER_URL = 'https://identity.private-api.cloud.yandex.net:14336/v1/allFolders?cloudId='
    COMPUTE_URL = 'https://iaas.private-api.cloud.yandex.net/compute/external/v1/quota/'
    IAM_URL = 'http://identity.private-api.cloud.yandex.net:4336/v1/tokens'
    S3_URL = 'https://storage-idm.private-api.cloud.yandex.net:1443/management/cloud/'
    S3_MGMT_URL = 'https://storage-idm.private-api.cloud.yandex.net:1443/management/cloud/'
    MDB_URL = 'https://mdb.private-api.cloud.yandex.net/mdb/v1/support/quota/'
    QUOTA_CALC = 'https://quotacalc.cloud-testing.yandex.net/qfiles/'
    BILLING_URL = 'https://billing.private-api.cloud.yandex.net:16465/billing/v1/private/'


if args.debug:
    http_client.HTTPConnection.debuglevel = 1
    logging.basicConfig(format='[%(asctime)s] [%(levelname)s] %(message)s',
                        datefmt='%D %H:%M:%S',
                        level=logging.DEBUG)

COMPUTE_QUOTA_NAMES = (
    'cores', 'disk-count', 'external-address-count', 'external-static-address-count', 'external-smtp-direct-address-count',
    'gpus', 'image-count', 'instance-count', 'memory', 'network-count', 'network-ssd-total-disk-size', 'snapshot-count',
    'network-hdd-total-disk-size', 'network-load-balancer-count', 'subnet-count', 'target-group-count',
    'total-disk-size', 'total-snapshot-size', 'route-table-count', 'static-route-count'
)

BYTES_QUOTA = (
    'total-snapshot-size', 'total-disk-size', 'memory', 'total_size_quota', 'total-size',
    'network-ssd-total-disk-size', 'network-hdd-total-disk-size', 'hdd_space', 'ssd_space'
)

MDB_PRESET_LIST = (
    's1.nano', 's1.micro', 's1.small', 's1.medium', 's1.large',
    's1.xlarge', 's2.micro', 's2.small', 's2.medium', 's2.large',
    's2.2xlarge', 's2.3xlarge', 's2.4xlarge', 'b1.nano', 'b1.micro',
    'b1.medium', 'b2.nano', 'b2.micro', 'b2.medium', 'db1.nano',
    'db1.mirco', 'db1.small', 'db1.medium', 'db1.large', 'db1.xlarge'
)

MENU_PATTERN = '\nYC Quota Manager ({env})\n\n'\
    '\033[90mCompute\033[0m\n'\
    '1. Show compute quota\n'\
    '2. Set compute quota (interactive)\n'\
    '3. Set all compute quota to default\n'\
    '4. Set all compute quota to zero\n'\
    '5. Multiply all compute quota (current values * n)\n\n'\
    '\033[90mObject Storage\033[0m\n'\
    '6. Show S3 quota\n'\
    '7. Set S3 quota (interactive)\n'\
    '8. Set S3 quota to zero\n'\
    '9. Set S3 quota to default\n\n'\
    '\033[90mManaged Databases\033[0m\n'\
    '10. Show MDB quota\n'\
    '11. Set MDB quota (replace current value)\n'\
    '12. Add MDB quota (+value)\n'\
    '13. Sub MDB quota (-value)\n'\
    '14. Add default MDB quota\n'\
    '15. Set MDB quota to zero\n\n'\
    '\033[90mFolder Operations Count\033[0m\n'\
    '16. Show folder quota\n'\
    '17. Set folder quota\n'\
    '18. Show folder quota (all folders in cloud)\n'\
    '19. Set folder quota (all folders in cloud)\n\n'\
    'a - show all services quota\n'\
    'z – set all quota to zero\n'\
    'h - help (this menu)\n'\
    'q - exit'.format(env=ENV_MODE)

COMPUTE_DEFAULT_VALUES = {
    'cores': 8,
    'disk-count': 32,
    'external-address-count': 8,
    'external-static-address-count': 2,
    'image-count': 32,
    'instance-count': 8,
    'memory': 68719476736,
    'network-count': 2,
    'network-ssd-total-disk-size': 53687091200,
    'network-hdd-total-disk-size': 214748364800,
    'network-load-balancer-count': 2,
    'snapshot-count': 32,
    'subnet-count': 6,
    'target-group-count': 100,
    'total-snapshot-size': 429496729600
}
#    'route-table-count': 8,
#    'static-route-count': 256,
#    'gpus': 0,
#    'external-smtp-direct-address-count': 0

S3_DEFAULT_VALUES = {
    'buckets_count_quota': 25,
    'total_size_quota': 5497558138880
}

MDB_DEFAULT_VALUES = {
    'clusters': 8,
    'hdd_space': 214748364800,
    'ssd_space': 214748364800,
    'resources': 8
}


# Connection errors handling
def retry(exceptions, tries=4, delay=5, backoff=2):
    def retry_decorator(func):
        @wraps(func)
        def func_retry(*args, **kwargs):
            mtries, mdelay = tries, delay
            while mtries > 1:
                try:
                    return func(*args, **kwargs)
                except exceptions as e:
                    msg = 'Connection error. Retrying in {mdelay} seconds...'.format(mdelay=mdelay)
                    print(msg)
                    time.sleep(mdelay)
                    mtries -= 1
                    mdelay *= backoff
            return func(*args, **kwargs)
        return func_retry
    return retry_decorator


# Bytes to human-readable output wrapper
def size_wrapper(bytes, granularity=2):
    result = []
    sizes = (
        ('TB', 1024**4),
        ('GB', 1024**3),
        ('MB', 1024**2),
        ('KB', 1024),
        ('B', 1)
    )
    if bytes == 0:
        return 0
    else:
        for name, count in sizes:
            value = bytes // count
            if value:
                bytes -= value * count
                result.append("{v} {n}".format(v=value, n=name))
        return ', '.join(result[:granularity])


# human-readable input to bytes wrapper
def bytes_wrapper(size):
    try:
        if re.fullmatch(r'[\d{1,20}]+B', size):
            new_size = size[:-1]
            return int(new_size)
        elif re.fullmatch(r'[\d{1,20}]+K', size):
            separator = size[:-1]
            new_size = int(separator) * 1024
            return new_size
        elif re.fullmatch(r'[\d{1,20}]+M', size):
            separator = size[:-1]
            new_size = int(separator) * 1024**2
            return new_size
        elif re.fullmatch(r'[\d{1,20}]+G', size):
            separator = size[:-1]
            new_size = int(separator) * 1024**3
            return new_size
        elif re.fullmatch(r'[\d{1,20}]+T', size):
            separator = size[:-1]
            new_size = int(separator) * 1024**4
            return new_size
        else:
            print('Please type correct size, examples: 1B, 10K, 100M, 100G, 2T')
            pass
    except Exception as e:
        print('Error: {error}'.format(error=e))


def get_cloud_id():
    while True:
        cloud_id = input('Enter Cloud-ID: ').strip()
        if re.fullmatch('b1+\w{18}', cloud_id) \
            or re.fullmatch('ao+\w{18}', cloud_id):
            return cloud_id
        elif cloud_id == 'q':
            print('Exit\n')
            quit()
        else:
            print('Incorrect Cloud-ID, try again. Example: b1g00011x0xzx0y0x0xx\n')


@retry((ConnectionError, Timeout))
def get_iam(token):
    r = requests.post(IAM_URL, json={"oauthToken": token})
    if r.status_code != 200:
        error = r.json()
        if 'oauthToken' in error['message']:
            print('Please add OAuth token in "~/.rei/rei.cfg"')
            quit()
        else:
            print('\033[91mERROR:\033[0m {msg}'.format(msg=error['message']))
            quit()
    data = json.loads(r.text)
    if args.debug:
        print(data)
    return data.get('iamToken')


# Billing

@retry((ConnectionError, Timeout))
def get_billing_id(cloud_id):
    iam_token = get_iam(oauth_token)
    headers = {'X-YaCloud-SubjectToken': iam_token}
    r = requests.post(BILLING_URL + 'support/resolve', headers=headers, json={'cloud_id': '{cloud}'.format(cloud=cloud_id)})
    res = json.loads(r.text)
    if args.debug:
        print(res)
    if r.status_code != 200:
        print('ERROR: {msg}'.format(msg=res["message"]))
    else:
        return res['id']


@retry((ConnectionError, Timeout))
def get_billing_full(billing_id):
    iam_token = get_iam(oauth_token)
    headers = {'X-YaCloud-SubjectToken': iam_token}
    r = requests.get(BILLING_URL + 'billingAccounts/{billing_id}/fullView'.format(billing_id=billing_id), headers=headers)
    res = json.loads(r.text)
    if args.debug:
        print(res)
    if r.status_code != 200:
        print('ERROR: {msg}'.format(msg=res["message"]))
    else:
        data = {
            "Status": res.get('displayStatus'),
            "UsageStatus": res.get('usageStatus'),
            "BillingID": res.get('id'),
            "Balance": res.get('balance'),
            "Credit": res.get('billingThreshold'),
            "Type": res.get('personType')
        }
        return data


def get_grants(billing_id):
    iam_token = get_iam(oauth_token)
    headers = {'X-YaCloud-SubjectToken': iam_token}
    r = requests.get(BILLING_URL + 'billingAccounts/{billing_id}/monetaryGrants'.format(billing_id=billing_id), headers=headers)
    res = json.loads(r.text)
    if r.status_code != 200:
        print('ERROR: {msg}'.format(msg=res["message"]))
    else:
        grants = res.get('monetaryGrants')
        return grants


# Folders

@retry((ConnectionError, Timeout))
def folder_quota_get(folder_id):
    iam_token = get_iam(oauth_token)
    headers = {
        'X-YaCloud-SubjectToken': iam_token
    }
    req = requests.get(FOLDER_URL + folder_id, headers=headers)
    res = json.loads(req.text)
    if req.ok:
        return res
    else:
        print('\n\033[91mERROR:\033[0m {msg}'.format(msg=res['message']))


def folder_quota_list(folder_id):
    table = PrettyTable()
    table.field_names = ['Name', 'Limit', 'Used']
    table.align = 'l'
    quotas = folder_quota_get(folder_id)
    if quotas:
        for q in quotas['metrics']:
            table.add_row([q['name'], q['limit'], q['value']])
        print(table)
        if args.debug:
            print(quotas)


@retry((ConnectionError, Timeout))
def folder_quota_set(folder_id, name, limit):
    iam_token = get_iam(oauth_token)
    headers = {
        'X-YaCloud-SubjectToken': iam_token,
        'content-type': 'application/json'
    }
    data = {
        'metric': {
            'name': name,
            'limit': int(limit)
        }
    }
    req = requests.patch(FOLDER_URL + '{folder}/metric'.format(folder=folder_id), json=data, headers=headers)
    result = json.loads(req.text)
    if args.debug:
        print(result)
    if req.ok:
        print('\033[92m{name} OK\033[0m (Folder ID: {folder})'.format(name=name, folder=folder_id))
    else:
        print('\033[91mERROR.\033[0m API: {msg}'.format(msg=result['message']))


def folder_multiset_quota(folder_id):
    quotas = folder_quota_get(folder_id)
    if quotas:
        print('\nPress ENTER to make no changes.\n')
        for q in quotas['metrics']:
            name = q['name']
            limit = q['limit']
            while True:
                try:
                    new_limit = input('{name} [{limit}]: '.format(name=name, limit=limit))
                    if new_limit == '':
                        break
                    else:
                        folder_quota_set(folder_id.strip(), name, int(new_limit))
                        break
                except ValueError:
                    print('Limit must be integer')
                except TypeError:
                    pass
        folder_quota_list(folder_id.strip())


def folders_list_quota_set(folder_list, limit):
    with open(folder_list, 'r') as infile:
        folders = infile.readlines()
        for folder in folders:
            folder_quota_set(folder.strip(), 'active-operation-count', limit)
            folder_quota_list(folder.strip())
            print('')


# Experimental block folder quota by cloud

@retry((ConnectionError, Timeout))
def folders_by_cloud_get(cloud_id):
    r = requests.get(ALL_FOLDER_URL + cloud_id + '&pageSize=100')
    data = json.loads(r.text)
    final = []
    for folder in data['result']:
        final.append(folder_quota_get(folder["id"]))
    return final


def folders_by_cloud_list_table(cloud_id):
    table = PrettyTable()
    table.field_names = ['Folder ID', 'Name', 'Limit', 'Used']
    table.align = 'l'
    folders = folders_by_cloud_get(cloud_id)
    if folders:
        for folder_id in folders:
            for metric in folder_id['metrics']:
                table.add_row([folder_id['folder_id'], metric['name'], metric['limit'], metric['value']])
        print(table)
        if args.debug:
            print(folders)


@retry((ConnectionError, Timeout))
def folders_by_cloud_set(cloud_id, limit):
    folders = folders_by_cloud_get(cloud_id)
    for folder in folders:
        folder_id = folder['folder_id']
        folder_quota_set(folder_id, 'active-operation-count', limit)
    folders_by_cloud_list_table(cloud_id)


# Compute

@retry((ConnectionError, Timeout))
def compute_quota_get(cloud_id):
    iam_token = get_iam(oauth_token)
    headers = {
        'X-YaCloud-SubjectToken': iam_token
    }
    req = requests.get(COMPUTE_URL + cloud_id, headers=headers)
    res = json.loads(req.text)
    if req.ok:
        return res
    else:
        print('\n\033[91mERROR:\033[0m {msg}'.format(msg=res['message']))


def compute_quota_list(cloud_id):
    table = PrettyTable()
    table.field_names = ['Name', 'Limit', 'Used']
    table.align = 'l'
    quotas = compute_quota_get(cloud_id)
    if quotas:
        quotas_dict = {}
        for quota in quotas['metrics']:
            quotas_dict[quota['name']]={'limit': quota['limit'], 'value': quota['value']}
        for name in list(sorted(quotas_dict)):
            value = quotas_dict[name]['value']
            limit = quotas_dict[name]['limit']
            if name in BYTES_QUOTA:
                table.add_row([name, size_wrapper(limit), size_wrapper(value)])
            else:
                table.add_row([name, limit, value])
        print(table)
        if args.debug:
            print(quotas)


@retry((ConnectionError, Timeout))
def compute_quota_set(cloud_id, name, limit):
    iam_token = get_iam(oauth_token)
    headers = {
        'X-YaCloud-SubjectToken': iam_token,
        'content-type': 'application/json'
    }
    data = {
        'metric': {
            'name': name,
            'limit': int(limit)
        }
    }
    req = requests.patch(COMPUTE_URL + '{cloud}/metric'.format(cloud=cloud_id), json=data, headers=headers)
    result = json.loads(req.text)
    if req.ok:
        print('\033[92m{name} OK\033[0m'.format(name=name))
    else:
        print('\033[91mERROR.\033[0m API: {msg}'.format(msg=result['message']))


@retry((ConnectionError, Timeout))
def compute_quota_manual_set(cloud_id, name, limit):
    iam_token = get_iam(oauth_token)
    headers = {
        'X-YaCloud-SubjectToken': iam_token,
        'content-type': 'application/json'
    }
    if name in BYTES_QUOTA:
        quota_limit = bytes_wrapper(limit)
    else:
        quota_limit = int(limit)
    data = {
        'metric': {
            'name': name,
            'limit': quota_limit
        }
    }
    req = requests.patch(COMPUTE_URL + '{cloud}/metric'.format(cloud=cloud_id), json=data, headers=headers)
    result = json.loads(req.text)
    if req.ok:
        print('\033[92m{name} OK\033[0m'.format(name=name))
    else:
        print('\033[91mERROR.\033[0m API: {msg}'.format(msg=result['message']))


def compute_multiset_quota(cloud_id):
    quotas = compute_quota_get(cloud_id)
    if quotas:
        print('\nPress ENTER to make no changes.')
        print('disk-size, memory, etc must be B, K, M, G, T (example: 1B, 10K, 100M, 100G, 2T)\n')
        quotas_dict = {}
        for quota in quotas['metrics']:
            quotas_dict[quota['name']]={'limit': quota['limit'], 'value': quota['value']}
        for name in list(sorted(quotas_dict)):
            limit = quotas_dict[name]['limit']
            if name in BYTES_QUOTA:
                visible_limit = size_wrapper(limit)
            else:
                visible_limit = limit
            while True:
                try:
                    new_limit = input('{name} [{lim}]: '.format(name=name, lim=visible_limit))
                    if new_limit == '':
                        break
                    elif new_limit == 'q':
                        print('Aborted\n')
                        compute_quota_list(cloud_id)
                        return False
                    else:
                        if name in BYTES_QUOTA:
                            raw_limit = bytes_wrapper(new_limit)
                            compute_quota_set(cloud_id, name, int(raw_limit))
                        else:
                            compute_quota_set(cloud_id, name, int(new_limit))
                        break
                except ValueError:
                    print('Limit must be integer')
                except TypeError:
                    pass
        compute_quota_list(cloud_id)


def compute_multiply_quota(cloud_id):
    while True:
        try:
            multiplier = int(input('Enter quota multiplier (must be integer): '))
            if multiplier >= 2:
                break
            else:
                print('''Multiplier can't be 0 or 1''')
        except ValueError:
            print('Multiplier must be integer, example: 2')
    quotas = compute_quota_get(cloud_id)
    if quotas:
        for q in quotas['metrics']:
            name = q['name']
            limit = q['limit'] * multiplier
            compute_quota_set(cloud_id, name, int(limit))
        compute_quota_list(cloud_id)


def compute_default_quota(cloud_id):
    for key, value in COMPUTE_DEFAULT_VALUES.items():
        compute_quota_set(cloud_id, key, value)
    compute_quota_list(cloud_id)


def compute_zero_quota(cloud_id):
    quotas = compute_quota_get(cloud_id)
    if quotas:
        for q in quotas['metrics']:
            compute_quota_set(cloud_id, q['name'], 0)
        compute_quota_list(cloud_id)


def compute_quota_resources():
    table = PrettyTable()
    table.field_names = ['Name']
    table.align = 'l'
    for name in COMPUTE_QUOTA_NAMES:
        table.add_row([name])
    print('\nCompute resources:')
    print(table)


def compute_quota_set_json(cloud_id, file):
    try:
        infile = open(file, 'r')
    except FileNotFoundError:
        print('File not found')
        quit()
    else:
        values = json.load(infile)
        infile.close()
    print('\nCloudID: {cloud}'.format(cloud=cloud_id))
    for key, value in values.items():
        if value != 0:
            if key in BYTES_QUOTA:
                new_value = bytes_wrapper(value)
            else:
                new_value = value
            compute_quota_set(cloud_id, key, int(new_value))
    compute_quota_list(cloud_id)


def compute_shortlink_check(cloud_id, datadict):
    table = PrettyTable()
    table.field_names = ['Name', 'New limit']
    table.align = 'l'
    original_comp_quota = {}
    for comp_quota in compute_quota_get(cloud_id)['metrics']:
        name = comp_quota.get('name')
        limit = comp_quota.get('limit')
        original_comp_quota[name] = limit
    data = {}
    for key, value in datadict.items():
        if value != original_comp_quota.get(key):
            data[key] = value
            table.add_row([key, value if key not in BYTES_QUOTA else size_wrapper(value)])
    if data:
        print('Compute \n{table}\n'.format(table=table))
        return data


# Object Storage (S3)

@retry((ConnectionError, Timeout))
def object_storage_quota_get(cloud_id):
    iam_token = get_iam(oauth_token)
    headers = {
        'X-YaCloud-SubjectToken': iam_token,
        'cloud': cloud_id
    }
    req = requests.get(S3_URL + cloud_id, headers=headers)
    res = json.loads(req.text)
    if req.ok:
        return res
    else:
        print('\n\033[91mERROR:\033[0m {msg}'.format(msg=res['detail']))


def object_storage_quota_list(cloud_id):
    table = PrettyTable()
    table.field_names = ['Name', 'Limit', 'Used']
    table.align = 'l'
    s3_bytes_quota = ('total-size', 'total-size-quota')
    quotas = object_storage_quota_get(cloud_id)
    if quotas:
        total_size = size_wrapper(quotas['total_size'])
        total_size_quota = size_wrapper(quotas['total_size_quota'])
        table.add_row(['buckets_count', quotas['buckets_count_quota'], quotas['buckets_count']])
        table.add_row(['total_size', total_size_quota, total_size])
        print(table)
        if args.debug:
            print(quotas)


@retry((ConnectionError, Timeout))
def object_storage_quota_set(cloud_id, name, quota_value):
    iam_token = get_iam(oauth_token)
    headers = {
        'X-YaCloud-SubjectToken': iam_token,
        'content-type': 'application/json'
    }
    data = {
        'cloud_id': cloud_id,
        name: int(quota_value)
    }
    req = requests.put(S3_MGMT_URL + cloud_id, data=json.dumps(data), headers=headers)
    if req.ok:
        print('\033[92m{name} OK\033[0m'.format(name=name))
    else:
        result = json.loads(req.text)
        print('\033[91mERROR:\033[0m API: {status} {data}'.format(status=req.status_code, data=result))


def object_storage_quota(cloud_id):
    print('\nPress ENTER to make no changes.')
    print('total_size must be B, K, M, G, T (example: 1B, 10K, 100M, 100G, 2T)\n')
    s3_values = ('buckets_count_quota', 'total_size_quota')
    current_value = object_storage_quota_get(cloud_id)
    if current_value:
        for name in s3_values:
            if name == 'total_size_quota':
                original_value = current_value['total_size_quota']
                limit = size_wrapper(int(original_value))
            elif name == 'buckets_count_quota':
                limit = current_value['buckets_count_quota']
            while True:
                try:
                    new_value = input('{name} [{limit}]: '.format(name=name, limit=limit))
                    if new_value == '':
                        break
                    elif new_value == 'q':
                        print('Aborted\n')
                        object_storage_quota_list(cloud_id)
                        return False
                    else:
                        if name == 'total_size_quota':
                            raw_limit = bytes_wrapper(new_value)
                            object_storage_quota_set(cloud_id, name, int(raw_limit))
                        else:
                            object_storage_quota_set(cloud_id, name, int(new_value))
                        break
                except ValueError:
                    print('Value must be integer')
                except TypeError:
                    pass
        object_storage_quota_list(cloud_id)


def object_storage_zero_quota(cloud_id):
    s3_keys = ('buckets_count_quota', 'total_size_quota')
    for key in s3_keys:
        object_storage_quota_set(cloud_id, key, 1)
    object_storage_quota_list(cloud_id)


def object_storage_default_quota(cloud_id):
    for key, value in S3_DEFAULT_VALUES.items():
        object_storage_quota_set(cloud_id, key, value)
    object_storage_quota_list(cloud_id)


def object_storage_shortlink_check(cloud_id, datadict):
    table = PrettyTable()
    table.field_names = ['Name', 'New limit']
    table.align = 'l'
    original_s3_quotas = object_storage_quota_get(cloud_id)
    data = {}
    for key, value in datadict.items():
        if value != original_s3_quotas.get(key):
            data[key] = value
            table.add_row([key, value if key not in BYTES_QUOTA else size_wrapper(value)])
    if data:
        print('Object Storage \n{table}\n'.format(table=table))
        return data


# Managed Database (MDB)

@retry((ConnectionError, Timeout))
def mdb_quota_get(cloud_id):
    iam_token = get_iam(oauth_token)
    headers = {
        'X-YaCloud-SubjectToken': iam_token
    }
    req = requests.get(MDB_URL + cloud_id, headers=headers)
    res = json.loads(req.text)
    if req.ok:
        return res
    else:
        return req.status_code


def mdb_quota_list(cloud_id):
    table = PrettyTable()
    table.field_names = ['Name', 'Limit', 'Used']
    table.align = 'l'
    quotas = mdb_quota_get(cloud_id)
    if quotas == 503:
        print('\n\033[91mERROR:\033[0m Cloud not exist.')
    elif quotas == 403:
        print('\n\033[91mERROR:\033[0m 403. Cloud not found.')
    elif 'cloudId' in quotas:
        table.add_row(['Clusters', quotas['clustersQuota'], quotas['clustersUsed']])
        table.add_row(['CPU', quotas['cpuQuota'], quotas['cpuUsed']])
        table.add_row(['Memory', size_wrapper(quotas['memoryQuota']), size_wrapper(quotas['memoryUsed'])])
        table.add_row(['HDD space', size_wrapper(quotas['hddSpaceQuota']), size_wrapper(quotas['hddSpaceUsed'])])
        table.add_row(['SSD space', size_wrapper(quotas['ssdSpaceQuota']), size_wrapper(quotas['ssdSpaceUsed'])])
        table.add_row(['IO', size_wrapper(quotas['ioQuota']), size_wrapper(quotas['ioUsed'])])
        table.add_row(['Network', quotas['networkQuota'], quotas['networkUsed']])
        print(table)
        if args.debug:
            print(quotas)
    else:
        names = ('Clusters', 'CPU', 'Memory', 'HDD space', 'SSD space', 'IO', 'Network')
        for n in names:
            table.add_row([n, 0, 0])
        print(table)


@retry((ConnectionError, Timeout))
def mdb_quota_set(cloud_id, action, limit, target):
    iam_token = get_iam(oauth_token)
    headers = {
        'X-YaCloud-SubjectToken': iam_token,
        'content-type': 'application/json'
    }
    if target == 'resources':
        data = {
            'action': action,
            'count': int(limit),
            'presetId': 's1.nano'
        }
    elif target == 'ssd_space':
        data = {
            'action': action,
            'ssdSpaceQuota': int(limit)
        }
    elif target == 'hdd_space':
        data = {
            'action': action,
            'hddSpaceQuota': int(limit)
        }
    elif target == 'clusters':
        data = {
            'action': action,
            'clustersQuota': int(limit)
        }
    else:
        pass
    req = requests.post(MDB_URL + '{cloud}/{target}'.format(cloud=cloud_id, target=target), data=json.dumps(data), headers=headers)
    if req.ok:
        print('\033[92m{target} OK\033[0m'.format(target=target))
    else:
        result = json.loads(req.text)
        print('\033[91mERROR:\033[0m API: {msg}'.format(msg=result))


def mdb_quota_manage(cloud_id, act):
    print('Size for hdd/ssd space must be B, K, M, G, T (example: 1B, 10K, 100M, 100G, 2T)')
    print('Press ENTER to make no changes.\n')
    current_value = mdb_quota_get(cloud_id)
    quotas = ('resources', 'clusters', 'hdd_space', 'ssd_space')
    bytes_like = ('hdd_space', 'ssd_space')
    if current_value == 503:
        print('\n\033[91mERROR:\033[0m Cloud not exist.')
    elif current_value == 403:
        print('\n\033[91mERROR:\033[0m 403. Cloud not found')
    elif 'cloudId' in current_value:
        for q_name in quotas:
            if q_name == 'clusters':
                limit = current_value['clustersQuota']
            elif q_name == 'hdd_space':
                original_value = current_value['hddSpaceQuota']
                limit = size_wrapper(int(original_value))
            elif q_name == 'ssd_space':
                original_value = current_value['ssdSpaceQuota']
                limit = size_wrapper(int(original_value))
            elif q_name == 'resources':
                limit = current_value['cpuQuota']
            while True:
                try:
                    new_value = input('{name} [{limit}]: '.format(name=q_name, limit=limit))
                    if new_value == '':
                        break
                    elif new_value == 'q':
                        print('Aborted\n')
                        mdb_quota_list(cloud_id)
                        return False
                    else:
                        if q_name in bytes_like:
                            raw_limit = bytes_wrapper(new_value)
                            mdb_quota_set(cloud_id, act, int(raw_limit), q_name)
                        else:
                            mdb_quota_set(cloud_id, act, int(new_value), q_name)
                        break
                except ValueError:
                    print('Value must be integer')
                except TypeError:
                    pass
        mdb_quota_list(cloud_id)
    else:
        print('\n{cloud} is virgin cloud without default quota'.format(cloud=cloud_id))
        defloration = input('Do you want to add default values for this cloud? (y/n): ')
        if defloration == 'y':
            mdb_default_quota(cloud_id)


def mdb_replace_quota(cloud_id):
    print('\nPress ENTER to make no changes.')
    print('Size for hdd/ssd space must be B, K, M, G, T (example: 1B, 10K, 100M, 100G, 2T)\n')
    current_value = mdb_quota_get(cloud_id)
    quotas = ('resources', 'clusters', 'hdd_space', 'ssd_space')
    bytes_like = ('hdd_space', 'ssd_space')
    if current_value == 503:
        print('\n\033[91mERROR:\033[0m Cloud not exist.')
    elif current_value == 403:
        print('\n\033[91mERROR:\033[0m 403. Cloud not found')
    elif 'cloudId' in current_value:
        for q_name in quotas:
            if q_name == 'clusters':
                original_value = current_value['clustersQuota']
            elif q_name == 'hdd_space':
                original_value = current_value['hddSpaceQuota']
            elif q_name == 'ssd_space':
                original_value = current_value['ssdSpaceQuota']
            elif q_name == 'resources':
                original_value = int(current_value['cpuQuota'])
            while True:
                try:
                    if q_name in bytes_like:
                        limit = size_wrapper(int(original_value))
                    else:
                        limit = original_value
                    in_value = input('{name} [{limit}]: '.format(name=q_name, limit=limit))
                    if in_value == '':
                        break
                    elif in_value == 'q':
                        print('Aborted\n')
                        mdb_quota_list(cloud_id)
                        return False
                    else:
                        if q_name in bytes_like:
                            input_value = bytes_wrapper(in_value)
                        else:
                            input_value = int(in_value)
                        if input_value < original_value:
                            new_value =  original_value - input_value
                            action = 'sub'
                            mdb_quota_set(cloud_id, action, int(new_value), q_name)
                        elif input_value == original_value:
                            print("Identical values, skip")
                        else:
                            new_value = input_value - original_value
                            action = 'add'
                            mdb_quota_set(cloud_id, action, int(new_value), q_name)
                        break
                except ValueError:
                    print('Value must be integer')
                except TypeError:
                    pass
        mdb_quota_list(cloud_id)
    else:
        print('\n{cloud} is virgin cloud without default quota'.format(cloud=cloud_id))
        defloration = input('Do you want to add default values for this cloud? (y/n): ')
        if defloration == 'y':
            mdb_default_quota(cloud_id)


def mdb_replace_quota_shortlink(cloud_id, name, limit):
    current_value = mdb_quota_get(cloud_id)
    if name == 'clusters':
        original_value = current_value['clustersQuota']
    elif name == 'hdd_space':
        original_value = current_value['hddSpaceQuota']
    elif name == 'ssd_space':
        original_value = current_value['ssdSpaceQuota']
    elif name == 'resources':
        original_value = int(current_value['cpuQuota'])
    if limit < original_value:
        new_value =  original_value - limit
        action = 'sub'
        mdb_quota_set(cloud_id, action, new_value, name)
    else:
        new_value = limit - original_value
        action = 'add'
        mdb_quota_set(cloud_id, action, new_value, name)


def mdb_zero_quota(cloud_id):
    quotas = mdb_quota_get(cloud_id)
    if quotas == 503:
        print('Cloud not exist.')
    elif 'cloudId' not in quotas:
        for key, value in MDB_DEFAULT_VALUES.items():
            mdb_quota_set(cloud_id, 'add', value, key)
            mdb_replace_quota_shortlink(cloud_id, key, 0)
        mdb_quota_list(cloud_id)
    else:
        for key in MDB_DEFAULT_VALUES.keys():
            mdb_replace_quota_shortlink(cloud_id, key, 0)
        mdb_quota_list(cloud_id)


# TODO: rewrite like a replace func
def mdb_default_quota(cloud_id):
    quotas = mdb_quota_get(cloud_id)
    if quotas == 503:
        print('Cloud not exist.')
    elif 'cloudId' not in quotas:
        for key, value in MDB_DEFAULT_VALUES.items():
            mdb_quota_set(cloud_id, 'add', value, key)
        mdb_quota_list(cloud_id)
    else:
        for key, value in MDB_DEFAULT_VALUES.items():
            mdb_replace_quota_shortlink(cloud_id, key, value)
        mdb_quota_list(cloud_id)


def mdb_replace_default_quota(cloud_id):
    quotas = mdb_quota_get(cloud_id)
    if quotas == 503:
        print('Cloud not exist.')
    elif 'cloudId' not in quotas:
        for key, value in MDB_DEFAULT_VALUES.items():
            mdb_quota_set(cloud_id, 'add', value, key)
    else:
        print('''Sorry, but I won't add default quota for this cloud, use "11" or "12" to add quota in normal mode.''')
        print('Default quota have already been provided for this cloud: {cloud}'.format(cloud=cloud_id))
        mdb_quota_list(cloud_id)


def mdb_shortlink_check(cloud_id, datadict):
    table = PrettyTable()
    table.field_names = ['Name', 'New limit']
    table.align = 'l'
    original_mdb_quota = {}
    quotas = mdb_quota_get(cloud_id)
    if not quotas:
        mdb_replace_default_quota(cloud_id)
        print('''\nRelax, i'am just added default MDB quota for {cloud}, because MDB quota was equal to 0.\n'''.format(cloud=cloud_id))

    mdb_quotas = mdb_quota_get(cloud_id)
    original_mdb_quota['clusters'] = mdb_quotas['clustersQuota']
    original_mdb_quota['resources'] = mdb_quotas['cpuQuota']
    original_mdb_quota['hdd_space'] = mdb_quotas['hddSpaceQuota']
    original_mdb_quota['ssd_space'] = mdb_quotas['ssdSpaceQuota']
    data = {}
    for key, value in datadict.items():
        if value != original_mdb_quota.get(key):
            data[key] = value
            table.add_row([key, value if key not in BYTES_QUOTA else size_wrapper(value)])
    if data:
        print('Managed Databases \n{table}\n'.format(table=table))
        return data


# Other

def all_quotas_list(cloud_id):
    backoffice_link = 'https://backoffice.cloud.yandex.ru/clouds/{cloud}?section=quotas'.format(cloud=cloud_id)
    print('\nCompute')
    compute_quota_list(cloud_id)
    print('\nObject Storage')
    object_storage_quota_list(cloud_id)
    print('\nManaged Database')
    mdb_quota_list(cloud_id)
    print('\nFolder Operation Count')
    folders_by_cloud_list_table(cloud_id)
    print('\nBackoffice: {link}\n'.format(link=backoffice_link))


def all_zero_set_quota(cloud_id):
    print('\n\033[91mWARNING: This option set ALL quota for cloud {cloud} to ZERO values.\033[0m'.format(cloud=cloud_id))
    checkout = input('Continue? (y/n): ')
    if checkout == "y" or checkout == "yes":
        mdb_zero_quota(cloud_id)
        compute_zero_quota(cloud_id)
        object_storage_zero_quota(cloud_id)
    else:
        print('Aborted.')


@retry((ConnectionError, Timeout))
def quota_set_from_url(token):
    try:
        r = requests.get(QUOTA_CALC + token)
        data = json.loads(r.text)
    except json.decoder.JSONDecodeError:
        print('\033[91mERROR.\033[0m Token from quotacalc expired')
        quit()
    if args.debug:
        print(data)
    cloud_id = data.get('cloudId')
    compute = compute_shortlink_check(cloud_id, data.get('compute'))
    mdb = mdb_shortlink_check(cloud_id, data.get('mdb'))
    s3 = object_storage_shortlink_check(cloud_id, data.get('s3'))
    if not s3 and not compute and not mdb:
        print('\033[91mNo changes found\033[0m')
        return False
    check = input('\033[91mPlease, verify new limits for CloudID {cloud}.\033[0m \nApply changes? (y/n): '.format(cloud=cloud_id))
    if check == 'y' or check == 'yes':
        if compute:
            for c_key, c_value in compute.items():
                compute_quota_set(cloud_id, c_key, c_value)
        if mdb:
            for m_key, m_value in mdb.items():
                mdb_replace_quota_shortlink(cloud_id, m_key, m_value)
        if s3:
            for s_key, s_value in s3.items():
                object_storage_quota_set(cloud_id, s_key, s_value)
        all_quotas_list(cloud_id)
    else:
        print('\nCanceled.\nBye!\n')


def menu():
    print(MENU_PATTERN)
    while True:
        choose = input('\nEnter command ("h" for help): ')
        if choose == '1':
            print('Compute quota list')
            compute_quota_list(get_cloud_id())
        elif choose == '2':
            print('interactive compute quota set')
            compute_multiset_quota(get_cloud_id())
        elif choose == '3':
            print('Default compute quota set')
            compute_default_quota(get_cloud_id())
        elif choose == '4':
            print('Zero compute quota set')
            compute_zero_quota(get_cloud_id())
        elif choose == '5':
            print('Multiplier compute quota set')
            compute_multiply_quota(get_cloud_id())
        elif choose == '6':
            print('S3 quota list')
            object_storage_quota_list(get_cloud_id())
        elif choose == '7':
            print('S3 quota set')
            object_storage_quota(get_cloud_id())
        elif choose == '8':
            print('S3 zero quota set')
            object_storage_zero_quota(get_cloud_id())
        elif choose == '9':
            print('S3 default quota set')
            object_storage_default_quota(get_cloud_id())
        elif choose == '10':
            print('MDB quota list')
            mdb_quota_list(get_cloud_id())
        elif choose == '11':
            print('MDB replace quota')
            print('\033[91mResource preset - s1.nano (1CPU, 4RAM)\033[0m\n')
            mdb_replace_quota(get_cloud_id())
        elif choose == '12':
            print('MDB quota add')
            print('\nResource preset - s1.nano (1CPU, 4RAM) \n\033[91mWARNING: Quota is NOT REPLACED but ADDED +value\033[0m \n')
            mdb_quota_manage(get_cloud_id(), 'add')
        elif choose == '13':
            print('MDB quota sub')
            print('\nResource preset - s1.nano (1CPU, 4RAM) \n\033[91mWARNING: Quota is NOT REPLACED but SUBTRACTS -value\033[0m \n')
            mdb_quota_manage(get_cloud_id(), 'sub')
        elif choose == '14':
            print('MDB default quota set\n')
            mdb_default_quota(get_cloud_id())
        elif choose == '15':
            print('MDB zero quotas set\n')
            mdb_zero_quota(get_cloud_id())
        elif choose == '16':
            print('Folder quota show')
            folder_quota_list(input('Enter Folder ID: ').strip())
        elif choose == '17':
            print('Folder quota set')
            folder_multiset_quota(input('Enter Folder ID: ').strip())
        elif choose == '18':
            print('All folders quota SHOW by cloud id')
            folders_by_cloud_list_table(get_cloud_id())
        elif choose == '19':
            print('All folders quota SET by cloud id')
            folders_by_cloud_set(get_cloud_id(), int(input('\nNew active-operation-count limit: ')))
        elif choose == 'a':
            print('All quota list')
            all_quotas_list(get_cloud_id())
        elif choose == 'z':
            all_zero_set_quota(get_cloud_id())
        elif choose == 'h' or choose == 'help':
            print(MENU_PATTERN)
        elif choose == 'q' or choose == 'quit':
            print('Exit\n')
            quit()
        else:
            print('\nInput error. Please type digits (1-18) or "h" for help.')


# Argparse

try:
    if args.cloud:
        if re.fullmatch('b1+\w{18}', args.cloud) \
            or re.fullmatch('ao+\w{18}', args.cloud):
            if args.set:
                if args.name and args.limit:
                    compute_quota_manual_set(args.cloud, args.name, args.limit)
                    compute_quota_list(args.cloud)
                elif args.from_json:
                    compute_quota_set_json(args.cloud, args.from_json)
                else:
                    compute_multiset_quota(args.cloud)
            elif args.storage:
                object_storage_quota(args.cloud)
            elif args.mdb:
                mdb_replace_quota(args.cloud)
            elif args.zero:
                all_zero_set_quota(args.cloud)
            else:
                all_quotas_list(args.cloud)
        else:
            print('\nError. Usage: ./quotas -c <CLOUD_ID>\n')
    elif args.token:
        quota_set_from_url(args.token)
    elif args.resources:
        compute_quota_resources()
    elif args.billing:
        for key, value in get_billing_full(get_billing_id(args.billing)).items():
            print('{key}: {value}'.format(key=key, value=value))
    elif args.folders_list:
        if not args.limit:
            print('Error. --limit required. \nExample: ./quotas.py -fl <file> -l <limit>')
        else:
            folders_list_quota_set(args.folders_list, args.limit)
    elif args.folder:
        if args.limit:
            folder_quota_set(args.folder, 'active-operation-count', args.limit)
            folder_quota_list(args.folder)
        else:
            folder_quota_list(args.folder)
    elif args.folders_by_cloud:
        if args.limit:
            folders_by_cloud_set(args.folders_by_cloud, args.limit)
        else:
            folders_by_cloud_list_table(args.folders_by_cloud)
    else:
        menu()
except KeyboardInterrupt:
    print('\nExit')
    quit()
