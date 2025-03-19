#!/usr/bin/env python3

import requests
import sys
import json

ABC_URL = 'https://abc-back.yandex-team.ru/api/v4/services/members/?page_size=1000&fields=role,person&service__slug={}'
STAFF_URL = 'https://staff-api.yandex-team.ru/v3/persons?login={}&_pretty=1&_fields=login,keys,id&_sort=login'

def main():
    args = json.load(sys.stdin)
    yandex_token = args['yandex_token']
    abc_service = args['abc_service']
    abc_service_scopes = list(filter(None, args['abc_service_scopes'].split(",")))
    abc_request_url = ABC_URL.format(abc_service)

    if len(abc_service_scopes) > 0:
        abc_request_url += '&role__scope__slug__in={}'.format(",".join(abc_service_scopes))

    resp = requests.get(abc_request_url, headers=auth_header(yandex_token))
    resp.raise_for_status()
    keys_by_user = {}
    all_persons = get_abc_service_members(abc_service, abc_service_scopes, yandex_token)

    resp = requests.get(STAFF_URL.format(','.join(all_persons)), headers=auth_header(yandex_token))
    resp.raise_for_status()
    for result in resp.json()['result']:
        login = result['login']
        if login not in keys_by_user:
            keys_by_user[login] = []
        keys = result['keys']
        for k in keys:
            key = k['key']
            keys_by_user[login].append(key)

    keys_metadata = []
    for login in sorted(keys_by_user):
        for key in sorted(keys_by_user[login]):
            keys_metadata.append("{}:{}".format(login, key))
    keys_metadata = "\n".join(keys_metadata)
    output = {}
    output['logins'] = ','.join(keys_by_user.keys())
    output['logins_count'] = str(len(keys_by_user.keys()))
    output['keys_count'] = str(len([user_key for user_keys in keys_by_user.values() for user_key in user_keys]))
    output['ssh-keys'] = keys_metadata
    output['abc-service'] = abc_service
    print(json.dumps(output))


def auth_header(token):
    return {'Authorization': 'OAuth {}'.format(token)}


def get_abc_service_members(service_slug, scope_slugs, yandex_token):
    def abc_api(url):
        resp = requests.get(url, headers=auth_header(yandex_token))
        resp.raise_for_status()
        return resp

    scope_filter = lambda result: True
    if scope_slugs:
        scope_filter = lambda result: result['role']['scope']['slug'] in scope_slugs

    def members_from_response(resp):
        members = []
        for item in resp.json()['results']:
            if not scope_filter(item):
                continue
            members.append(item['person']['login'])
        return members

    init_abc_url = ABC_URL.format(service_slug)
    resp = abc_api(init_abc_url)
    service_logins = members_from_response(resp)
    while resp.json()['next']:
        resp = abc_api(resp.json()['next'])
        service_logins.extend(members_from_response(resp))

    return sorted(set(service_logins))


if __name__ == '__main__':
    import http.client

    http.client.HTTPConnection.debuglevel = 0
    main()

