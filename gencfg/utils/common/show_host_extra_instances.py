#!/skynet/python/bin/python
"""Utility to show extra instances on host (instances of old or removed groups) (RX-197)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import json
import re
import subprocess
import socket

import api.cqueue
from kernel.util.errors import formatException

try:  # fix to start script on remote machine
    import gencfg
    from core.db import CURDB
    from core.argparse.parser import ArgumentParserExt
    import core.argparse.types
    import config
    import gaux.aux_utils
    import gaux.aux_colortext
    from core.settings import SETTINGS
    from gaux.aux_utils import getlogin
except ImportError:
    pass


class EMethod(object):
    """Method to retrieve information on raised instances on host"""
    SKYNET = "skynet"
    CLUSTERSTATE = "clusterstate"
    ALL = [SKYNET, CLUSTERSTATE]


class TFindRaisedInstances(object):
    """Class, executed on remote host by skynet to get list of raised instances"""

    def __init__(self):
        self.osUser = getlogin()

    def run(self):
        try:
            result = []

            hostname = socket.gethostname()

            ih_listtags_data = subprocess.check_output(['ih', 'listtags'])
            ih_listtags_data = ih_listtags_data.strip().split('\n')

            ih_list_data = subprocess.check_output(['ih', 'list', '-a', '-s']).strip()

            if ih_list_data == '':
                return []

            for line in ih_list_data.split('\n'):
                # load data from <ih listtags>
                configuration_name = line.split(' ')[0].strip(':')
                try:
                    index = ih_listtags_data.index(configuration_name) + 1
                except ValueError:  # some instances are not found in listtags, because they a being restarted now
                    continue

                instance_port = int(configuration_name.partition('@')[0])

                m = re.match('.* a_topology_group-([^ ]+).*', ih_listtags_data[index])
                if m is None:
                    continue
                instance_groupname = m.group(1)

                m = re.match('.* a_topology_version-([^ ]+).*', ih_listtags_data[index])
                if m is None:
                    continue
                instance_release_tag = m.group(1)

                # load current memory guaranee from portoctl
                porto_container_name  = 'ISS-AGENT--{}/iss_hook_start'.format(line.split(' ')[1])
                try:
                    instance_mem_guarantee = int(subprocess.check_output(['portoctl', 'get', porto_container_name, 'memory_guarantee']))
                except (subprocess.CalledProcessError, OSError):
                    instance_mem_guarantee = None

                result.append(TRaisedInstanceInfo(hostname, instance_port, instance_groupname, release_tag=instance_release_tag, mem_guarantee=instance_mem_guarantee))

            return result
        except subprocess.CalledProcessError as e:
            raise Exception((e.returncode, e.cmd, e.output))


def get_last_tag():
    if get_last_tag.result is None:
        tag_prefix = 'stable-{}-r'.format(config.BRANCH_VERSION)
        get_last_tag.result = 'stable-{}-r{}'.format(config.BRANCH_VERSION, gaux.aux_utils.get_last_release(CURDB.get_repo(), tag_prefix))
    return get_last_tag.result
get_last_tag.result = None

class TRaisedInstanceInfo(object):
    """Info on raise instance"""

    def __init__(self, hostname, port, groupname, release_tag=None, mem_guarantee=None):
        self.hostname = hostname
        self.port = port
        self.groupname = groupname
        self.release_tag = release_tag
        self.mem_guarantee = mem_guarantee

    def compare_and_diff(self):
        """Compare remote instance state with our db"""
        if self.mem_guarantee is not None:
            instance_str = '{} ({:.3f} Gb)'.format(self.as_str(), self.mem_guarantee / 1024. / 1024 / 1024)
        else:
            instance_str = '{} (unknown slot size)'.format(self.as_str())

        host = CURDB.hosts.get_host_by_name(self.hostname)
        host_instances = CURDB.groups.get_host_instances(host)

        compare_instance = [x for x in host_instances if x.port == self.port and x.type == self.groupname]
        if compare_instance:  # have instance in our db
            group = CURDB.groups.get_group(self.groupname)
            if (self.mem_guarantee is not None) and abs(self.mem_guarantee - group.card.reqs.instances.memory_guarantee.value) > 1000000:
                return 'Instance {}: slot size in db is {:.3f} Gb required recluster {} => {}'.format(instance_str,
                        group.card.reqs.instances.memory_guarantee.value / 1024. / 1024 / 1024, self.release_tag, get_last_tag())
        else:
            if CURDB.groups.has_group(self.groupname):  # have group in our db
                return 'Instance {}: not found in last tag, required reculster {} => {}'.format(instance_str, self.release_tag, get_last_tag())
            else:
                return 'Instance {}: group {} removed or renamed'.format(instance_str, self.groupname)

        return None

    def as_str(self, verbose=0):
        if verbose == 0:
            return '{}:{}:{}'.format(self.hostname, self.port, self.groupname)
        else:
            if self.mem_guarantee is None:
                mem_guarantee_str = 'None'
            else:
                mem_guarantee_str = '{:.3f} Gb'.format(self.mem_guarantee / 1024. / 1024 / 1024)
            return 'Instance {}:{}:{} (rev. {}, guarantee {})'.format(self.hostname, self.port, self.groupname, self.release_tag, mem_guarantee_str)


def get_parser():
    parser = ArgumentParserExt(usage='Show extra instances on host')

    parser.add_argument('-s', '--hosts', type=core.argparse.types.hosts, required=True,
                        help='Obligatory. Comma-separate dist of hosts to process')
    parser.add_argument('-m', '--method', type=str, default=EMethod.CLUSTERSTATE,
                        choices=EMethod.ALL,
                        help='Optional. Method to retrieve info about raised instances: one of {} ({} by default'.format(
                             EMethod.CLUSTERSTATE, ','.join(EMethod.ALL)))
    parser.add_argument('-v', '--verbose', action='count', default=0,
                        help='Optional. Verbose level (maximum is 1)')

    return parser


def raised_instances_from_clusterstate(options):
    """Load raised instances info from clusterstate"""

    result = []
    for host in options.hosts:
        url = '{}/hosts/{}'.format(SETTINGS.services.clusterstate.rest.url, host.name)
        content = gaux.aux_utils.retry_urlopen(3, url)
        jsoned = json.loads(content)

        # parse instances section
        for k in sorted(jsoned.get('i', dict())):
            v = jsoned['i'][k]
            hostname, _, port = k.partition(':')
            hostname = '{}.search.yandex.net'.format(hostname)
            try:
                port = int(port)
            except ValueError:
                port = None

            groupname = v['group_from_tag']
            if not groupname:
                continue

            release_tag = v['version_from_tag']
            if release_tag is not None:
                release_tag = int(release_tag)
                if release_tag < 1000000000:
                    release_tag = 'stable-{}-r{}'.format(release_tag / 1000000, release_tag % 1000000)
                else:
                    release_tag = 'trunk-{}'.format(release_tag)

            result.append(TRaisedInstanceInfo(hostname, port, groupname, release_tag=release_tag, mem_guarantee=None))

    return result


def raised_isntances_from_skynet(options):
    """Load raised instances info using skynet to execute code on remote host"""
    client = api.cqueue.Client('cqudp', netlibus=True)
    client.register_safe_unpickle('__main__', 'TRaisedInstanceInfo')

    skynet_iterator = client.run([x.name for x in options.hosts], TFindRaisedInstances()).wait(timeout=20)

    result = []
    for host, host_raised_instances, failure in skynet_iterator:
        if failure is not None:
            print 'Failure on {}'.format(host)
            print 'Error message <{}>'.format(formatException(failure))
            raise Exception('Skynet failed on <{}> with message <{}>'.format(host, formatException(failure)))
        else:
            result.extend(host_raised_instances)
    return result


def main(options):
    # load instances
    if options.method == EMethod.CLUSTERSTATE:
        print gaux.aux_utils.red_text('[WARN]: memory guarantee is not tested in <--method {}> mode'.format(options.method))
        raised_instances = raised_instances_from_clusterstate(options)
    elif options.method == EMethod.SKYNET:
        print gaux.aux_utils.red_text(('[WARN]: some instances can be skipped in <--method {}> mode (instances are '
                                      'being restarted now or instances without groupname specified)').format(options.method))
        raised_instances = raised_isntances_from_skynet(options)
    else:
        raise Exception('Unknown method <{}>'.format(options.method))

    if options.verbose > 0 :
        print 'Found {} instances:'.format(len(raised_instances))
        for raised_instance in raised_instances:
            print '    {}'.format(raised_instance.as_str(verbose=options.verbose))

    # compare
    raised_instances_diff = [x.compare_and_diff() for x in raised_instances]
    raised_instances_diff = '\n'.join(x for x in raised_instances_diff if x is not None)

    print 'Found diff:\n{}'.format(gaux.aux_utils.indent(raised_instances_diff))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
