#!/skynet/python/bin/python
import os
import sys
import logging
import argparse

import requests
import pymongo

from mongo_params import ALL_HEARTBEAT_C_MONGODB

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import config


def group_person_map_collection():
    return pymongo.MongoReplicaSetClient(
        ALL_HEARTBEAT_C_MONGODB.uri,
        connectTimeoutMS=5000,
        replicaSet=ALL_HEARTBEAT_C_MONGODB.replicaset,
        w='3',
        wtimeout=60000,
        read_preference=ALL_HEARTBEAT_C_MONGODB.read_preference,
    )['staff_cache']['group_persons_map']


GROUP_MEMBERSHIP_URL = 'http://staff-api.yandex-team.ru/v3/groupmembership'


def get_staff_request_json(url, oauth_token, params=None):
    return requests.get(
        url,
        headers={'Authorization': 'OAuth {}'.format(oauth_token)},
        params=params,
    ).json()


def get_group_to_persons_mapping(oauth_token):
    mappings = []
    response = get_staff_request_json(GROUP_MEMBERSHIP_URL, oauth_token, params={
        '_limit': 10000,
        '_query': 'group.type=="department" and person.is_deleted == false',
        '_fields': 'group.url,person.login',
    })
    while response['links'].get('next'):
        mappings += response['result']
        response = get_staff_request_json(response['links']['next'], oauth_token)
    mappings += response['result']
    result = {}
    for mapping in mappings:
        if mapping['group']['url'] not in result:
            result[mapping['group']['url']] = []
        result[mapping['group']['url']].append(mapping['person']['login'])
    return result


def populate(oauth_token, dry_run=False):
    if dry_run:
        import json
        print json.dumps(get_group_to_persons_mapping(oauth_token))
    else:
        bulk = group_person_map_collection().initialize_unordered_bulk_op()
        for group, persons in get_group_to_persons_mapping(oauth_token).items():
            bulk.find({'group': group}).upsert().replace_one({
                'group': group,
                'persons': persons,
            })
        bulk.execute({'w': 2, 'timeout': 10})


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--nomongo', action='store_true')
    parser.add_argument('--oauth-token', type=str, required=True,
                        help='Obligatory. Oauth token to request staff')
    return parser.parse_args()


def main():
    logging.basicConfig(level=logging.DEBUG)
    arguments = parse_args()
    try:
        populate(arguments.oauth_token, arguments.nomongo)
    except Exception as ex:
        logging.exception(ex)


if __name__ == '__main__':
    main()

