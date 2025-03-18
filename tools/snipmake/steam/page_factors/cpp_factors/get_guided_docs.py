# -*- coding: utf-8 -*-

import argparse
import os
import requests
import sys
import urlparse


FETCH_PAGE_TPL = 'fetch?url=%s&offline=1'


def parse_args():
    parser = argparse.ArgumentParser(
        description='Fetch docs with guids from STEAM.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-l', dest='docs_list_file', default='docs_list.txt',
                        help='-')
    parser.add_argument('-d', dest='docs_dir', default='guided_docs',
                        help='-')
    parser.add_argument('-s', dest='server',
                        default='https://mcquack.search.yandex.net:8043',
                        help='-')

    return parser.parse_args()

def main():
    args = parse_args()
    fetch_url_tpl = urlparse.urljoin(args.server, FETCH_PAGE_TPL)
    with open(args.docs_list_file, 'r') as docs_list:
        for line in docs_list:
            line = line.strip()
            fname, url = line.split('\t', 1)
            try:
                doc = requests.get(fetch_url_tpl % url, verify=False)
                with open(os.path.join(args.docs_dir, fname), 'w') as doc_file:
                    doc_file.write(doc.content)
            except requests.exceptions.RequestException as e:
                print >>sys.stderr, e


if __name__ == '__main__':
    main()
