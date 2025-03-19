import requests
import sys

import library.python.oauth as lpo


def fetch_token_info(token):
    r = requests.get('https://api-staff.n.yandex-team.ru/resolve_user', headers={'Authorization': 'OAuth ' + token})

    r.raise_for_status()

    return r.json()


if __name__ == '__main__':
    # NOTE: this is special oauth.yandex-team.ru application registered under robot-yc-billing-sdk.
    client_id = '2a59e7fe723948d6af34f8957e8d33f3'
    client_secret = 'c4b05e11c2d84c24a2ccf24323a53eba'
    token = lpo.get_token(client_id, client_secret, keys=sys.argv[1:], raise_errors=True)

    # NOTE: staff api is not so stable, uncomment for debug info if needed.
    # data = fetch_token_info(token)
    print(token)
