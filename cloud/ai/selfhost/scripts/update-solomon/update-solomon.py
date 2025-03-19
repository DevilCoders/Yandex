#!/usr/bin/env python3
import os
import argparse
import requests
import sys
import jwt
import time

prod_cluster = {
    'name': 'cloud_ai_prod',
    'ig_ids': [
        'cl1mfk6346pnfqhvhdc1',  # stt-demosthenes
        'cl1s9srvpku4r7jrld3e',  # stt-kazakh
        'cl192vg51u4j90mrapg2',  # stt-biovitrum
        'cl16eltt434uj6blilmu',  # stt-kaspi
        'cl1dci1t88mqscfast6o',  # stt-fio
        'cl1o20jg7mi1gcsr5ht5',  # stt-rupor
        'cl1cr40ug2k8ttanhs50',  # stt-general-v3
        'cl1b2e3dqthsgs7m62ou',  #stt-general-rc-v3

        'cl1dr7u0mbhft2ub6g5k',  # tts-oksana
        'cl1r58a4o98hvd498ukn',  # tts-valtz
        'cl1ls89rr8c4o4djvb4c',  # tts-oksana-rc
        'cl1v9qnrkthf2ns1r343',  # tts-sergey-prod

        'cl1kas2j7couoglqbim1',  # services-proxy-stt-green
        'cl1l1hfpgs67pecth02s',  # services-proxy-stt-blue
        'cl1k486ac51r7a17vlvf',  # services-proxy-stt-rest
        'cl13fq4n7ehunb8q2hvq',  # services-proxy-tts-rest
        'cl1767q0556ednsa2ckv',  # services-proxy-tts-rest-canary
        'cl1m35li98tb94duh9f5',  # services-proxy-translate
        'cl1qjls0g46fjn1s7kvv',  # services-proxy-translate-rest
        'cl1fh3hqo51i23487q5e',  # services-proxy-vision
        'cl1no3q418vcadlhg9at',  # services-proxy-locator
        'cl19sfpq4kdiuonvbbcp',  # ai-services-proxy-tts-prod

        'cl1fmsc1b0jl1f3vr649',  # operations-queue

        'cl1k21pi35c61bbe97b6',  # stt-feedback
    ],
}

preprod_cluster = {
    'name': 'cloud_ai_test',
    'ig_ids': [
        'cl136k54g0n5smkfee13',  # stt-preprod
        'cl16keaj2eq8ohqokhrd',  # stt-preprod-cpu

        'cl1cm7vbiipv7u812plu',  # ai-tts-service-ycp-preprod

        'cl1ab9ctrptf7l34gemk',  # services-proxy-stt-preprod
        'cl1h49h60jtiuk1uo67r',  # services-proxy-stt-rest-preprod
        'cl1enlalc7imjdpl6sj5',  # services-proxy-tts-rest-preprod
        'cl1gqbmbs17bqr00p0bp',  # services-proxy-translate-preprod
        'cl1sec664ncbp5jgt7s5',  # services-proxy-translate-rest-preprod
        'cl13fhof2lel9h0m1hgv',  # services-proxy-vision-preprod
        'cl1g60e9gvihoipls3h0',  # services-proxy-locator-preprod
        'cl1misohrnro1ccaqblb',  # ai-services-proxy-tts-preprod

        'cl1ejme58gqdllajjiuk',  # operations-queue-preprod

        'cl1rmh4r3kuutn9dvvjl',  # stt-feedback-preprod
    ],
}

clusters = [
    preprod_cluster,
    prod_cluster,
]

# APP page https://oauth.yandex-team.ru/client/e9d4b6d697b2481d9d47cc404039d89b
# https://oauth.yandex-team.ru/authorize?response_type=token&client_id=e9d4b6d697b2481d9d47cc404039d89b
releaser_token = os.environ['YC_AI_RELEASER_TOKEN']
cloud_token = os.environ.get('YC_OAUTH_TOKEN', None) or os.environ['YC_TOKEN']

solomon_url = "https://solomon.cloud.yandex-team.ru/api/v2/projects/cloud_ai/clusters"
solomon_headers = {
    'Authorization': 'OAuth ' + releaser_token,
    'Content-Type': 'application/json',
    'Accept': 'application/json',
}

COLOR_GREEN = '\033[92m'
COLOR_RED = '\033[93m'
COLOR_RESET = '\033[0m'


def get_jwt(jwt_private_key):
    service_account_id = "ajeomtglndk13mrpq8eo"
    key_id = "ajeugi25fl06p6lpt5r6"

    with open(jwt_private_key, 'r') as private:
        private_key = private.read()
    now = int(time.time())
    payload = {
        'aud': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
        'iss': service_account_id,
        'iat': now,
        'exp': now + 360}

    encoded_token = jwt.encode(
        payload,
        private_key,
        algorithm='PS256',
        headers={'kid': key_id})
    token = encoded_token.decode('utf8').replace("'", '"')

    return token


def get_http_client():
    retry_strategy = requests.packages.urllib3.util.retry.Retry(
        total=3,
        status_forcelist=[429, 500, 502, 503, 504],
        method_whitelist=['HEAD', 'GET', 'OPTIONS']
    )
    adapter = requests.adapters.HTTPAdapter(max_retries=retry_strategy)
    http = requests.Session()
    http.mount('https://', adapter)
    http.mount('http://', adapter)
    return http


http = get_http_client()  # enable retries


def get_cluster_conf(cluster_name):
    r = http.get(solomon_url + '/{cluster_name}'.format(cluster_name=cluster_name), headers=solomon_headers,
                 verify=False)
    return r.json()


def get_conf_ips(conf):
    ips = []
    for network in conf.get('networks', []):
        ips.append(network['network'])
    return ips


def get_iam_token(jwt_private_key):
    if jwt_private_key != "":
        params = {'jwt': get_jwt(jwt_private_key)}
    else:
        params = {'yandexPassportOauthToken': cloud_token}
    response = http.post('https://iam.api.cloud.yandex.net/iam/v1/tokens', json=params)
    return response.json()['iamToken']


def get_ig_ips(ig_ids, iam_token):
    ips = []
    cloud_headers = {
        'Authorization': 'Bearer ' + iam_token,
    }
    for ig_id in ig_ids:
        r = http.get(
            'https://compute.api.cloud.yandex.net/compute/v1/instanceGroups/{instance_group_id}/instances'.format(
                instance_group_id=ig_id), headers=cloud_headers)
        resp = r.json()
        if 'instances' not in resp:
            print('WARNING: No instances in ' + ig_id, 'response: ' + str(resp))
            continue  # no ig found or ig does not have instances

        for instance in resp['instances']:
            ips.append(instance['networkInterfaces'][0]['primaryV6Address']['address'])
    return ips


def show_diff(old, new):
    old_set = set(old)
    new_set = set(new)
    additions = new_set - old_set
    deletions = old_set - new_set

    print('additions:', len(additions), 'deletions:', len(deletions))
    for elem in deletions:
        print(COLOR_RED + '  -', elem)
    for elem in additions:
        print(COLOR_GREEN + '  +', elem)
    print(COLOR_RESET)
    return additions, deletions


def get_updated_conf(conf, new_ips):
    networks = []
    for ip in new_ips:
        networks.append(
            {
                'network': ip,
                'port': 8004,
                'labels': [],
            }
        )
    conf['networks'] = networks
    return conf


def apply_cluster_conf(cluster_name, conf):
    r = http.put(solomon_url + '/{cluster_name}'.format(cluster_name=cluster_name),
                 headers=solomon_headers,
                 verify=False,
                 json=conf
                 )
    print(r.status_code, r.text)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--yes', default=False, action='store_true')
    parser.add_argument('--jwt-private-key', default="")
    args = parser.parse_args()

    cluster_updates = {}
    for cluster in clusters:
        print('Prepare cluster update:', cluster['name'])
        conf = get_cluster_conf(cluster['name'])

        old_ips = get_conf_ips(conf)
        new_ips = get_ig_ips(cluster['ig_ids'], get_iam_token(args.jwt_private_key))

        (additions, deletions) = show_diff(old_ips, new_ips)
        if len(additions) == 0 and len(deletions) == 0:
            print('No updates for', cluster['name'], '\n')
            continue

        new_conf = get_updated_conf(conf, new_ips)
        cluster_updates[cluster['name']] = new_conf

    if len(cluster_updates) == 0:
        print('Nothing to update.')
        return

    if not args.yes:
        input('Sure to continue? Press Enter to continue...')

    for cluster_name in cluster_updates:
        apply_cluster_conf(cluster_name, cluster_updates[cluster_name])


if __name__ == '__main__':
    main()
