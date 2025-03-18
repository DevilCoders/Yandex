import json
import requests
import hashlib
import sys

import library.python.oauth as lpo


def fetch_token_info(token):
    r = requests.get('https://api-staff.n.yandex-team.ru/resolve_user', headers={'Authorization': 'OAuth ' + token})

    r.raise_for_status()

    return r.json()


if __name__ == '__main__':
    client_id = '23b4f83306e3469abdee07054d307e7c'
    client_secret = '87dcc81340254b12a4cecdfe34c6d387'
    token = lpo.get_token(client_id, client_secret, keys=sys.argv[1:])
    data = fetch_token_info(token)

    print('md5(token) = ' + hashlib.md5(token.encode('ascii')).hexdigest() + ', login = ' + data['login'] + ', ' + 'scope = ' + data['oauth']['scope'])
