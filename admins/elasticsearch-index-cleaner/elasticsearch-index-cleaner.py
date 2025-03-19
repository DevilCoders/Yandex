#!/usr/bin/env python3
import requests
import datetime
import argparse
import json


def parse_args():
    """
    Parse command line arguments
    :return:
    """
    p = argparse.ArgumentParser()
    p.add_argument('--index', help='index name prefix', required=True)
    p.add_argument('--ttl-mode', choices=['minutes', 'hours', 'days'], required=True, help='ttl mode')
    p.add_argument('--ttl-value', type=int, help='ttl value', required=True)
    p.add_argument('--host', default='127.0.0.1', help='elasticsearch host')
    p.add_argument('--port', type=int, default=9200, help='elasticsearch port')
    p.add_argument('--dry', action='store_true')
    return p.parse_args()


def get_indices():
    """
    Get all indices names from elasticsearch instance
    :return:
    """
    global args
    r = requests.get('http://{}:{}/_cat/indices'.format(args.host, args.port))
    for line in r.text.splitlines():
        _, _, name, _id, _ = line.split(None, 4)
        if not name.startswith(args.index):
            continue
        yield {'name': name, 'id': _id}


def get_index_date(name):
    """
    Trying to get index datetime from its last item
    :param name:
    :return:
    """
    global args
    payload = {
       "size": 1,
       "sort": { "timestamp": "desc"},
       "query": {
          "match_all": {}
       }
    }
    headers = {
        'Content-Type': 'application/json'
    }
    uri = '{}/_search'.format(name)
    addr = 'http://{}:{}/{}'.format(args.host, args.port, uri)
    r = requests.post(addr, headers=headers, data=json.dumps(payload))
    s_dt = r.json()['hits']['hits'][0]['_source']['timestamp']
    result = datetime.datetime.strptime(s_dt, '%Y-%m-%dT%H:%M:%S')
    return result



def delete_index(name):
    """
    Delete index from elasticsearch by name
    :param name:
    :return:
    """
    global args
    print('remove index {}'.format(name))
    if args.dry:
        return
    return requests.delete('http://{}:{}/{}'.format(args.host, args.port, name))


def get_divider():
    """
    Return divider for total_secons of index age
    :return:
    """
    global args
    d = {
        'minutes': 60.0,
        'hours': 60.0 * 60.0,
        'days': 60.0 * 60.0 * 24.0
    }
    return d[args.ttl_mode]


args = parse_args()


def main():
    """
    Main function
    :return:
    """
    global args
    now = datetime.datetime.now()
    for index in get_indices():
        try:
            dt = get_index_date(index['name'])
        except ValueError:
            continue
        delta = now - dt
        divider = get_divider()
        if abs(delta.total_seconds()/divider) >= args.ttl_value:
            delete_index(index['name'])


if __name__ == '__main__':
    main()
