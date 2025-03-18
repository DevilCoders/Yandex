#!/skynet/python/bin/python

import argparse
import os
import sys

from collections import defaultdict

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gaux.aux_mongo

from utils.mongo.populate_dns import get_gencfg_data


def cleanup(limit=None, clean=False):
    trunk_data = {record.dns.ipv6addr: record.dns.hostname for record in get_gencfg_data(None)}

    coll = gaux.aux_mongo.get_mongo_collection('gencfg_dns')

    result = defaultdict(list)
    for elem in coll.find():
        result[elem['ipv6addr']].append((elem['hostname'], elem['commit']))
    result = {ipv6addr: fqdn_commits_list for ipv6addr, fqdn_commits_list in result.iteritems() if
              len(fqdn_commits_list) > 1}

    print 'Total {} elems'.format(len(result))

    processed = 0

    for ipv6addr, fqdn_commits_list in result.iteritems():
        if len(fqdn_commits_list) == 1:
            continue

        trunk_fqdn = trunk_data.get(ipv6addr)
        fqdn_commits_list.sort(key=lambda (fqdn, commit): ((trunk_fqdn == fqdn), commit), reverse=True)

        print 'Ipv6addr {}: {}'.format(ipv6addr, fqdn_commits_list)

        for fqdn, commit in fqdn_commits_list[1:]:
            print '    Removing {} {} {}'.format(ipv6addr, fqdn, commit)
            if clean:
                coll.remove({'ipv6addr': ipv6addr, 'hostname': fqdn, 'commit': commit})

        processed += 1
        if limit and processed >= limit:
            break


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-y', '--yes', action='store_true', help='do cleanup')
    parser.add_argument('-l', '--limit', type=int)
    options = parser.parse_args()

    cleanup(limit=options.limit, clean=options.yes)


if __name__ == '__main__':
    main()
