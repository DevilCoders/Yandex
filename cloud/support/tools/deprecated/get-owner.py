#!/usr/bin/env python3

import requests
import json
import os
import argparse
import configparser
from os.path import expanduser
from collections import OrderedDict
from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

homeDir = expanduser("~")
config = configparser.RawConfigParser()
config.read('{}/.rei/rei.cfg'.format(homeDir))

parser = argparse.ArgumentParser(description='Ugly prototype of AIO support tool. Can do stuff. The easy way.')
parser.add_argument("--cloud", "-c", required=False, type=str,
    help="Resolve creator login with specified <cloud_id>")
parser.add_argument("--allfolders", "-af", default=False, action='store_true',
    help="Set this flag to display all folders when searching for cloud with --cloud")
parser.add_argument("--login", "-l", required=False, type=str,
    help="Resolve list of all clouds where specified user login is present. \
    Use --role to resolve only clouds where user have a specific role")
parser.add_argument("--name", "-n", required=False, type=str,
    help="Resolve cloud by name")
parser.add_argument("--bucket", "-s3", required=False, type=str,
    help="Resolve S3 bucket by name")
parser.add_argument("--billing", "-b", required=False, action='store_true',
    help="Resolve billing info when using --cloud or --login.")
parser.add_argument("--get-billing", "-gb", required=False, type=str,
    help="Resolve billing ID.")
parser.add_argument("--folder", "-f", required=False, type=str,
    help="Show folder info")
parser.add_argument("--instance", "-i", required=False, type=str,
    help="Get basic VM information")
parser.add_argument("--all-instances", "-ai", required=False, type=str,
    help="List all instances for <CLOUD_ID>")
parser.add_argument("--ip", "-ip", required=False, type=str,
    help="Resolve instance by IP")
parser.add_argument("--clusters", "-cr", required=False, type=str,
    help="List all clusters for <cloud_id>")
parser.add_argument("--get-cl-op", "-clop", required=False, type=str,
    help="Get OP by OP_ID")
parser.add_argument("--metadata", "-m", default=False, action='store_true',
    help="Set this flag to display VM metadata when searching for instance with --instance")
parser.add_argument("--network", "-nt", default=False, action='store_true',
    help="Set this flag to resolve all networks with their subnets for <cloud_id>")
parser.add_argument("--users", "-ru", default=False, action='store_true',
    help="Set this flag to resolve all users in provided <cloud_id>")
parser.add_argument("--role", "-r", required=False, type=str,
    help="Used to specify user role when searching for cloud with --login. \
    See --rolelist for list of available roles")
parser.add_argument("--rolelist", "-rl", required=False, action='store_true',
    help="Print available roles.")
parser.add_argument("--get-cluster-ops", "-clops", required=False, action='store_true',
    help="Get cluster operations")
parser.add_argument("--operations", "-o", required=False, action='store_true',
    help="List instance actions")
parser.add_argument("--debug", "-d", required=False, action='store_true',
    help="When everything goes wrong. For now only works when resolving YDB clusters.")
args = parser.parse_args()

roles = {'admin': 'admin',
'editor': 'editor',
'member': 'resource-manager.clouds.member',
'owner': 'resource-manager.clouds.owner',
'viewer': 'viewer'}

try:
    oauthToken=config.get('REI_AUTH', 'oAuth_token')
except configparser.NoSectionError:
    print("Corrupted config or no config file present.")
    quit()


def getOps(data, token):
    final = []
    IAAS_URL = 'https://compute.api.cloud.yandex.net/compute/v1/instances/{}/operations'.format(data)
    r = requests.get(IAAS_URL, headers={'Authorization': 'Bearer {}'.format(token)}, verify=False)
    if r.status_code == 200:
        ops = json.loads(r.text)
        if ops.get('operations'):
            for op in ops.get('operations'):
                final.append({
                    'OP ID:': op.get('id'),
                    'Desc:': op.get('description'),
                    'Created At:': op.get('createdAt'),
                    'Created By:': op.get('createdBy')
                })

        return final

def getSubnets(data, token):
    CP_URL = 'https://iaas.private-api.cloud.yandex.net/compute/external/v1/subnets?folderId='
    r = requests.get('{}{}'.format(CP_URL, data), headers={'X-YaCloud-SubjectToken': '{}'.format(token)})
    response = json.loads(r.text)
    if response.get('subnets'):
        for subnet in response.get('subnets'):
            print('SubnetID: {} Name: {} NetID: {} CIDR Block: {} AZ: {}'.format(subnet.get('id'),
            subnet.get('name'),
            subnet.get('networkId'),
            subnet.get('v4CidrBlock'),
            subnet.get('zoneId')))

def getBucket(data, token):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem' # https://crls.yandex.net/allCAs.pem
    S3_URL = 'https://storage-idm.private-api.cloud.yandex.net:1443/stats/buckets/'
    r = requests.get('{}{}'.format(S3_URL, data))
    response = json.loads(r.text)
    if response.get('cloud_id'):
        return response
    else:
        return {'Code': r.status_code, 'Response': response, 'cloud_id': ''}

def resolveUsers(data, token):
    final = []
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem' # https://crls.yandex.net/allCAs.pem
    IAM_URL = 'http://identity.private-api.cloud.yandex.net:4336/v1/clouds/{}:listAccessBindings?pageSize=100&showSystemBindings=True'.format(data)
    response = json.loads(requests.get(IAM_URL, headers={'X-YaCloud-SubjectToken': '{}'.format(token)}).text)

    for user in response.get('accessBindings'):
        IAM_URL = 'http://identity.private-api.cloud.yandex.net:4336/v1/userAccounts/{}'.format(user.get('subject').get('id'))
        userData = json.loads(requests.get(IAM_URL, headers={'X-YaCloud-SubjectToken': '{}'.format(token)}).text)
        if userData.get('code'):
            pass
        else:
            final.append({'User:': userData.get('yandexPassportUserAccount').get('login'),
            'Email:': userData.get('yandexPassportUserAccount').get('defaultEmail'),
            'ID:': userData.get('id'),
            'Role:': user.get('roleId')})

    return final


def getBilling(data, token):
    '''Пишу это во время корпоратива, kill me'''
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem'
    BILLING_URL = 'https://billing.private-api.cloud.yandex.net:16465/billing/v1/private/support/resolve'
    r = requests.post(BILLING_URL,
        headers={'X-YaCloud-SubjectToken': '{}'.format(token)}, json=json.loads(data))# Это временно
    return json.loads(r.text)


def getIamToken(oauthToken):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem'
    IAM_URL = 'http://identity.private-api.cloud.yandex.net:4336/v1/tokens'
    r = requests.post(IAM_URL, json={"oauthToken": oauthToken})
    if args.debug:
        print('IAM: {}, Response: {}'.format(r.status_code, r.text))
    if r.status_code == 200:
        pass
    else:
        print('Something wrong happened during acquiring IAM token. API Response: {} '.format(r.status_code))
    data = json.loads(r.text)
    return data.get('iamToken')


def getClusters(data, token):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem'
    dbTypes = {'pg': 'managed-postgresql', 'mongo': 'managed-mongodb', 'ch': 'managed-clickhouse'}
    final = []
    for folder in allFolders(data):
        if folder:
            for dbtype, dbvalue in dbTypes.items():
                MDB_URL = 'https://mdb.api.cloud.yandex.net/{}/v1/clusters?folderId={}'.format(dbvalue, folder.get('Folder ID:'))
                r = requests.get(MDB_URL, headers={'Authorization': 'Bearer {}'.format(token)}, verify=False)
                if args.debug:
                    print('DEBUG Folder: {}'.format(folder.get('Folder ID:')))
                    print('DEBUG DB Type: {}'.format(dbvalue))
                    print('DEBUG MDB Status: {}'.format(r.status_code))
                    print('DEBUG MDB Response: {}'.format(r.text))
                if r.text:
                    responseData = json.loads(r.text)
                    for cluster in responseData.get('clusters', {}):
                        if dbtype == 'mongo':
                            result = {'Folder ID:': folder.get('Folder ID:'),
                            'Name:':cluster.get('name'),
                            'Type:': dbvalue,
                            'ID:':cluster.get('id'),
                            'Health:':cluster.get('health'),
                            'Status:':cluster.get('status'),
                            'Preset:':''}
                            if cluster.get('config').get('mongodb36'):
                                result['Preset:'] = cluster.get('config').get('mongodb36').get('mongod').get('resources').get('resourcePresetId')
                            else:
                                result['Preset:'] = cluster.get('config').get('mongodb40').get('mongod').get('resources').get('resourcePresetId')
                            final.append(result)
                        else:
                            result = {'Folder ID:': folder.get('Folder ID:'),
                            'Name:':cluster.get('name'),
                            'Type:': dbvalue,
                            'ID:':cluster.get('id'),
                            'Health:':cluster.get('health'),
                            'Status:':cluster.get('status'),
                            'Preset:':cluster.get('config', {}).get('resources', {}).get('resourcePresetId')}
                            final.append(result)
    return final


def getClustersOps(data, token, dbtype, clid):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem'
    OPS_URL = 'https://mdb.api.cloud.yandex.net/{}/v1/clusters/{}/operations'.format(dbtype, clid)
    r = requests.get(OPS_URL, headers={'Authorization': 'Bearer {}'.format(token)}, verify=False)
    response = json.loads(r.text)
    for op in response.get('operations'):
        print('OP: {} Done: {} Created At: {} Created By: {} ID: {}'.format(op.get('description'),
        op.get('done'), op.get('createdAt'), op.get('createdBy'), op.get('id')) )


def getClusterOps(data, token):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem'
    OPS_URL = 'https://operation.api.cloud.yandex.net/operations/{}'.format(data)
    r = requests.get(OPS_URL, headers={'Authorization': 'Bearer {}'.format(token)}, verify=False)
    response = json.loads(r.text)

    return response


def allFolders(data):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem' # https://crls.yandex.net/allCAs.pem
    IAM_URL = 'https://identity.private-api.cloud.yandex.net:14336/v1/allFolders?cloudId={}&pageSize=100'.format(data)
    r = json.loads(requests.get(IAM_URL).text)

    if r['result']:
        final = []
        for folder in r['result']:
            final.append({'Folder ID:':folder.get('id'),
            'Name:':folder.get('name'),
            'Status:':folder.get('status'),
            'Desc:':folder.get('description')})

        return final

    else:
        final = [{'Folder ID:':'',
            'Name:':'',
            'Status:':'',
            'Desc:':''}]
        return final


def allInstances(data, token):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem' # https://crls.yandex.net/allCAs.pem
    for folder in allFolders(data):
        fid = folder.get('Folder ID:')
        CP_URL = 'https://iaas.private-api.cloud.yandex.net/compute/external/v1/instances?folderId='
        r = requests.get('{}{}'.format(CP_URL, fid), headers={'X-YaCloud-SubjectToken': '{}'.format(token)})
        response = json.loads(r.text)

        if response.get('instances'):
            print('{}: {}'.format('Folder', fid))
            for instance in response.get('instances'):
                for response, value in getVM(instance.get('id'), token).items():
                    print(response, value)
                print('\n')
        else:
            print('No instances in folder {}\n'.format(fid))
        if args.debug:
            print('GET: {}'.format(CP_URL))
            print('Response status: {}'.format(r.status_code))
            print('Response body: {}'.format(r.text))


def getIp(data, token):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem' # https://crls.yandex.net/allCAs.pem
    CP_URL = 'https://iaas.private-api.cloud.yandex.net/compute/external/v1/admin/addresses?address={}'.format(data)
    r = requests.get('{}'.format(CP_URL), headers={'X-YaCloud-SubjectToken': '{}'.format(token)})
    response = json.loads(r.text)
    if args.debug:
        print('GET: {}'.format(CP_URL))
        print('Response: {} {}'.format(r.status_code ,r.text))
    if 'instance_id' in response:
        for response, value in getVM(response.get('instance_id'), token).items():
            print(response, value)
    else:
        print(response.get('message'))


def getOwner(data, token):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem' # https://crls.yandex.net/allCAs.pem
    IAM_URL = 'http://identity.private-api.cloud.yandex.net:4336/v1/clouds/{}'.format(data)
    cloud = json.loads(requests.get(IAM_URL, headers={'X-YaCloud-SubjectToken': '{}'.format(token)}).text)
    owner = json.loads(requests.get('{}{}'.format(IAM_URL,':getCreator')).text)

    if args.debug:
        print('GET: {}'.format(IAM_URL))
        print('Response body: {}'.format(cloud))

    return {'Cloud name:': cloud.get('name'),
    'Desc:': cloud.get('description'),
    'Status:': cloud.get('status'),
    'Created At:': cloud.get('createdAt'),
    'Owner e-mail:': owner.get('email'),
    'Owner name:': '{} {}'.format(owner.get('firstName'), owner.get('lastName')),
    'Owner Passport UID:': owner.get('passportUid'),
    'Owner phone:': owner.get('phone')}

    # if 'email' in r:
    #     return {'msg':data, 'data':'{} {}'.format(r.get('email'), r.get('passportUid'))}
    # else:
    #     return {'msg': data, 'data': 'Cloud not found, or no email in cloud'}


def getName(data, token):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem' # https://crls.yandex.net/allCAs.pem
    IAM_URL = 'https://identity.private-api.cloud.yandex.net:14336/v1/allClouds?name={}'.format(data)
    r = json.loads(requests.get(IAM_URL).text)

    if r.get('result'):
        cid = r.get('result')[0].get('id')
        print('Cloud ID:', cid)
        return getOwner(cid, token)
    else:
        return {'Response:':'Not Found'}


def getCloud(data):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem' # https://crls.yandex.net/allCAs.pem
    IAM_URL = 'https://identity.private-api.cloud.yandex.net:14336/v1/clouds:byLogin?'

    if args.role:
        IAM_URL = '{}login={}&role={}'.format(IAM_URL, data,
            roles.get(args.role))
        exitMsg = [{'Response: ': 'No clouds found for login {}, or {} does not have role "{}" in any cloud'.format(data,
        data,
        args.role)}]
    else:
        IAM_URL = '{}login={}'.format(IAM_URL, data)
        exitMsg = [{'Response: ': 'No clouds found for login {}'.format(data)}]

    r = requests.get(IAM_URL)
    response = json.loads(r.text)

    if args.debug:
        print('GET: {}'.format(IAM_URL))
        print('Response status: {}'.format(r.status_code))
        print('Response body: {}'.format(r.text))

    if response.get('clouds'):
        final = []

        for cloud in response['clouds']:
            final.append({'Cloud ID:': cloud.get('id'),
                'Name:': cloud.get('name'),
                'Status:': cloud.get('status'),
                'Desc:': cloud.get('description'),
                'Created at:': cloud.get('createdAt')})
        return final

    else:
        return exitMsg


def getFolder(data):
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem' # https://crls.yandex.net/allCAs.pem
    IAM_URL = 'https://identity.private-api.cloud.yandex.net:14336/v1/allFolders?id={}'.format(data)
    folder = json.loads(requests.get(IAM_URL).text)

    if folder['result']:
        final = []
        final.append('Cloud ID: {} Name: {} Status: {} Created At: {}'.format(folder['result'][0].get('cloudId'),
            folder['result'][0].get('name'),
            folder['result'][0].get('status'),
            folder['result'][0].get('createdAt')))

        return final

    else:
        return ['No folder with id {} found or something terribly wrong happened. Contact uplink@'.format(data)]


def getVM(vmid, token):
    VM_URL = 'https://iaas.private-api.cloud.yandex.net/compute/private/v1/instances/'
    os.environ['REQUESTS_CA_BUNDLE'] = 'allCAs.pem' # https://crls.yandex.net/allCAs.pem
    vmurl = '{}{}'.format(VM_URL, vmid)
    data = json.loads(requests.get(vmurl).text)

    if args.debug:
        print('GET: {}'.format(vmurl))
        print('Response body: {}'.format(data))

    if data.get('code'):
        return {'API Says: ': data.get('code')}
    else:
        resources = data.get('resources')

        if args.metadata:
            metadata = '\n{}'.format(data.get('metadata'))
        else:
            metadata = 'Metadata display disabled. Use --metadata or -m to enable.'

        if data.get('status') == 'running':
            network = data.get('interface_attachments')[0]['data']
            preemptibleState = data.get('allocation', {}).get('preemptible')
        else:
            preemptibleState = data.get('scheduling_policy', {}).get('preemptible')
            network = {}


        return {'Instance ID:': vmid,
        'Status:': data.get('status'),
        'Folder ID:': data.get('folder_id'),
        'Cloud ID:': data.get('cloud_id'),
        'Zone:': data.get('zone_id'),
        'Compute node:': '{} {}'.format(data.get('compute_node'), data.get('graphics_port')),
        'Core type:': data.get('allocation', {}).get('core_type'),
        'Preemptible:': preemptibleState,
        'Decription:': data.get('description'),
        'Hostname:': data.get('hostname'),
        'Internal IP:': network.get('ip_address'),
        'Public IP:': network.get('public_ip_address'),
        'Cores:': resources.get('cores'),
        'Memory:': '{}G'.format(resources.get('memory')>>30),
        'Name:': data.get('name'),
        'TAP Interface:': 'tap{}-0'.format(vmid[3:12]),
        'Usage:': '{}{}'.format('https://backoffice.cloud.yandex.ru/compute/',vmid),
        'Metadata:': metadata}

if args.cloud and args.login:
    print('Please use CloudID OR Login, not both. Use --help for more details.')
elif args.instance:
    for response, value in getVM(args.instance, getIamToken(oauthToken)).items():
        print(response, value)
    if args.operations:
        if getOps(args.instance, getIamToken(oauthToken)):
            print('Trying to resolve actions list:')
            for op in getOps(args.instance, getIamToken(oauthToken)):
                print('OP ID: {} Desc: {} Created at: {} Created by: {}'.format(op.get('OP ID:'),
                op.get('Desc:'),
                op.get('Created At:'),
                op.get('Created By:')))
elif args.cloud:
    for k,v in getOwner(args.cloud, getIamToken(oauthToken)).items():
        print(k, v)
    print('\n')
    if args.allfolders:
        for folder in allFolders(args.cloud):
            print('Folder ID: {} Name: {} Status: {} Desc: {}'.format(folder.get('Folder ID:'),
            folder.get('Name:'),
            folder.get('Status:'),
            folder.get('Desc:')))
    if args.billing:
        data = '{{"cloud_id":"{}"}}'.format(args.cloud)
        for key, value in getBilling(data, getIamToken(oauthToken)).items():
            print('{}: {}'.format(key, value))
        print('\n')
    if args.users:
        for user in resolveUsers(args.cloud, getIamToken(oauthToken)):
            for key, value in user.items():
                print(key, value)
            print('\n')
    if args.network:
        for folder in allFolders(args.cloud):
            fid = folder.get('Folder ID:')
            getSubnets(fid, getIamToken(oauthToken))
elif args.clusters:
    print('Cloud: {}'.format(args.clusters))
    for response in getClusters(args.clusters, getIamToken(oauthToken)):
        print(''.join(['{0} {1} '.format(k, v) for k,v in response.items()]))
        if args.get_cluster_ops:
            print('Operations:')
            getClustersOps(args.clusters, getIamToken(oauthToken), response.get('Type:'), response.get('ID:'))
        print('\n')
    print('\n')
elif args.get_billing:
    data = '{{"billing_account_id":"{}"}}'.format(args.get_billing)
    for key, value in getBilling(data, getIamToken(oauthToken)).items():
        print('{}: {}'.format(key, value))
    print('\n')
elif args.folder:
    for response in getFolder(args.folder):
        print(response)
elif args.bucket:
    for k,v in getOwner(getBucket(args.bucket, getIamToken(oauthToken)).get('cloud_id', {}), getIamToken(oauthToken)).items():
        print(k, v)
    print('\n')
    for k,v in getBucket(args.bucket, getIamToken(oauthToken)).items():
        print('{}: {}'.format(k, v))
elif args.login:
    for response in getCloud(args.login):
        for key, value in response.items():
            print(key, value)
        if args.allfolders:
            for folder in allFolders(response.get('Cloud ID:')):
                print('Folder ID: {} Name: {} Status: {} Desc: {}'.format(folder.get('Folder ID:'),
                folder.get('Name:'),
                folder.get('Status:'),
                folder.get('Desc:')))
        if args.billing:
            data = '{{"passport_login":"{}"}}'.format(args.login)
            for key, value in getBilling(data, getIamToken(oauthToken)).items():
                print('{}: {}'.format(key, value))
        print('\n')
elif args.rolelist:
    for role in roles:
        print(role)
elif args.get_cl_op:
    for k,v in getClusterOps(args.get_cl_op, getIamToken(oauthToken)).items():
        print('{}: {}'.format(k, v))
elif args.ip:
    getIp(args.ip, getIamToken(oauthToken))
elif args.name:
    for k,v in getName(args.name, getIamToken(oauthToken)).items():
        print(k,v)
elif args.all_instances:
    allInstances(args.all_instances, getIamToken(oauthToken))
else:
    print('Input Error. Use --help for more details.')

