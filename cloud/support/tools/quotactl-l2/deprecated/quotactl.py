#!/usr/bin/env python3

import os
import re
import json
import grpc
import time
import logging
import requests
import argparse
import configparser

from functools import wraps
from datetime import datetime
from os.path import expanduser

import http.client as http_client

from prettytable import PrettyTable
from requests.exceptions import ConnectionError, Timeout


from yandex.cloud.priv.quota import quota_pb2
from yandex.cloud.priv.serverless.functions.v1 import \
    quota_service_pb2_grpc as serverless_quota_service
from yandex.cloud.priv.serverless.triggers.v1 import \
    quota_service_pb2_grpc as triggers_quota_service
from yandex.cloud.priv.microcosm.instancegroup.v1 import \
    quota_service_pb2_grpc as instance_group_quota_service
from yandex.cloud.priv.k8s.v1 import quota_service_pb2_grpc as k8s_quota_service
from yandex.cloud.priv.containerregistry.v1 import \
    quota_service_pb2_grpc as container_registry_quota_service

from yandex.cloud.priv.compute.v1 import quota_service_pb2_grpc as compute_quota_service


from yandex.cloud.priv.monitoring.v2 import quota_service_pb2_grpc as monitoring_quota_service
from yandex.cloud.priv.vpc.v1 import quota_service_pb2_grpc as vpc_quota_service
from yandex.cloud.priv.ai.config.v1 import quota_service_pb2_grpc as ai_quota_service
from yandex.cloud.priv.iot.devices.v1 import quota_service_pb2_grpc as iot_quota_service

# Config

home_dir = expanduser('~')
config = configparser.RawConfigParser()
config.read('{home}/.rei/rei.cfg'.format(home=home_dir))


def init_config_setup():
    print('Checkout working directory...')

    if not os.path.exists('{home}/.rei'.format(home=home_dir)):
        print('Creating directory ~/.rei/')
        os.system('mkdir -p {home}/.rei'.format(home=home_dir))

    print('\nGet OAuth-token here: https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb')
    token = input('\nEnter your OAuth-token: ')

    if not os.path.isfile('{home}/.rei/allCAs.pem'.format(home=home_dir)):
        print('\nTrying download allCAs.pem')
        os.system('wget https://crls.yandex.net/allCAs.pem -O {home}/.rei/allCAs.pem'.format(home=home_dir))
        cert_path = '{home}/.rei/allCAs.pem'.format(home=home_dir)
        if not os.path.isfile('{home}/.rei/allCAs.pem'.format(home=home_dir)):
            print('\nPlease download CA cert from here: https://crls.yandex.net/allCAs.pem')
            cert_path = input('And enter /path/to/allCAs.pem: ')
    else:
        cert_path = '{home}/.rei/allCAs.pem'.format(home=home_dir)

    with open('{home}/.rei/rei.cfg'.format(home=home_dir), 'w') as cfgfile:
        try:
            config.add_section('REI_AUTH')
        except configparser.DuplicateSectionError as e:
            logging.debug(e)
        config.set('REI_AUTH', 'oauth_token', token)
        try:
            config.add_section('CA')
        except configparser.DuplicateSectionError as e:
            logging.debug(e)
        config.set('CA', 'cert', cert_path)
        try:
            config.add_section('ENDPOINTS')
        except configparser.DuplicateSectionError as e:
            logging.debug(e)
        config.set('ENDPOINTS', 'priv_iaas_api', 'https://iaas.private-api.cloud.yandex.net')
        config.set('ENDPOINTS', 'priv_identity_api', 'https://identity.private-api.cloud.yandex.net:14336')
        config.set('ENDPOINTS', 'priv_s3_api', 'https://storage-idm.private-api.cloud.yandex.net:1443')
        config.set('ENDPOINTS', 'priv_mdb_api', 'https://mdb.private-api.cloud.yandex.net')
        config.set('ENDPOINTS', 'priv_billing_api', 'https://billing.private-api.cloud.yandex.net:16465')
        config.set('ENDPOINTS', 'pub_mdb_api', 'https://mdb.api.cloud.yandex.net')
        config.set('ENDPOINTS', 'pub_compute_api', 'https://compute.api.cloud.yandex.net')
        config.write(cfgfile)
        print('\nDone. You can check your config:\ncat ~/.rei/rei.cfg\n')
    cfgfile.close()
    quit()


try:
    oauth_token = config.get('REI_AUTH', 'oAuth_token')
except (FileNotFoundError, ValueError, configparser.NoSectionError):
    print('\nCorrupted config or no config file present.\nInitialization...')
    init_config_setup()

try:
    ca_cert = config.get('CA', 'cert')
except (configparser.NoSectionError, ValueError):
    print('\nCorrupted config or no config file present.\nInitialization...')
    init_config_setup()


try:
    os.environ['REQUESTS_CA_BUNDLE'] = ca_cert
except NameError:
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem'  # CA Certificate for HTTPS (https://crls.yandex.net/allCAs.pem)


# Parser config

parser = argparse.ArgumentParser(description='Yandex.Cloud Quota Manager for Support team (YCLOUD-1460)', usage='''
    quotactl                                               interactive mode (prod)
    quotactl --init                                        generate config file and download CA cert
    quotactl -pre                                          preprod env
    quotactl -a <cloud>                                    show all quotas in cloud
    quotactl -a <cloud> -pre                               show all quotas (preprod)
    quotactl -z <cloud>                                    set all quotas to zero values (for LLC Yandex clouds)
    quotactl -t <token>                                    set new quota limits from quotacalc
    quotactl -c <cloud>                                    show all quotas
    quotactl -c <cloud> -s                                 set compute quota (interactive mode)
    quotactl -c <cloud> -s -n <name> -l <limit>            set compute quota manual
    quotactl -c <cloud> -fj <file.json>                    set compute quota from json (YCLOUD-1548)
    quotactl -f <folder>                                   show folder quota
    quotactl -f <folder> -l <limit>                        set folder quota (target: active-operation-count)
    quotactl -fl <file> -l <limit>                         set folder quota for folders from file (new line separated)
    quotactl -fc <cloud>                                   show folder quota for all folders (100 max) in cloud id
    quotactl -fc <cloud> -l <limit>                        set folder quota for all folders (100 max) in cloud id
    quotactl -s3 <cloud> -s                                show s3 quota
    quotactl -s3 <cloud> -s -n <name> -l <limit>           set s3 quota
    quotactl -s3 <cloud> -s                                set s3 quota (interactive mode)
    quotactl -m <cloud>                                    show mdb quota
    quotactl -m <cloud> -s -n <name> -l <limit>            set mdb quota
    quotactl -m <cloud> -s                                 set mdb quota (interactive mode)
    quotactl -srv <cloud>                                  list serverless quota
    quotactl -srv <cloud> -s                               set serverless quota interactive
    quotactl -srv <cloud> -s -n <name> -l <limit>          set serverless quota manual
    quotactl -ig <cloud>                                   show instance group quota
    quotactl -ig <cloud> -s                                set instance group quota interactive
    quotactl -ig <cloud> -s -n <name> -l <limit>           set instance group quota manual
    quotactl -k8s <cloud>                                  show kubernetes quota
    quotactl -k8s <cloud> -s                               set kubernetes quota interactive
    quotactl -k8s <cloud> -s -n <name> -l <limit>          set kubernetes quota manual
    quotactl -cr <cloud>                                   show container registry quotat
    quotactl -cr <cloud> -s                                set container registry quota interactive
    quotactl -cr <cloud> -s -n <name> -l <limit>           set container registry quota manual
    quotactl -r                                            list quota resources
    quotactl -pl                                           list mdb presets
    \nDisk size, memory, etc values must be B, KB, MB, GB, TB. Example: 1B, 10K, 100M, 100G, 2T ''')

# General args
parser.add_argument('--version', action='version', version='quotactl v0.5.2')
parser.add_argument('--update', action='store_true', help='update quotactl. works only if it was installed by a script')
parser.add_argument('-v', '--debug', action='store_true', help='print debug messages')
parser.add_argument('--init', action='store_true', help='generate config file')
parser.add_argument('-pre', '--preprod', action='store_true', help='preprod mode')
parser.add_argument('-t', '--token', required=False, type=str, help='set quota with token from quotacalc')
parser.add_argument('-z', '--zero', required=False, metavar='CLOUD', type=str, help='set all quotas to zero')
parser.add_argument('-a', '--all-quota', metavar='CLOUD', type=str, help='show all quota in cloud')
parser.add_argument('--test', type=str)

# Additional args
additional_group = parser.add_argument_group('Additional args')
additional_group.add_argument('-s', '--set', required=False, action='store_true', help='set quota')
additional_group.add_argument('-n', '--name', default=False, type=str, help='a name of limit')
additional_group.add_argument('-l', '--limit', default=False, type=str, help='a value of limit')
additional_group.add_argument('-r', '--resources', action='store_true', help='resources quota list')
additional_group.add_argument('--grpc', action='store_true', help='arg for grpc compute quota list/set')

# Compute args
comp_group = parser.add_argument_group('Compute service')
comp_group.add_argument('-c', '--compute', required=False, metavar='CLOUD', type=str, help='manage compute quota')
comp_group.add_argument('-fj', '--from-json', type=str, metavar='FILE', help='path to json file with values (only compute)')

# MDB args
mdb_group = parser.add_argument_group('MDB service')
mdb_group.add_argument('-m', '--mdb', required=False, metavar='CLOUD', type=str, help='manage mdb quota')

# DEPRECATED
# mdb_group.add_argument('--add', required=False, action='store_true', help='add resources for mdb')
# mdb_group.add_argument('--sub', required=False, action='store_true', help='sub resources for mdb')
# mdb_group.add_argument('--preset', required=False, type=str, default='s1.nano', help='preset for hosts in CLI mode')
# mdb_group.add_argument('-pl', '--preset-list', required=False, action='store_true', help='list mdb resources')

# Object storage args
s3_group = parser.add_argument_group('S3 service')
s3_group.add_argument('-s3', '--storage', required=False, metavar='CLOUD', type=str, help='manage s3 quota')

# Kubernetes args
k8s_group = parser.add_argument_group('Kubernetes service')
k8s_group.add_argument('-k8s', '--kubernetes', required=False, type=str, metavar='CLOUD', help='manage k8s quota')

# Serverless args
srv_group = parser.add_argument_group('Serverless service')
srv_group.add_argument('-srv', '--serverless', required=False, type=str, metavar='CLOUD', help='manage serverless quota')

# Instance group args
ig_group = parser.add_argument_group('Instance Group service')
ig_group.add_argument('-ig', '--instance-group', required=False, type=str, metavar='CLOUD', help='manage instance group quota')

# Container registry args
container_group = parser.add_argument_group('Container Registry service')
container_group.add_argument('-cr', '--container-registry', required=False, type=str, metavar='CLOUD', help='manage container registry quota')

# Resource Manager Folder args
folder_group = parser.add_argument_group('Folder service')
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
    IAM_URL = 'https://identity.private-api.cloud-preprod.yandex.net:14336/v1/tokens'
    S3_URL = 'https://storage-idm.private-api.cloud-preprod.yandex.net:1443/management/cloud/'
    S3_MGMT_URL = 'https://storage-idm.private-api.cloud-preprod.yandex.net:1443/management/cloud/'
    MDB_URL = 'https://mdb.private-api.cloud-preprod.yandex.net/mdb/v1/support/quota/'
    MDB_V2_URL = 'https://mdb.private-api.cloud-preprod.yandex.net/mdb/v2/quota'
    QUOTA_CALC = 'https://quotacalc.cloud-testing.yandex.net/qfiles/'
    BILLING_URL = 'https://billing.private-api.cloud-preprod.yandex.net:16465/billing/v1/private/'
    ENDPOINTS = {
        'serverless': 'serverless-functions.private-api.ycp.cloud-preprod.yandex.net:443',
        'triggers': 'serverless-triggers.private-api.ycp.cloud-preprod.yandex.net:443',
        'instance_group': 'instance-group.private-api.ycp.cloud-preprod.yandex.net:443',
        'kubernetes': 'mk8s.private-api.ycp.cloud-preprod.yandex.net:443',
        'container_registry': 'container-registry.private-api.ycp.cloud-preprod.yandex.net:443',
        'compute': 'compute-api.cloud-preprod.yandex.net:9051',
        'iot': 'iot.private-api.ycp.cloud-preprod.yandex.net:443',
        'ai': 'ml-services-config.private-api.ycp.cloud-preprod.yandex.net:443'
    }
else:
    ENV_MODE = '\033[92mPROD\033[0m'
    FOLDER_URL = 'https://iaas.private-api.cloud.yandex.net/compute/external/v1/folderQuota/'
    ALL_FOLDER_URL = 'https://identity.private-api.cloud.yandex.net:14336/v1/allFolders?cloudId='
    COMPUTE_URL = 'https://iaas.private-api.cloud.yandex.net/compute/external/v1/quota/'
    IAM_URL = 'https://identity.private-api.cloud.yandex.net:14336/v1/tokens'
    S3_URL = 'https://storage-idm.private-api.cloud.yandex.net:1443/management/cloud/'
    S3_MGMT_URL = 'https://storage-idm.private-api.cloud.yandex.net:1443/management/cloud/'
    MDB_URL = 'https://mdb.private-api.cloud.yandex.net/mdb/v1/support/quota/'
    MDB_V2_URL = 'https://mdb.private-api.cloud.yandex.net/mdb/v2/quota'
    QUOTA_CALC = 'https://quotacalc.cloud-testing.yandex.net/qfiles/'
    BILLING_URL = 'https://billing.private-api.cloud.yandex.net:16465/billing/v1/private/'
    ENDPOINTS = {
        'serverless': 'serverless-functions.private-api.ycp.cloud.yandex.net:443',
        'triggers': 'serverless-triggers.private-api.ycp.cloud.yandex.net:443',
        'instance_group': 'instance-group.private-api.ycp.cloud.yandex.net:443',
        'kubernetes': 'mk8s.private-api.ycp.cloud.yandex.net:443',
        'container_registry': 'container-registry.private-api.ycp.cloud.yandex.net:443',
        'compute': 'compute-api.cloud.yandex.net:9051',
        'iot': 'iot.private-api.ycp.cloud.yandex.net:443',
        'monitoring': 'monitoring.private-api.ycp.cloud.yandex.net:443',
        'vpc': 'vpc.private-api.ycp.cloud.yandex.net:443',
        'ai': 'ml-services-config.private-api.ycp.cloud.yandex.net:443'
    }


if args.debug:
    http_client.HTTPConnection.debuglevel = 1
    logging.basicConfig(format='[%(asctime)s] [%(levelname)s] %(message)s',
                        datefmt='%D %H:%M:%S',
                        level=logging.DEBUG)

COMPUTE_QUOTA_NAMES = (
    'cores',
    'disk-count',
    'external-address-count',
    'external-static-address-count',
    'external-smtp-direct-address-count',
    'gpus',
    'image-count',
    'instance-count',
    'memory',
    'network-count',
    'network-ssd-total-disk-size',
    'snapshot-count',
    'network-hdd-total-disk-size',
    'network-load-balancer-count',
    'subnet-count',
    'target-group-count',
    'total-snapshot-size',
    'placement-group-count',
    'route-table-count',
    'static-route-count'
)

MDB_QUOTA_NAMES = (
    'mdb.hdd.size',
    'mdb.ssd.size',
    'mdb.memory.size',
    'mdb.gpu.count',
    'mdb.cpu.count',
    'mdb.clusters.count',
)

KUBERNETES_QUOTA_NAMES = (
    'managed-kubernetes.clusters.count',
    'managed-kubernetes.node-groups.count',
    'managed-kubernetes.nodes.count',
    'managed-kubernetes.cores.count',
    'managed-kubernetes.memory.size',
    'managed-kubernetes.disk.size'
)

SERVERLESS_QUOTA_NAMES = (
    'serverless.memory.usage',
    'serverless.functions.count',
    'serverless.workers.count',
    'serverless.request.count',
    'serverless.triggers.count',
)

CONTAINER_REGISTRY_QUOTA_NAMES = (
    'container-registry.registries.count',
)

INSTANCE_GROUP_QUOTA_NAMES = (
    'GROUP_COUNT',
)

# FIXME
OBJECT_STORAGE_QUOTA_NAMES = (
    'buckets_count_quota',
    'total_size_quota'
)

TRIGGERS_QUOTA_NAMES = (
    'serverless.triggers.count',
)

BYTES_QUOTA = (
    'total-snapshot-size',
    'total-disk-size',
    'memory',
    'total_size_quota',
    'total_size',
    'network-ssd-total-disk-size',
    'network-hdd-total-disk-size',
    'hdd_space',
    'ssd_space',
    'memory-bytes-per-cloud',
    'disk-bytes-per-cloud',
    'mdb.hdd.size',
    'mdb.ssd.size',
    'mdb.memory.size',
    'serverless.memory.size',
    'compute.ssd-disks.size',
    'compute.snapshots.size',
    'compute.hdd-disks.size',
    'compute.instances.memory',
    'managed-kubernetes.memory.size',
    'managed-kubernetes.disk.size',
    'compute.ssdDisks.size',
    'compute.instanceMemory.size',
    'compute.hddDisks.size'
)

MDB_PRESET_LIST = (
    's1.nano', 's1.micro', 's1.small', 's1.medium', 's1.large',
    's1.xlarge', 'm1.small', 'm1.medium', 'm1.large', 'm1.xlarge',
    'm1.2xlarge', 'm1.3xlarge', 'm1.4xlarge','s2.micro', 's2.small',
    's2.medium', 's2.large', 's2.2xlarge', 's2.3xlarge', 's2.4xlarge',
    's2.5xlarge', 'm1.small', 'm2.medium', 'm2.large', 'm2.xlarge',
    'm2.2xlarge', 'm2.3xlarge', 'm2.4xlarge', 'm2.5xlarge', 'm2.6xlarge',
    'b1.nano', 'b1.micro', 'b1.medium', 'b2.nano', 'b2.micro', 'b2.medium',
    'db1.nano', 'db1.mirco', 'db1.small', 'db1.medium', 'db1.large', 'db1.xlarge',
)

MENU_PATTERN = '\nYC Quota Manager ({env})\n\n'\
        '\033[90mCompute\033[0m                                \033[90mObject Storage\033[0m\n'\
        '1. Show compute quota                  14. Show S3 quota\n'\
        '2. Set compute quota                   15. Set S3 quota\n'\
        '3. Set compute quota to default        16. Set S3 quota to zero\n'\
        '4. Set compute quota to zero           17. Set S3 quota to default\n'\
        '5. Multiply compute quota\n\n'\
        '\033[90mManaged Databases\033[0m                      \033[90mFolder Operations Count\033[0m\n'\
        '6. Show MDB quota                      18. Show folder quota by folder\n'\
        '7. Set MDB quota                       19. Set folder quota by folder\n'\
        '8. Set MDB quota to default            20. Show folder quota by cloud\n'\
        '9. Set MDB quota to zero               21. Set folder quota by cloud\n\n'\
        '\033[90mCloud Functions\033[0m                        \033[90mContainer Registry\033[0m\n'\
        '10. Show cloud functions quota         22. Show container registry quota\n'\
        '11. Set cloud functions quota          23. Set container registry quota\n\n'\
        '\033[90mInstance Group\033[0m                         \033[90mManaged Kubernetes\033[0m\n'\
        '12. Show instance group quota          24. Show kubernetes quota\n'\
        '13. Set instance group quota           25. Set kubernetes quota\n\n'\
        'a - show all services quota            z – set all quota to zero\n'\
        'h - help (this menu)                   q - exit'.format(env=ENV_MODE)


COMPUTE_DEFAULT_VALUES = {
    'cores': 32,
    'disk-count': 32,
    'external-address-count': 8,
    'external-static-address-count': 2,
    'image-count': 32,
    'instance-count': 12,
    'memory': 137438953472,
    'network-count': 2,
    'network-ssd-total-disk-size': 214748364800,
    'network-hdd-total-disk-size': 536870912000,
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

NEW_MDB_DEFAULT_VALUES = {
    'mdb.clusters.count': 16,
    'mdb.cpu.count': 64,
    'mdb.gpu.count': 0,
    'mdb.hdd.size': 4398046511104,
    'mdb.memory.size': 549755813888,
    'mdb.ssd.size': 4398046511104
}

CONTAINER_REGISTRY_DEFAULT_VALUES = {}

INSTANCE_GROUP_DEFAULT_VALUES = {}

MANAGED_KUBERNETES_DEFAULT_VALUES = {}

SERVERLESS_DEFAULT_VALUES = {}


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
    res = json.loads(r.text)
    logging.debug(res)
    return res


def iam_token_updater():
    data = get_iam(oauth_token)
    token = data.get('iamToken')
    expires = data.get('expiresAt').split('.')[0]

    with open('{home}/.rei/rei.cfg'.format(home=home_dir), 'w') as cfgfile:
        try:
            config.add_section('IAM')
        except configparser.DuplicateSectionError as e:
            logging.debug(e)
        config.set('IAM', 'token', token)
        config.set('IAM', 'expires', expires)
        config.write(cfgfile)
    cfgfile.close()


def get_cached_token():
    if not os.path.exists('{home}/.rei'.format(home=home_dir)):
        os.system('mkdir -p {home}/.rei'.format(home=home_dir))

    try:
        cached_token = config.get('IAM', 'token')
        expires_token = config.get('IAM', 'expires')
    except Exception:
        iam_token_updater()

    cached_token = config.get('IAM', 'token')
    expires_token = config.get('IAM', 'expires')
    expires = datetime.strptime(expires_token, '%Y-%m-%dT%H:%M:%S')
    timenow = datetime.utcnow()

    if args.preprod:
        return get_iam(oauth_token).get('iamToken')
    elif expires > timenow:
        logging.debug('IAM token is fresh. Use cached iam token')
        return cached_token
    else:
        logging.debug('IAM token is expired. Update...')
        iam_token_updater()
        return config.get('IAM', 'token')


# gRPC API

SERVICES = {
    'serverless': serverless_quota_service,
    'triggers': triggers_quota_service,
    'instance_group': instance_group_quota_service,
    'kubernetes': k8s_quota_service,
    'container_registry': container_registry_quota_service,
    'compute': compute_quota_service,
    'monitoring': monitoring_quota_service,
    'vpc': vpc_quota_service,
    'ai': ai_quota_service,
    'iot': iot_quota_service
}


# Global grpc funcs

def quota_service(service):
    if service not in SERVICES.keys():
        raise 'No service named {} supported'.format(service)

    iam_token = get_cached_token()
    with open(ca_cert, 'rb') as cert:
        ssl_creds = grpc.ssl_channel_credentials(cert.read())
    call_creds = grpc.access_token_call_credentials(iam_token)
    chan_creds = grpc.composite_channel_credentials(ssl_creds, call_creds)

    stub = SERVICES[service].QuotaServiceStub
    channel = grpc.secure_channel(ENDPOINTS[service], chan_creds)

    return stub(channel)


def grpc_get_quota(cloud_id, service):
    stub = quota_service(service)
    req = quota_pb2.GetQuotaRequest(cloud_id=cloud_id)
    resp = stub.Get(req)
    return resp.metrics


def grpc_set_quota(cloud_id, service, name, value):
    stub = quota_service(service)
    if name in BYTES_QUOTA:
        limit = bytes_wrapper(value)
    else:
        limit = value
    req = quota_pb2.UpdateQuotaMetricRequest(
        cloud_id=cloud_id,
        metric=quota_pb2.MetricLimit(
            name=name,
            limit=int(limit),
        ),
    )
    stub.UpdateMetric(req)
    logging.debug(req)
    print('\033[92m{name} OK\033[0m'.format(name=name))


# IOT

def iot_list_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'iot')
    t = PrettyTable()
    t.field_names = ["Name", "Limit", "Used"]
    t.align = "l"

    for i in quotas:
        if i.name in BYTES_QUOTA:
            limit = size_wrapper(i.limit)
            used = size_wrapper(i.usage)
        else:
            limit = i.limit
            used = i.value
        t.add_row([i.name, limit, used])

    print(t)


# AI

def ai_list_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'ai')
    t = PrettyTable()
    t.field_names = ["Name", "Limit", "Used"]
    t.align = "l"

    for i in quotas:
        if i.name in BYTES_QUOTA:
            limit = size_wrapper(i.limit)
            used = size_wrapper(i.usage)
        else:
            limit = i.limit
            used = i.value
        t.add_row([i.name, limit, used])

    print(t)


# Monitoring


def monitoring_list_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'monitoring')
    t = PrettyTable()
    t.field_names = ["Name", "Limit", "Used"]
    t.align = "l"

    for i in quotas:
        if i.name in BYTES_QUOTA:
            limit = size_wrapper(i.limit)
            used = size_wrapper(i.value)
        else:
            limit = i.limit
            used = i.value
        t.add_row([i.name, limit, used])

    print(t)


# VPC

def vpc_list_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'vpc')
    t = PrettyTable()
    t.field_names = ["Name", "Limit", "Used"]
    t.align = "l"

    for i in quotas:
        if i.name in BYTES_QUOTA:
            limit = size_wrapper(i.limit)
            used = size_wrapper(i.value)
        else:
            limit = i.limit
            used = i.value
        t.add_row([i.name, limit, used])

    print(t)


# Compute gRPC

def grpc_compute_list_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'compute')
    t = PrettyTable()
    t.field_names = ["Name", "Limit", "Used"]
    t.align = "l"

    for i in quotas:
        if i.name in BYTES_QUOTA:
            limit = size_wrapper(i.limit)
            used = size_wrapper(i.value)
        else:
            limit = i.limit
            used = i.value
        t.add_row([i.name, limit, used])
    print(t)


def grpc_compute_multiset_quota(cloud_id):
    print('\nCompute quota set (gRPC)')
    quotas = grpc_get_quota(cloud_id, 'compute')
    print('\nPress ENTER to make no changes.\n')
    for q in quotas:
        name = q.name
        if name in BYTES_QUOTA:
            limit = size_wrapper(q.limit)
        else:
            limit = q.limit
        while True:
            try:
                new_limit = input('{name} [{limit}]: '.format(name=name, limit=limit))
                if new_limit == '':
                    break
                elif new_limit == 'q':
                    print('Aborted\n')
                    grpc_compute_list_quota(cloud_id)
                    return False
                else:
                    grpc_set_quota(cloud_id, 'compute', name, new_limit)
                    break
            except ValueError:
                print('Limit must be integer')
            except TypeError as e:
                logging.debug(e)
    return grpc_compute_list_quota(cloud_id)


# Serverless

def serverless_list_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'serverless')
    t = PrettyTable()
    t.field_names = ["Name", "Limit", "Used"]
    t.align = "l"

    for i in quotas:
        if i.name in BYTES_QUOTA:
            limit = size_wrapper(i.limit)
            used = size_wrapper(i.value)
        else:
            limit = i.limit
            used = i.value
        t.add_row([i.name, limit, used])

    triggers_quota = grpc_get_quota(cloud_id, 'triggers')
    for i in triggers_quota:
        t.add_row([i.name, i.limit, i.value])

    print(t)


def triggers_multiset_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'triggers')
    for q in quotas:
        name = q.name
        if name in BYTES_QUOTA:
            limit = size_wrapper(q.limit)
        else:
            limit = q.limit
        while True:
            try:
                new_limit = input('{name} [{limit}]: '.format(name=name, limit=limit))
                if new_limit == '':
                    break
                elif new_limit == 'q':
                    print('Aborted\n')
                    serverless_list_quota(cloud_id)
                    return False
                else:
                    grpc_set_quota(cloud_id, 'triggers', name, new_limit)
                    break
            except ValueError:
                print('Limit must be integer')
            except TypeError as e:
                logging.debug(e)


def serverless_multiset_quota(cloud_id):
    print('\nServerless quota set')
    quotas = grpc_get_quota(cloud_id, 'serverless')
    print('\nPress ENTER to make no changes.\n')
    for q in quotas:
        name = q.name
        if name in BYTES_QUOTA:
            limit = size_wrapper(q.limit)
        else:
            limit = q.limit
        while True:
            try:
                new_limit = input('{name} [{limit}]: '.format(name=name, limit=limit))
                if new_limit == '':
                    break
                elif new_limit == 'q':
                    print('Aborted\n')
                    serverless_list_quota(cloud_id)
                    return False
                else:
                    grpc_set_quota(cloud_id, 'serverless', name, new_limit)
                    break
            except ValueError:
                print('Limit must be integer')
            except TypeError as e:
                logging.debug(e)
    triggers_multiset_quota(cloud_id)
    return serverless_list_quota(cloud_id)


def serverless_quota_resources():
    table = PrettyTable()
    table.field_names = ['SERVERLESS']
    table.align = 'l'
    for name in sorted(SERVERLESS_QUOTA_NAMES):
        table.add_row([name])
    print(table)


# Instance group

def instance_group_list_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'instance_group')
    t = PrettyTable()
    t.field_names = ["Name", "Limit", "Used"]
    t.align = "l"
    for i in quotas:
        t.add_row([i.name, i.limit, i.value])
    print(t)


def instance_group_multiset_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'instance_group')
    print('\nPress ENTER to make no changes.\n')
    for q in quotas:
        name = q.name
        limit = q.limit
        while True:
            try:
                new_limit = input('{name} [{limit}]: '.format(name=name, limit=limit))
                if new_limit == '':
                    break
                else:
                    grpc_set_quota(cloud_id, 'instance_group', name, int(new_limit))
                    break
            except ValueError:
                print('Limit must be integer')
            except TypeError as e:
                logging.debug(e)
    return instance_group_list_quota(cloud_id)


def instance_group_quota_resources():
    table = PrettyTable()
    table.field_names = ['INSTANCE GROUP']
    table.align = 'l'
    for name in sorted(INSTANCE_GROUP_QUOTA_NAMES):
        table.add_row([name])
    print(table)

# Kubernetes

def kubernetes_list_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'kubernetes')
    t = PrettyTable()
    t.field_names = ["Name", "Limit", "Used"]
    t.align = "l"
    for i in quotas:
        if i.name in BYTES_QUOTA:
            limit = size_wrapper(i.limit)
            used = size_wrapper(i.value)
        else:
            limit = i.limit
            used = i.value
        t.add_row([i.name, limit, used])
    print(t)


def kubernetes_multiset_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'kubernetes')
    print('\nPress ENTER to make no changes.\n')
    for q in quotas:
        name = q.name
        if name in BYTES_QUOTA:
            limit = size_wrapper(q.limit)
        else:
            limit = q.limit
        while True:
            try:
                new_limit = input('{name} [{limit}]: '.format(name=name, limit=limit))
                if new_limit == '':
                    break
                elif new_limit == 'q':
                    print('Aborted\n')
                    kubernetes_list_quota(cloud_id)
                    return False
                else:
                    grpc_set_quota(cloud_id, 'kubernetes', name, new_limit)
                    break
            except ValueError:
                print('Limit must be integer')
            except TypeError as e:
                logging.debug(e)
    return kubernetes_list_quota(cloud_id)


def kubernetes_quota_resources():
    table = PrettyTable()
    table.field_names = ['KUBERNETES']
    table.align = 'l'
    for name in sorted(KUBERNETES_QUOTA_NAMES):
        table.add_row([name])
    print(table)


# Container Registry

def container_registry_list_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'container_registry')
    t = PrettyTable()
    t.field_names = ["Name", "Limit", "Used"]
    t.align = "l"
    for i in quotas:
        t.add_row([i.name, i.limit, i.value])
    print(t)


def container_registry_multiset_quota(cloud_id):
    quotas = grpc_get_quota(cloud_id, 'container_registry')
    print('\nPress ENTER to make no changes.\n')
    for q in quotas:
        name = q.name
        limit = q.limit
        while True:
            try:
                new_limit = input('{name} [{limit}]: '.format(name=name, limit=limit))
                if new_limit == '':
                    break
                else:
                    grpc_set_quota(cloud_id, 'container_registry', name, int(new_limit))
                    break
            except ValueError:
                print('Limit must be integer')
            except TypeError as e:
                logging.debug(e)
    return container_registry_list_quota(cloud_id)


def container_registry_quota_resources():
    table = PrettyTable()
    table.field_names = ['CONTAINER REGISTRY']
    table.align = 'l'
    for name in sorted(CONTAINER_REGISTRY_QUOTA_NAMES):
        table.add_row([name])
    print(table)


# REST API

# Resource Manager (Folders)

@retry((ConnectionError, Timeout))
def folder_quota_get(folder_id):
    iam_token = get_cached_token()
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
    logging.debug(quotas)
    if quotas:
        for q in quotas['metrics']:
            table.add_row([q['name'], q['limit'], q['value']])
        print(table)
    else:
        print('\n\033[91mERROR:\033[0m Folder not found.')


@retry((ConnectionError, Timeout))
def folder_quota_set(folder_id, name, limit):
    iam_token = get_cached_token()
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
    logging.debug(result)
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
                except TypeError as e:
                    logging.debug(e)
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
    logging.debug(folders)
    if folders:
        for folder_id in folders:
            for metric in folder_id['metrics']:
                table.add_row([folder_id['folder_id'], metric['name'], metric['limit'], metric['value']])
        print(table)
    else:
        print('\n\033[91mERROR:\033[0m Cloud not found.')


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
    iam_token = get_cached_token()
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
    logging.debug(quotas)
    if quotas:
        quotas_dict = {}
        for quota in quotas['metrics']:
            quotas_dict[quota['name']]={'limit': quota['limit'], 'value': quota['value']}
        for name in list(sorted(quotas_dict)):
            value = quotas_dict[name]['value']
            limit = quotas_dict[name]['limit']
            key = name

            if name in BYTES_QUOTA:
                lim, val = size_wrapper(limit), size_wrapper(value)
            else:
                lim, val = limit, value

            if limit <= value:
                key = '\033[91m{key}\033[0m'.format(key=name)
                lim = '\033[91m{lim}\033[0m'.format(lim=lim)
                val = '\033[91m{val}\033[0m'.format(val=val)

            table.add_row([key, lim, val])
        print(table)


@retry((ConnectionError, Timeout))
def compute_quota_set(cloud_id, name, limit):
    iam_token = get_cached_token()
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
    iam_token = get_cached_token()
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
                except TypeError as e:
                    logging.debug(e)
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
    table.field_names = ['COMPUTE']
    table.align = 'l'
    for name in sorted(COMPUTE_QUOTA_NAMES):
        table.add_row([name])
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
        if value is not None:
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
    iam_token = get_cached_token()
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
    quotas = object_storage_quota_get(cloud_id)
    logging.debug(quotas)

    if quotas:
        data = {
            "buckets_count": {
                "limit": quotas['buckets_count_quota'],
                "used": quotas['buckets_count']
            },
            "total_size": {
                "limit": quotas['total_size_quota'],
                "used": quotas['total_size']
            }
        }

        for primary_key in data:
            name = primary_key
            used = data[primary_key]["used"]
            limit = data[primary_key]["limit"]

            if primary_key in BYTES_QUOTA:
                table.add_row([name, size_wrapper(limit), size_wrapper(used)])
            else:
                table.add_row([name, limit, used])

        print(table)


@retry((ConnectionError, Timeout))
def object_storage_quota_set(cloud_id, name, quota_value):
    iam_token = get_cached_token()
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
                except TypeError as e:
                    logging.debug(e)
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


def object_storage_quota_resources():
    table = PrettyTable()
    table.field_names = ['S3']
    table.align = 'l'

    for name in sorted(OBJECT_STORAGE_QUOTA_NAMES):
        table.add_row([name])

    print(table)

# Managed Database (MDB)

# DEPRECATED
@retry((ConnectionError, Timeout))
def mdb_quota_get(cloud_id):
    iam_token = get_cached_token()
    headers = {
        'X-YaCloud-SubjectToken': iam_token
    }
    req = requests.get(MDB_URL + cloud_id, headers=headers)
    res = json.loads(req.text)
    if req.ok:
        return res
    else:
        return req.status_code


# DEPRECATED
def mdb_quota_list(cloud_id):
    table = PrettyTable()
    table.field_names = ['Name', 'Limit', 'Used']
    table.align = 'l'
    quotas = mdb_quota_get(cloud_id)
    logging.debug(quotas)
    if quotas == 503:
        print('\n\033[91mERROR:\033[0m Cloud not exist.')
    elif quotas == 403:
        print('\n\033[91mERROR:\033[0m 403. Cloud not found.')
    elif 'cloudId' in quotas:
        table.add_row(['Clusters', quotas['clustersQuota'], quotas['clustersUsed']])
        table.add_row(['CPU', quotas['cpuQuota'], quotas['cpuUsed']])
        table.add_row(['GPU', quotas['gpuQuota'], quotas['gpuUsed']])
        table.add_row(['Memory', size_wrapper(quotas['memoryQuota']), size_wrapper(quotas['memoryUsed'])])
        table.add_row(['HDD space', size_wrapper(quotas['hddSpaceQuota']), size_wrapper(quotas['hddSpaceUsed'])])
        table.add_row(['SSD space', size_wrapper(quotas['ssdSpaceQuota']), size_wrapper(quotas['ssdSpaceUsed'])])
        table.add_row(['IO', size_wrapper(quotas['ioQuota']), size_wrapper(quotas['ioUsed'])])
        table.add_row(['Network', size_wrapper(quotas['networkQuota']), size_wrapper(quotas['networkUsed'])])
        print(table)
    else:
        names = ('Clusters', 'CPU', 'Memory', 'HDD space', 'SSD space', 'IO', 'Network')
        for n in names:
            table.add_row([n, 0, 0])
        print(table)


# DEPRECATED
def mdb_resource_preset():
    while True:
        preset = str(input('Enter resource preset [default s1.nano]: '))
        print(preset)
        if preset == '':
            preset = 's1.nano'
            return preset
        elif preset in MDB_PRESET_LIST:
            return preset
        else:
            print('Unknown preset recieved.\nSupported preset: ')
            print(', '.join(x for x in MDB_PRESET_LIST))


# DEPRECATED
@retry((ConnectionError, Timeout))
def mdb_quota_set(cloud_id, action, limit, target, preset='s1.nano'):
    iam_token = get_cached_token()
    headers = {
        'X-YaCloud-SubjectToken': iam_token,
        'content-type': 'application/json'
    }
    if target == 'resources':
        data = {
            'action': action,
            'count': int(limit),
            'presetId': preset
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


# DEPRECATED
def mdb_quota_manage(cloud_id, act, preset='s1.nano'):
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
                            mdb_quota_set(cloud_id, act, int(raw_limit), q_name, preset=preset)
                        else:
                            mdb_quota_set(cloud_id, act, int(new_value), q_name, preset=preset)
                        break
                except ValueError:
                    print('Value must be integer')
                except TypeError as e:
                    logging.debug(e)
        mdb_quota_list(cloud_id)
    else:
        print('\n{cloud} is virgin cloud without default quota'.format(cloud=cloud_id))
        defloration = input('Do you want to add default values for this cloud? (y/n): ')
        if defloration == 'y':
            mdb_default_quota(cloud_id)


# DEPRECATED
def mdb_replace_quota(cloud_id, preset='s1.nano'):
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
                            mdb_quota_set(cloud_id, action, int(new_value), q_name, preset=preset)
                        elif input_value == original_value:
                            print("Identical values, skip")
                        else:
                            new_value = input_value - original_value
                            action = 'add'
                            mdb_quota_set(cloud_id, action, int(new_value), q_name, preset=preset)
                        break
                except ValueError:
                    print('Value must be integer')
                except TypeError as e:
                    logging.debug(e)
        mdb_quota_list(cloud_id)
    else:
        print('\n{cloud} is virgin cloud without default quota'.format(cloud=cloud_id))
        defloration = input('Do you want to add default values for this cloud? (y/n): ')
        if defloration == 'y':
            mdb_default_quota(cloud_id)


# DEPRECATED
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


# DEPRECATED
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


# DEPRECATED
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


# DEPRECATED
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


# DEPRECATED
def mdb_shortlink_check(cloud_id, datadict):
    table = PrettyTable()
    table.field_names = ['Name', 'New limit']
    table.align = 'l'
    original_mdb_quota = {}
    quotas = mdb_quota_get(cloud_id)
    if not quotas:
        mdb_replace_default_quota(cloud_id)
        print('\nRelax, i am just added default MDB quota for {cloud}, because MDB quota was equal to 0.\n'.format(cloud=cloud_id))

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


# DEPRECATED
def mdb_preset_list():
    table = PrettyTable()
    table.field_names = ['PRESET']
    table.align = 'l'
    for name in sorted(MDB_PRESET_LIST):
        table.add_row([name])
    print(table)


# MDB API v2 [New spec]
# TODO: deprecate autoset quota if quota is null

@retry((ConnectionError, Timeout))
def new_mdb_quota_set(cloud_id, target, limit, manual=False):
    iam_token = get_cached_token()
    headers = {
        'X-YaCloud-SubjectToken': iam_token,
        'content-type': 'application/json'
    }

    if manual:
        if target in BYTES_QUOTA:
            quota_limit = bytes_wrapper(limit)
        else:
            quota_limit = int(limit)
    else:
        quota_limit = limit

    data = {
        "cloud_id": cloud_id,
        "metrics": [
            {
                "name": target,
                "limit": quota_limit
            }
        ]
    }

    req = requests.post(MDB_V2_URL, data=json.dumps(data), headers=headers)
    result = json.loads(req.text)
    logging.debug(result)

    if req.ok:
        print('\033[92m{name} OK\033[0m'.format(name=target))
    else:
        print('\033[91mERROR.\033[0m API: {msg}'.format(msg=result['message']))


@retry((ConnectionError, Timeout))
def new_mdb_quota_get(cloud_id):
    iam_token = get_cached_token()

    headers = {
        'X-YaCloud-SubjectToken': iam_token,
        'content-type': 'application/json'
    }

    req = requests.get(MDB_V2_URL + '/' + cloud_id, headers=headers)
    response = json.loads(req.text)
    logging.debug(response)

    if req.ok:
        return response
    else:
        print('\n\033[91mERROR:\033[0m {msg}'.format(msg=response['message']))


def new_mdb_quota_list(cloud_id):
    table = PrettyTable()
    table.field_names = ['Name', 'Limit', 'Used']
    table.align = 'l'

    quotas = new_mdb_quota_get(cloud_id)
    logging.debug(quotas)

    if quotas:
        quotas_dict = {}
        for quota in quotas['metrics']:
            quotas_dict[quota['name']]={'limit': quota['limit'], 'value': quota['usage']}
        for name in list(sorted(quotas_dict)):
            value = quotas_dict[name]['value']
            limit = quotas_dict[name]['limit']
            key = name

            if name in BYTES_QUOTA:
                lim, val = size_wrapper(limit), size_wrapper(value)
            else:
                lim, val = limit, value

            if limit <= value:
                key = '\033[91m{key}\033[0m'.format(key=name)
                lim = '\033[91m{lim}\033[0m'.format(lim=lim)
                val = '\033[91m{val}\033[0m'.format(val=val)

            table.add_row([key, lim, val])
        print(table)


def new_mdb_multiset_quota(cloud_id):
    quotas = new_mdb_quota_get(cloud_id)
    if quotas:
        print('\nPress ENTER to make no changes.')
        print('disk-size, memory, etc must be B, K, M, G, T (example: 1B, 10K, 100M, 100G, 2T)\n')
        quotas_dict = {}
        for quota in quotas['metrics']:
            quotas_dict[quota['name']]={'limit': quota['limit'], 'value': quota['usage']}
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
                        new_mdb_quota_list(cloud_id)
                        return False
                    else:
                        if name in BYTES_QUOTA:
                            raw_limit = bytes_wrapper(new_limit)
                            new_mdb_quota_set(cloud_id, name, int(raw_limit))
                        else:
                            new_mdb_quota_set(cloud_id, name, int(new_limit))
                        break
                except ValueError:
                    print('Limit must be integer')
                except TypeError as e:
                    logging.debug(e)
        new_mdb_quota_list(cloud_id)


def new_mdb_shortlink_check(cloud_id, datadict):
    table = PrettyTable()
    table.field_names = ['Name', 'New limit']
    table.align = 'l'

    original_mdb_quota = {}
    for mdb_quota in new_mdb_quota_get(cloud_id)['metrics']:
        name = mdb_quota.get('name')
        limit = mdb_quota.get('limit')
        original_mdb_quota[name] = limit

    data = {}
    for key, value in datadict.items():
        if value != original_mdb_quota.get(key):
            data[key] = value
            table.add_row([key, value if key not in BYTES_QUOTA else size_wrapper(value)])

    if data:
        print('MDB \n{table}\n'.format(table=table))
        return data



def new_mdb_default_quota(cloud_id):
    for key, value in NEW_MDB_DEFAULT_VALUES.items():
        new_mdb_quota_set(cloud_id, key, value)
    new_mdb_quota_list(cloud_id)


def new_mdb_zero_quota(cloud_id):
    quotas = new_mdb_quota_get(cloud_id)
    if quotas:
        for q in quotas['metrics']:
            new_mdb_quota_set(cloud_id, q['name'], 0)
        new_mdb_quota_list(cloud_id)


def mdb_quota_resources():
    table = PrettyTable()
    table.field_names = ['MDB']
    table.align = 'l'

    for name in sorted(MDB_QUOTA_NAMES):
        table.add_row([name])

    print(table)


# Other

def all_quotas_list(cloud_id):
    backoffice_link = 'https://backoffice.cloud.yandex.ru/clouds/{cloud}?section=quotas'.format(cloud=cloud_id)
    print('\nCompute')
    compute_quota_list(cloud_id)
    print('\nObject Storage')
    object_storage_quota_list(cloud_id)
    print('\nManaged Database')
    new_mdb_quota_list(cloud_id)
    print('\nServerless')
    serverless_list_quota(cloud_id)
    print('\nManaged Kubernetes')
    kubernetes_list_quota(cloud_id)
    print('\nInstance Group')
    instance_group_list_quota(cloud_id)
    print('\nContainer Registry')
    container_registry_list_quota(cloud_id)
    print('\nFolder Operation Count')
    folders_by_cloud_list_table(cloud_id)
    print('\nBackoffice: {link}\n'.format(link=backoffice_link))


def all_zero_set_quota(cloud_id):
    print('\n\033[91mWARNING: This option set [Compute, MDB and S3] quota for cloud {cloud} to ZERO values.\033[0m'.format(cloud=cloud_id))
    checkout = input('Continue? (y/n): ')
    if checkout == "y" or checkout == "yes":
        new_mdb_zero_quota(cloud_id)
        compute_zero_quota(cloud_id)
        object_storage_zero_quota(cloud_id)
    else:
        print('Aborted.')


@retry((ConnectionError, Timeout))
def quota_set_from_url(token):
    try:
        r = requests.get(QUOTA_CALC + token)
        data = json.loads(r.text)
        logging.debug(data)
    except json.decoder.JSONDecodeError:
        print('\033[91mERROR.\033[0m Token from quotacalc expired')
        quit()

    cloud_id = data.get('cloudId')
    compute = compute_shortlink_check(cloud_id, data.get('compute'))
    mdb = new_mdb_shortlink_check(cloud_id, data.get('mdb'))
    s3 = object_storage_shortlink_check(cloud_id, data.get('s3'))

    if not s3 and not compute and not mdb:
        print('\033[91mNo changes found\033[0m')
        return

    check = input('\033[91mPlease, verify new limits for CloudID {cloud}.\033[0m \nApply changes? (y/n): '.format(cloud=cloud_id))
    if check == 'y' or check == 'yes':
        if compute:
            for c_key, c_value in compute.items():
                compute_quota_set(cloud_id, c_key, c_value)
        if mdb:
            for m_key, m_value in mdb.items():
                new_mdb_quota_set(cloud_id, m_key, m_value)
        if s3:
            for s_key, s_value in s3.items():
                object_storage_quota_set(cloud_id, s_key, s_value)
        all_quotas_list(cloud_id)

    else:
        print('\nCanceled.\nBye!\n')


def qctl_update():
    os.system('quotactl-update')


# Argparse atribute error bypass
def main():
    pass


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
            print('Default compute quota set\n')
            compute_default_quota(get_cloud_id())
        elif choose == '4':
            print('Zero compute quota set\n')
            compute_zero_quota(get_cloud_id())
        elif choose == '5':
            print('Multiplier compute quota set\n')
            compute_multiply_quota(get_cloud_id())
        elif choose == '6':
            print('MDB quota list\n')
            new_mdb_quota_list(get_cloud_id())
        elif choose == '7':
            print('MDB quota set\n')
            new_mdb_multiset_quota(get_cloud_id())
        elif choose == '8':
            print('MDB default quota set\n')
            new_mdb_default_quota(get_cloud_id())
        elif choose == '9':
            print('MDB zero quota set\n')
            new_mdb_zero_quota(get_cloud_id())
        elif choose == '10':
            print('Cloud functions quota show\n')
            serverless_list_quota(get_cloud_id())
        elif choose == '11':
            print('Cloud functions quota show\n')
            serverless_multiset_quota(get_cloud_id())
        elif choose == '12':
            print('Show instance group quota\n')
            instance_group_list_quota(get_cloud_id())
        elif choose == '13':
            print('Set instance group quota\n')
            instance_group_multiset_quota(get_cloud_id())
        elif choose == '14':
            print('S3 quota list\n')
            object_storage_quota_list(get_cloud_id())
        elif choose == '15':
            print('S3 quota set\n')
            object_storage_quota(get_cloud_id())
        elif choose == '16':
            print('S3 zero quota set\n')
            object_storage_zero_quota(get_cloud_id())
        elif choose == '17':
            print('S3 default quota set\n')
            object_storage_default_quota(get_cloud_id())
        elif choose == '18':
            print('Folder quota show\n')
            folder_quota_list(input('Enter Folder ID: ').strip())
        elif choose == '19':
            print('Folder quota set\n')
            folder_multiset_quota(input('Enter Folder ID: ').strip())
        elif choose == '20':
            print('All folders quota SHOW by cloud id\n')
            folders_by_cloud_list_table(get_cloud_id())
        elif choose == '21':
            print('All folders quota SET by cloud id\n')
            folders_by_cloud_set(get_cloud_id(), int(input('\nNew active-operation-count limit: ')))
        elif choose == '22':
            print('Show container registry quota\n')
            container_registry_list_quota(get_cloud_id())
        elif choose == '23':
            print('Set container registry quota\n')
            container_registry_multiset_quota(get_cloud_id())
        elif choose == '24':
            print('Show kubernetes quota\n')
            kubernetes_list_quota(get_cloud_id())
        elif choose == '25':
            print('Set kubernetes quota\n')
            kubernetes_multiset_quota(get_cloud_id())

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
            print('\nInput error. Please type digits (1-23) or "h" for help.')


# Argparse

try:
    if args.compute:
        if re.fullmatch('b1+\w{18}', args.compute) \
            or re.fullmatch('ao+\w{18}', args.compute):
            if args.set:
                if args.name and args.limit:
                    if args.grpc:
                        grpc_get_quota(args.compute, 'compute', args.name, args.limit)
                        grpc_compute_list_quota(args.compute)
                    else:
                        compute_quota_manual_set(args.compute, args.name, args.limit)
                        compute_quota_list(args.compute)
                else:
                    if args.grpc:
                        grpc_compute_multiset_quota(args.compute)
                    else:
                        compute_multiset_quota(args.compute)
            elif args.from_json:
                compute_quota_set_json(args.compute, args.from_json)
            else:
                if args.grpc:
                    grpc_compute_list_quota(args.compute)
                else:
                    compute_quota_list(args.compute)
        else:
            print('\nError. Usage: ./quotas -c <CLOUD_ID>\n')

    elif args.zero:
        all_zero_set_quota(args.zero)

    elif args.mdb:
        if args.set:
            if args.limit and args.name:
                new_mdb_quota_set(args.mdb, args.name, args.limit, manual=True)
                new_mdb_quota_list(args.mdb)
            else:
                new_mdb_multiset_quota(args.mdb)
        else:
            new_mdb_quota_list(args.mdb)

    elif args.storage:
        if args.set:
            if args.limit and args.name:
                object_storage_quota_set(args.storage, args.name, args.limit)
                object_storage_quota_list(args.storage)
            else:
                object_storage_quota(args.storage)
        else:
            object_storage_quota_list(args.storage)

    elif args.serverless:
        if args.set:
            if args.limit and args.name:
                service = 'triggers' if args.name in TRIGGERS_QUOTA_NAMES else 'serverless'
                grpc_set_quota(args.serverless, service, args.name, args.limit)
                serverless_list_quota(args.serverless)
            else:
                serverless_multiset_quota(args.serverless)
        else:
            serverless_list_quota(args.serverless)

    elif args.kubernetes:
        if args.set:
            if args.limit and args.name:
                grpc_set_quota(args.kubernetes, 'kubernetes', args.name, args.limit)
                kubernetes_list_quota(args.kubernetes)
            else:
                kubernetes_multiset_quota(args.kubernetes)
        else:
            kubernetes_list_quota(args.kubernetes)

    elif args.instance_group:
        if args.set:
            if args.limit and args.name:
                grpc_set_quota(args.instance_group, 'instance_group', args.name, args.limit)
                instance_group_list_quota(args.instance_group)
            else:
                instance_group_multiset_quota(args.instance_group)
        else:
            instance_group_list_quota(args.instance_group)

    elif args.container_registry:
        if args.set:
            if args.limit and args.name:
                grpc_set_quota(args.container_registry, 'container_registry', args.name, args.limit)
                container_registry_list_quota(args.container_registry)
            else:
                container_registry_multiset_quota(args.container_registry)
        else:
            container_registry_list_quota(args.container_registry)

    elif args.token:
        quota_set_from_url(args.token)

    elif args.resources:
        compute_quota_resources()
        serverless_quota_resources()
        kubernetes_quota_resources()
        mdb_quota_resources()
        object_storage_quota_resources()
        instance_group_quota_resources()
        container_registry_quota_resources()

    # elif args.preset_list:
    #     mdb_preset_list()

    elif args.all_quota:
        all_quotas_list(args.all_quota)

    elif args.init:
        init_config_setup()

    elif args.update:
        qctl_update()

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

    elif args.test:
        print('YOU DIED')
        iot_list_quota(args.test)
        # monitoring_list_quota(args.test)  # StatusCode.UNIMPLEMENTED
        # try:
        #     vpc_list_quota(args.test)
        # except Exception as e:
        #     print(e)

        # mdb_quota_set(args.test, 'add', '1', 'gpus')
        # new_mdb_quota_set(args.test, args.name, args.limit)
        # new_mdb_quota_list(args.test)
        # new_mdb_multiset_quota(args.test)
        # new_mdb_default_quota(args.test)
        # new_mdb_zero_quota(args.test)
        # test_grpc_compute(args.test)

    else:
        menu()
except KeyboardInterrupt:
    print('\nExit')
    quit()
