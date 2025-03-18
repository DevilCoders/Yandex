#!/skynet/python/bin/python

import argparse
import errno
import os
import re
import sys

path = os.path.abspath(__file__)
for i in range(5):
    path = os.path.dirname(path)
sys.path.append(path)

import gencfg
import config
from core.db import CURDB

CONFIG_HEADER = """
tickTime=2000
initLimit=100
syncLimit=5
dataDir={data_dir}
clientPort={port}
leaderServes=yes

autopurge.purgeInterval=1
autopurge.snapRetainCount=50

"""


def format_instance(instance):
    return "{0}:{1}".format(instance.host.name.partition('.')[0], instance.port)


def write_instance_configs(instances, n, options):
    # write_myid_file
    instance_id, instance = instances[n]
    iname = format_instance(instance)

    # http://zookeeper.apache.org/doc/r3.4.5/zookeeperAdmin.html#sc_configuration
    # The myid file consists of a single line containing only the text of that machine's id.
    # So myid of server 1 would contain the text "1" and nothing else. The id must be unique within
    # the ensemble and should have a value between 1 and 255.

    target_directory = os.path.join(config.GENERATED_DIR, 'zookeeper')
    if not os.path.exists(target_directory):
        try:
            os.makedirs(target_directory)
        except OSError as e:
            if e.errno != errno.EEXIST:  # FIXME: incorrect when target_directory is file
                raise

    fname = iname + '.id'
    with open(os.path.join(target_directory, '%s' % fname), 'w') as fd:
        fd.write('%s\n' % (instance_id + 1))

    fname = iname + '.cfg'
    with open(os.path.join(target_directory, '%s' % fname), 'w') as fd:
        kwargs = dict(
            port=instance.port,
            data_dir='/db/bsconfig/webstate/{host}:{port}/zookeeper'.format(
                host=instance.host.name.partition('.')[0],
                port=instance.port,
            )
        )
        fd.write(CONFIG_HEADER.format(**kwargs))

        if options.quorum_listen_all:
            fd.write("quorumListenOnAllIPs=true\n")

        template = "server.{id}={host}:{leader_port}:{election_port}\n"
        for instance_id_, instance_ in instances:
            host_ = instance_.host.name
            if options.force_search_yandex_net:
                host_ = re.sub('\.yandex\.ru$', '.search.yandex.net', host_)

            kwargs = dict(
                id=instance_id_,
                host=host_,
                leader_port=instance_.port + 1,
                election_port=instance_.port + 2,
            )
            fd.write(template.format(**kwargs))


def parse_cmd():
    parser = argparse.ArgumentParser(description='Generate zookeeper config.')
    parser.add_argument('-i', dest='intlookup_name', action='store', required=True,
                        help='Obligatory. Intlookup file name')
    parser.add_argument('--force-search-yandex-net', action='store_true', default=False,
                        help="Optional. Force replacement '.yandex.ru' to '.search.yandex.net'")
    parser.add_argument('--quorum-listen-all', action='store_true', default=False,
                        help="Add option 'quorumListenOnAllIPs=true' to config")

    return parser.parse_args()


if __name__ == '__main__':
    options = parse_cmd()

    intlookup = CURDB.intlookups.get_intlookup(options.intlookup_name)

    instances = enumerate(intlookup.get_used_instances_by_shard())
    instances = filter(lambda (x, y): len(y) > 0, instances)
    instances = map(lambda (x, y): (x + 1, y[0]), instances)

    for i in range(len(instances)):
        write_instance_configs(instances, i, options)
