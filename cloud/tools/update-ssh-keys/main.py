import json
import uuid

import jwt
import os
import requests

import cProfile

from datetime import datetime, timedelta

VERSION = '1.1'
USER_AGENT = 'ssh-keys-updater/{} (https://nda.ya.ru/3VnVDx)'.format(VERSION)

# should be in ENV
TOKEN_CLOUD = os.getenv('TOKEN_CLOUD')
SA_KEY = os.getenv('SA_KEY')
TOKEN_YANDEX = os.getenv('TOKEN_YANDEX')
# colon-separated list
ABC_SERVICE = os.getenv('ABC_SERVICE')
CLOUD_ID = os.getenv('CLOUD_ID')
CLOUD_HOST = os.getenv('CLOUD_HOST')
FOLDER_ID = os.getenv('FOLDER_ID')
PROFILE = os.getenv('PROFILE', 'false')
DEBUG = os.getenv('DEBUG', 'false')

ABC_URL = 'https://abc-back.yandex-team.ru/api/v3/services/members/?service__slug={}'
STAFF_URL = 'https://staff-api.yandex-team.ru/v3/persons?_one=1&login={}&_pretty=1'
YAPI_HEADERS = {
    'Authorization': 'OAuth {}'.format(TOKEN_YANDEX),
    'User-Agent': USER_AGENT
}
CLOUD_URL = 'https://{}.api.{}.yandex.net/{}'
PAGE_SIZE = '100'


def main():
    if DEBUG.lower() != 'false':
        debug = True
    else:
        debug = False

    if PROFILE.lower() != 'false':
        profile = True
    else:
        profile = False

    if profile:
        print 'Profiling ENABLED'
        pr = cProfile.Profile()
        pr.enable()

    g = globals()
    for i in ['TOKEN_YANDEX', 'ABC_SERVICE', 'CLOUD_HOST']:
        if i not in g:
            raise Exception('No global var {} defined'.format(i))
        if not g[i]:
            raise Exception('No env var {} provided'.format(i))

    # strict one of 'CLOUD_ID' or 'FOLDER_ID' defined to proceed
    if not (bool(g['CLOUD_ID']) ^ bool(g['FOLDER_ID'])):
        raise Exception('Specify strict one of {} or {}'.format('CLOUD_ID', 'FOLDER_ID'))

    # strict one of 'TOKEN_CLOUD' or 'SA_KEY' defined to proceed
    if not (bool(g['TOKEN_CLOUD']) ^ bool(g['SA_KEY'])):
        raise Exception('Specify strict one of {} or {}'.format('TOKEN_CLOUD', 'SA_KEY'))

    if FOLDER_ID:
        print 'Syncing ssh public keys for ABC service {} to folder with ID {} at api.{}.yandex.net'.format(ABC_SERVICE,
                                                                                                            FOLDER_ID,
                                                                                                            CLOUD_HOST)
    elif CLOUD_ID:
        print 'Syncing ssh public keys for ABC service {} to cloud with ID {} at api.{}.yandex.net'.format(ABC_SERVICE,
                                                                                                           CLOUD_ID,
                                                                                                           CLOUD_HOST)
    print ''
    keys_by_user = {}
    for abc_service in ABC_SERVICE.split(':'):
        print '==> Fetching ABC members for "{}" service'.format(abc_service)
        service_logins = get_abc_service_members(abc_service)
        print '====> Retrieved {} members for ABC service slug "{}"'.format(len(service_logins), abc_service)
        for login in service_logins:
            if login not in keys_by_user:
                keys_by_user[login] = []
            print '====> Fetching ssh keys for {}'.format(login)

            resp = requests.get(STAFF_URL.format(login), headers=YAPI_HEADERS)
            try:
                resp.raise_for_status()
            except:
                print resp.text
                raise

            keys = resp.json()['keys']
            print '======> Got {} keys for {} person'.format(len(keys), login)
            for k in keys:
                key = k['key']
                keys_by_user[login].append(key)
    print '==> Got {} ssh public keys for {} users'.format(
        len([user_key for user_keys in keys_by_user.values() for user_key in user_keys]), len(keys_by_user))
    print ''

    keys_metadata = []
    for login in sorted(keys_by_user):
        for key in sorted(keys_by_user[login]):
            keys_metadata.append("{}:{}".format(login, key))
    keys_metadata = "\n".join(keys_metadata)

    print '==> Fetching IAM token'
    token = get_iam_token()
    print ''

    folders = {}
    if FOLDER_ID:
        print 'Fetching folder info for folder "{}"'.format(FOLDER_ID)
        resp = cloud_api('resource-manager', 'get',
                         'resource-manager/v1/folders/' + FOLDER_ID, token=token)
        folders[resp['id']] = resp['name']
    elif CLOUD_ID:
        print '==> Fetching folder list in cloud "{}"'.format(CLOUD_ID)
        resp = cloud_api('resource-manager', 'get',
                         'resource-manager/v1/folders?cloudId=' + CLOUD_ID + '&pageSize=' + PAGE_SIZE, token=token)
        for i in resp['folders']:
            folders[i['id']] = i['name']
    print '==> Got {} folders to proceed in cloud'.format(len(folders))
    print ''

    ops = set()
    for folder_id in folders:
        print '====> Updating folder {} ({})'.format(folders[folder_id], folder_id)
        resp = cloud_api('compute', 'get', 'compute/v1/instances?pageSize=1000&folderId=' + folder_id, token=token)
        if 'instances' not in resp:
            print '======> Empty response from compute, skipping'
            continue
        for vm in resp['instances']:
            resp = cloud_api('compute', 'get', 'compute/v1/instances/' + vm['id'] + '?view=FULL', token=token,
                             debug=debug)
            opid = update_vm_keys(resp, keys_metadata, token, debug=debug)
            if opid:
                ops.add(opid)
        print ''

    errors = []
    while len(ops) > 0:
        print '====> {} operations still running'.format(len(ops))
        for opid in set(ops):
            resp = cloud_api('operation', 'get', 'operations/' + opid, token=token)
            if 'error' in resp:
                errors.append('{} failed: {}'.format(opid, resp['error'].message))
                ops.remove(opid)
            if 'done' in resp:
                ops.remove(opid)

    print ''
    print ''
    if errors:
        print '===== {} operations failed ====='.format(len(errors))
        for err in errors:
            print ''
            print '  * {}'.format(err)
        print ''
        raise Exception()
    else:
        print '===== All required keys are updated ====='
        print ''

    if profile:
        pr.disable()
        print 'Profiling STATS'
        pr.print_stats()


def get_abc_service_members(service_slug):
    def abc_api(url):
        resp = requests.get(url, headers=YAPI_HEADERS)
        resp.raise_for_status()
        return resp

    def members_from_response(resp):
        members = []
        for item in resp.json()['results']:
            members.append(item['person']['login'])
        return members

    service_logins = []
    init_abc_url = ABC_URL.format(service_slug)
    resp = abc_api(init_abc_url)
    service_logins.extend(members_from_response(resp))
    while resp.json()['next']:
        resp = abc_api(resp.json()['next'])
        service_logins.extend(members_from_response(resp))
    return sorted(set(service_logins))


def update_vm_keys(vm, ssh_keys, token, debug=False):
    name = vm['name'] if 'name' in vm and vm['name'] else 'unnamed'
    fqdn = vm['fqdn'] if 'fqdn' in vm and vm['fqdn'] else 'unnamed'
    name = '{} ({})'.format(name, fqdn)

    if 'labels' in vm and 'skip_update_ssh_keys' in vm['labels'] and \
            (vm['labels']['skip_update_ssh_keys'].lower() == 'true' or
             vm['labels']['skip_update_ssh_keys'].lower() == '1'):
        print '======> Skipping {} - label "skip_update_ssh_keys" set to true'.format(name)
        return None

    old_metadata = {}
    if 'metadata' in vm:
        old_metadata = vm['metadata']

    if 'ssh-keys' in old_metadata and old_metadata['ssh-keys'] == ssh_keys:
        print '======> Skipping {} - keys are up-to-date'.format(name)
        return None

    print '======> Updating keys on {}'.format(name)
    req = {'upsert': {'ssh-keys': ssh_keys}}
    resp = cloud_api('compute', 'post', 'compute/v1/instances/' + vm['id'] + "/updateMetadata", data=req, token=token,
                     debug=debug)
    return resp['id']


def get_cloud_url(service, path):
    return CLOUD_URL.format(service, CLOUD_HOST, path)


def cloud_api(service, method, path, data=None, token=None, debug=False):
    if not token:
        token = get_iam_token()
    fn = getattr(requests, method)
    headers = {
        'Authorization': 'Bearer ' + token,
        'User-Agent': USER_AGENT
    }
    resp = fn(get_cloud_url(service, path), json=data, headers=headers)
    try:
        resp.raise_for_status()
    except:
        print resp.text
        raise
    if debug:
        print 'DEBUG: request-id:{} service:{} method:{} path:{} '.format(resp.headers.get('x-request-id'), service,
                                                                          method, path)
    return resp.json()


def get_iam_token():
    request_id = str(uuid.uuid4())
    http_headers = {
        "Content-Type": "application/json",
        "X-Client-Request-ID": request_id,
        "User-Agent": USER_AGENT
    }

    if TOKEN_CLOUD:
        print '====> Using OAuth token'
        resp = requests.post(get_cloud_url('iam', 'iam/v1/tokens'), json={'yandexPassportOauthToken': TOKEN_CLOUD},
                             headers=http_headers)
        resp.raise_for_status()
        return resp.json()['iamToken']

    if SA_KEY:
        print '====> Using Service Account private key'
        sa_key = json.loads(SA_KEY)
        unixtime_now = datetime.utcnow()
        token_payload = {
            'iss': sa_key['service_account_id'],
            'aud': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
            'iat': unixtime_now,
            'exp': unixtime_now + timedelta(hours=1),
        }
        token_headers = {
            'typ': 'JWT',
            'alg': 'PS256',
            'kid': sa_key['id']}
        j = jwt.encode(token_payload, sa_key['private_key'], algorithm='PS256', headers=token_headers)

        resp = requests.post(get_cloud_url('iam', 'iam/v1/tokens'), json={'jwt': j}, headers=http_headers)
        resp.raise_for_status()
        return resp.json()['iamToken']


if __name__ == '__main__':
    import httplib as http_client

    http_client.HTTPConnection.debuglevel = 1
    main()
