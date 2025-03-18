#!/skynet/python/bin/python

import re
import os
import sys
import pymongo
import datetime
import subprocess as sb
import xml.etree.ElementTree as ET

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from core.svnapi import SvnRepository
from gaux.aux_repo import get_tags_by_tag_pattern


TAG_PATTERN = re.compile('^stable-(?P<branch>\d+)-r(?P<tag>\d+)$')
GENCFG_TAGS_SVN = 'svn+ssh://arcadia.yandex.ru/arc/tags/gencfg'

ALL_HEARTBEAT_C_MONGODB = ','.join([
    'myt0-4012.search.yandex.net:27017',
    'myt0-4019.search.yandex.net:27017',
    'sas1-6063.search.yandex.net:27017',
    'sas1-6136.search.yandex.net:27017',
    'vla1-3984.search.yandex.net:27017',
])


def _parse_svn_log(output):
    result = []
    for line in output.split('\n'):
        if not line:
            continue

        parts = line.split(' | ')

        if len(parts) != 4:
            continue

        commit, user, time, _ = parts
        commit = int(commit[1:])
        date, time, _ = time.split(' ', 2)

        result.append(
            (commit, user, datetime.datetime.strptime('{}T{}'.format(date, time), '%Y-%m-%dT%H:%M:%S'))
        )

    return result


def get_collection(db_name, collection_name):
    return pymongo.MongoReplicaSetClient(
        ALL_HEARTBEAT_C_MONGODB,
        connectTimeoutMS=15000,
        replicaSet='heartbeat_mongodb_c',
        w='majority',
        wtimeout=15000,
    )[db_name][collection_name]


def update_one(collection, query, updates, upsert=True):
    collection.update(query, {"$set": updates}, upsert=upsert)


def find_one(collection, query, sort_key=None, sort_value=None):
    sort_key = sort_key or '$natural'
    sort_value = sort_value or pymongo.ASCENDING
    requests = collection.find(query).sort(sort_key, sort_value).limit(1)
    request = requests[0] if requests.count() else None
    return request


def get_list_last_tags():
    xml_tags = sb.check_output(['svn', 'ls', '--xml', GENCFG_TAGS_SVN])
    tags_tree = ET.fromstring(xml_tags)

    filtered_tags = []
    for entry in tags_tree[0]:
        if not TAG_PATTERN.match(entry[0].text):
            continue
        filtered_tags.append((entry[0].text, int(entry[1].attrib['revision'])))

    filtered_tags.sort(key=lambda x: x[1], reverse=True)

    return filtered_tags


def get_last_tag_commit(tag_name, tag_revision=None):
    tag_commits = _parse_svn_log(sb.check_output(['svn', 'log', '-l', '3', '{}/{}'.format(GENCFG_TAGS_SVN, tag_name)]))
    for commit in tag_commits:
        if tag_revision is None or commit[0] < tag_revision:
            return commit
    return None, None, None


def get_tags_commits_ranges(stop_tag_name=None, limit=10000):
    tags_ranges = {}
    last_tag_name = None

    list_last_tags = get_list_last_tags()
    for i, (tag_name, tag_commit) in enumerate(list_last_tags):
        if i > limit:
            break

        last_commit_in_tag = get_last_tag_commit(tag_name, tag_commit)[0]

        if last_tag_name and last_tag_name in tags_ranges:
            tags_ranges[last_tag_name]['first_commit'] = last_commit_in_tag + 1

        if tag_name == stop_tag_name:
            break

        if tag_name not in tags_ranges:
            tags_ranges[tag_name] = {'first_commit': None, 'last_commit': None}
        tags_ranges[tag_name]['last_commit'] = last_commit_in_tag

        last_tag_name = tag_name

    return tags_ranges


def update_tag_range_in_mongo(tags_ranges_collection, tag_name, first_commit, last_commit):
    update_one(tags_ranges_collection, {'tag': tag_name}, {
        'first_commit': first_commit,
        'last_commit': last_commit
    })


def update_tags_commits_ranges_mongo(stop_tag_name, limit):
    tags_ranges = get_collection('topology_commits', 'tags_ranges')

    tags_commits_ranges = get_tags_commits_ranges(stop_tag_name=stop_tag_name, limit=limit)
    for i, (tag_name, data) in enumerate(tags_commits_ranges.iteritems()):
        update_tag_range_in_mongo(tags_ranges, tag_name, data['first_commit'], data['last_commit'])
        print('{}: written ({}/{})'.format(tag_name, i + 1, len(tags_commits_ranges)))


def get_last_tag_range():
    tags_ranges = get_collection('topology_commits', 'tags_ranges')
    last_tag_range = find_one(tags_ranges, {}, sort_key='last_commit', sort_value=-1)
    return last_tag_range


def update_tag_commits_range(tag):
    repo = SvnRepository(os.path.join(os.path.dirname(__file__), '..', '..'))
    previous_tag = _find_previous_tag(tag, repo)
    if not previous_tag:
        print 'No previous tag found for', tag
        return

    ranges_collection = get_collection('topology_commits', 'tags_ranges')
    previous_tag_record = find_one(ranges_collection, {'tag': previous_tag})
    print 'Previous tag record:', previous_tag_record

    current_tag_commit = int(get_last_tag_commit(tag)[0])
    print 'Current tag commit:', current_tag_commit

    first_commit = previous_tag_record['last_commit'] + 1
    update_tag_range_in_mongo(ranges_collection, tag, first_commit, current_tag_commit)
    return {'tag': tag,
            'last_commit': current_tag_commit,
            'first_commit': first_commit}


def _find_previous_tag(tag, repo):
    tags = get_tags_by_tag_pattern(TAG_PATTERN, repo)
    for i in xrange(len(tags) - 1, -1, -1):
        if tags[i] == tag:
            return tags[i - 1]


def main(args):
    # future single tag mode
    if len(args) == 3:
        record = update_tag_commits_range(args[2])
        print 'Constructed record: ', record
        return 0

    # legacy mode
    # only ('last', None) mode is really used
    stop_tag_name = args[0]
    if args[0] == 'last':
        stop_tag_name = get_last_tag_range()['tag']
    elif args[0] == 'None':
        stop_tag_name = None

    limit = int(10000 if args[1] == 'None' else args[1])

    update_tags_commits_ranges_mongo(stop_tag_name, limit)

    return 0


if __name__ == '__main__':
    if len(sys.argv) not in (3, 4):
        print('Usage: ')
        print('    ./update_tags_commits_ranges.py <stop_tag_name> <limit>')
        print('    can use ./update_tags_commits_ranges.py None None')
        print('    can use ./update_tags_commits_ranges.py last None')
        sys.exit(-1)

    ret_value = main(sys.argv[1:])
    sys.exit(ret_value)
