import json
import requests
import sys


def main():
    input = json.load(sys.stdin)
    oauth = input['yt_oauth']
    id = input['id']
    value_name = input['value_name']
    SECRET_VERSIONS_URL = 'https://vault-api.passport.yandex.net/1/secrets/{}'
    SECRET_VERSION_URL = 'https://vault-api.passport.yandex.net/1/versions/{}'
    OAUTH_HEADERS = {'Authorization': 'OAuth {}'.format(oauth)}

    resp = requests.get(SECRET_VERSIONS_URL.format(id), headers=OAUTH_HEADERS, verify=False)
    version = resp.json()['secret']['secret_versions'][0]['version']
    resp = requests.get(SECRET_VERSION_URL.format(version), headers=OAUTH_HEADERS, verify=False)
    output = {}
    output['id'] = id
    output['version'] = version
    for value in resp.json()['version']['value']:
        if value_name == value['key']:
            output['secret'] = value['value']
            break

    print(json.dumps(output))


if __name__ == '__main__':
    import http.client as http_client
    from urllib3.exceptions import InsecureRequestWarning

    http_client.HTTPConnection.debuglevel = 0
    requests.packages.urllib3.disable_warnings(category=InsecureRequestWarning)
    main()
