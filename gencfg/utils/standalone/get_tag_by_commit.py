#!/skynet/python/bin/python

import re
import sys
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


def get_tag_by_commit(commit):
    revision = int(commit)
    for i, (tag_name, tag_commit) in enumerate(reversed(get_list_last_tags())):
        if revision < tag_commit:
            return tag_name


def main(args):
    print(get_tag_by_commit(args[0]))
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: ')
        print('    ./get_tag_by_commit.py <commit>')
        sys.exit(0)

    ret_value = main(sys.argv[1:])
    sys.exit(ret_value)
