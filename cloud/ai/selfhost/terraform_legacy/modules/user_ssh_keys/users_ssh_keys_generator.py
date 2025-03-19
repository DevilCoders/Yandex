import requests
import sys
import json
import collections

input = json.load(sys.stdin)
SHH_KEYS_STAFF_URL = 'https://staff-api.yandex-team.ru/v3/persons?groups.group.type=service&groups.group.url=svc_{}&_fields=login,keys.key&_limit=500'
YANDEX_TOKEN = input['yandex_token']
ABC_SERVICE = input['abc_service']
YAPI_HEADERS = {'Authorization': 'OAuth {}'.format(YANDEX_TOKEN)}

USER_TEMPLATE = '''
  - name: {}
    sudo: ['ALL=(ALL) NOPASSWD:ALL']
    groups: sudo
    shell: /bin/bash
    ssh-authorized-keys:''' # Leading \n is important here

def main():
    resp = requests.get(SHH_KEYS_STAFF_URL.format(ABC_SERVICE), headers=YAPI_HEADERS)
    resp.raise_for_status()
    # print resp.content
    # exit(0)
    keys_by_user = collections.defaultdict(list)
    for result in resp.json()['result']:
        login = result['login']
        keys = result['keys']
        for k in keys:
            keys_by_user[login].append(k['key'])

    users = []
    keys_metadata = []
    for login in sorted(keys_by_user):
        user = USER_TEMPLATE.format(login)
        for key in sorted(keys_by_user[login]):
            keys_metadata.append("{}:{}".format(login, key))
            user += "\n      - {}".format(key)
        if len(keys_by_user[login]) != 0:
            users.append(user)
    keys_metadata = "\n".join(keys_metadata)
    users = "".join(users)
    output = {}
    output['logins'] = ','.join(keys_by_user.keys())
    output['logins_count'] = str(len(keys_by_user.keys()))
    output['keys_count'] = str(len([user_key for user_keys in keys_by_user.values() for user_key in user_keys]))
    output['ssh_keys'] = keys_metadata
    output['users'] = users
    output['abc_service'] = ABC_SERVICE
    print(json.dumps(output))


if __name__ == '__main__':
    import http.client as http_client

    http_client.HTTPConnection.debuglevel = 0
    main()
