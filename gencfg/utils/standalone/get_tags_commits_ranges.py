#!/skynet/python/bin/python

import re
import sys
import json
import subprocess as sb
import xml.etree.ElementTree as ET


TAG_PATTERN = re.compile('^stable-(?P<branch>\d+)-r(?P<tag>\d+)$')
GENCFG_TAGS_SVN = 'svn+ssh://arcadia.yandex.ru/arc/tags/gencfg'


def get_list_last_tags():
    xml_tags = sb.check_output(['svn', 'ls', '--xml', GENCFG_TAGS_SVN])
    tags_tree = ET.fromstring(xml_tags)

    filtred_tags = []
    for entry in tags_tree[0]:
        if not TAG_PATTERN.match(entry[0].text):
            continue
        filtred_tags.append((entry[0].text, int(entry[1].attrib['revision'])))

    filtred_tags.sort(key=lambda x: x[1], reverse=True)

    return filtred_tags


def get_tags_commits_ranges(stop_tag_name=None, limit=10000):
    tags_ranges = {}
    last_tag_name = None

    list_last_tags = get_list_last_tags()
    for i, (tag_name, tag_commit) in enumerate(list_last_tags):
        if i > limit:
            break

        if last_tag_name and last_tag_name in tags_ranges:
            tags_ranges[last_tag_name]['first_commit'] = tag_commit + 1

        if tag_name == stop_tag_name:
            break

        if tag_name not in tags_ranges:
            tags_ranges[tag_name] = {'first_commit': None, 'last_commit': None}
        tags_ranges[tag_name]['last_commit'] = tag_commit

        last_tag_name = tag_name

    return tags_ranges


def main(args):
    stop_tag_name = None if args[0] == 'None' else args[0]
    limit = int(10000 if args[1] == 'None' else args[1])

    print(json.dumps(get_tags_commits_ranges(stop_tag_name=stop_tag_name, limit=limit), indent=4))
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: ')
        print('    ./get_tags_commits_ranges.py <stop_tag_name> <limit>')
        print('    can use ./get_tags_commits_ranges.py None None')
        sys.exit(0)

    ret_value = main(sys.argv[1:])
    sys.exit(ret_value)
