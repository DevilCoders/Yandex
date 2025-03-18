"""
    Searcherlookup structures.
"""

import inspect
import os
from collections import defaultdict
from xml.etree import cElementTree
import gc

import legacy.searcherlookup_postactions as searcherlookup_postactions
from core.hosts import FakeHost
from core.instances import FakeInstance


class TBaseSearcherlookup(object):
    """
        Basic searcherlookup structure. Every group generates this type one during searcherlookup generation.
    """
    KEYS = ['slookup', 'ilookup', 'stags', 'itags_auto', 'instances']

    def __init__(self):
        for key in self.KEYS:
            setattr(self, key, defaultdict(list))

    def normalize(self, normalize_instances=True):
        for key in self.KEYS:
            if key == 'instances' and (not normalize_instances):
                continue

            subdict = getattr(self, key)
            for subkey in subdict:
                subdict[subkey].sort()

    def append_slookup(self, other):
        for key in self.KEYS:
            mydict = getattr(self, key)
            otherdict = getattr(other, key)
            for subkey in otherdict:
                mydict[subkey].extend(otherdict[subkey])


# class representing info about searcherlookup tags and instances
# created from searcherlookup.conf
class StrippedSearcherlookup(object):
    """
        Class representing info about searcherlookup tags and instances. Created from searcherlookup.conf
    """

    class EStateTypes(object):
        START = 0
        SLOOKUP = 1
        ILOOKUP = 2
        SHARD_TAG = 3
        INSTANCE_TAG = 4

    def __init__(self, fname):
        self.slookup = defaultdict(list)
        self.ilookup = defaultdict(list)
        self.stags = defaultdict(list)
        self.itags_auto = defaultdict(list)
        self.instances = defaultdict(list)

        state_type = self.EStateTypes.START
        state_data = None
        try:
            gc.disable()
            for line in open(fname).readlines():
                line = line.strip()

                if line == '':
                    continue

                if line.startswith('%'):
                    if line.startswith('%slookup'):
                        state_type = self.EStateTypes.SLOOKUP
                    elif line.startswith('%ilookup'):
                        state_type = self.EStateTypes.ILOOKUP
                    elif line.startswith('%shard_tag'):
                        state_type = self.EStateTypes.SHARD_TAG
                        state_data = line.partition(' ')[2]
                    elif line.startswith('%instance_tag'):
                        state_type = self.EStateTypes.INSTANCE_TAG
                        state_data = line.partition(' ')[2]
                    elif line.startswith('%instance_tag_auto'):
                        state_type = self.EStateTypes.INSTANCE_TAG
                        state_data = line.partition(' ')[2]
                    elif line.startswith('%'):
                        raise Exception("Can not parse line %s" % line)
                else:
                    if state_type == self.EStateTypes.INSTANCE_TAG:
                        hostname, _, port = line.partition(':')
                        instance = FakeInstance(FakeHost(hostname), int(port))

                        self.itags_auto[state_data].append(instance)
                        self.instances[instance].append(state_type)
                    elif state_type == self.EStateTypes.SLOOKUP:
                        hosts = line.split(' ')
                        primus = hosts[0]
                        hosts = map(lambda x: FakeHost(x), hosts[2:-2])

                        self.slookup[primus] = hosts
                    elif state_type == self.EStateTypes.ILOOKUP:
                        instances = line.split(' ')
                        primus = instances[0]

                        if instances[1:-2] == ['']:  # no instances for this tier
                            instances = []
                        else:
                            instances = map(lambda x: FakeInstance(FakeHost(x.split(':')[0]), int(x.split(':')[-1])),
                                            instances[1:-2])

                        self.ilookup[primus] = instances
                    elif state_type == self.EStateTypes.SHARD_TAG:
                        self.stags[state_data].append(line)
                    else:
                        raise Exception("Can not be in state %s" % state_type)
        finally:
            gc.enable()


class Searcherlookup(TBaseSearcherlookup):
    def __init__(self, db, config_path=None):
        super(Searcherlookup, self).__init__()

        self.db = db
        self.init(config_path=config_path)

    def init(self, config_path=None):
        if config_path is None:
            config_path = os.path.join(self.db.CONFIG_DIR, 'searcherlookups', 'web.xml')

        tree = cElementTree.parse(open(config_path))

        if self.db.version <= '2.1':
            subtree = tree.find('searcherlookup/postactions')
            self.filename = tree.find('searcherlookup/filename').text.strip()
        else:
            subtree = tree.find('postactions')
            self.gendir = tree.find('gendir').text.strip()
            self.filename = tree.find('filename').text.strip()

        postactions_by_name = {}
        for name, obj in inspect.getmembers(searcherlookup_postactions):
            if inspect.isclass(obj):
                try:
                    postactions_by_name[obj.NAME] = obj
                except:
                    pass

        self.postactions = []
        if subtree is not None:
            for elem in subtree:
                if elem.tag not in postactions_by_name:
                    raise Exception("Unknown postaction %s" % elem.tag)
                self.postactions.append(postactions_by_name[elem.tag](elem, self.db))

    def _primus_name_by_shard_name(self, name):
        if name.startswith('fasttier'): return '-'.join(name.split('-')[:2])
        return name.partition('-')[0]

    def write_searcherlookup(self, fname):
        f = open(os.path.join(fname), 'w', 2 * 1024 * 1024)

        # %slookup
        # primus_shard_id_0  primus  hosts_list   # tier
        # primus_shard_id_1  primus  hosts_list   # tier
        # ...
        print >> f, "%slookup"
        for primus, tier in sorted(self.slookup.keys()):
            hosts = list(set(self.slookup[(primus, tier)]))
            hosts = sorted(list(set(map(lambda x: x.name.partition('.')[0], hosts))))
            print >> f, "%s %s %s # %s" % (primus, self._primus_name_by_shard_name(primus), ' '.join(hosts), tier)
        print >> f, ''

        # %ilookup
        # primus_shard_id_0   instances_list   # tier
        # primus_shard_id_1   instances_list   # tier
        # ...
        print >> f, "%ilookup"
        for primus, tier in sorted(self.ilookup.keys()):
            instances = list(set(self.ilookup[(primus, tier)]))
            instances = sorted(list(set(map(lambda x: '%s:%s' % (x.host.name.split('.')[0], x.port), instances))))
            print >> f, "%s %s # %s" % (primus, ' '.join(instances), tier)
        print >> f, ''

        # %shard_tag tier
        # primus_shard_id_0
        # primus_shard_id_1
        # ...
        for tier in sorted(self.stags.keys(), key=lambda x: x.name):
            print >> f, '\n'.join(['%%shard_tag %s' % tier.name] + sorted(list(set(self.stags[tier]))))
            print >> f, ''

        # %instance_tag_auto tag
        # instance_0
        # instance_1
        # ...
        for k in sorted(self.itags_auto.keys()):
            idata = sorted(list(set(map(lambda x: '%s:%s' % (x.host.name.partition('.')[0], x.port), self.itags_auto[k]))))
            if len(idata) > 0:
                print >> f, '\n'.join(['%%instance_tag_auto %s' % k] + idata)
                print >> f, ''

    def generate_searcherlookup(self):
        for group in self.db.groups.get_groups():
            if group.card.tags.metaprj == 'unsorted':
                continue

            group_sgen = group.generate_searcherlookup(normalize=False)
            self.append_slookup(group_sgen)

        # fill stag section
        for tier in self.db.tiers.get_tiers():
            self.stags[tier].extend(
                map(lambda x: tier.get_shard_id_for_searcherlookup(x), range(tier.get_shards_count())))

        # run general postactions
        for postaction in self.postactions:
            postaction.generate(self)

        # write instances
        for itag in self.itags_auto:
            for instance in self.itags_auto[itag]:
                self.instances[instance].append(itag)

        # check slookup for hosts with same short name and different domain
        domain_by_short_name = defaultdict(set)
        for lst in self.slookup.itervalues():
            for host in lst:
                sname, _, domain = host.name.partition('.')
                domain_by_short_name[sname].add(domain)
        for sname in domain_by_short_name:
            if len(domain_by_short_name[sname]) > 1:
                print "Host <%s> with multiple domains in searcherlookup: %s" % (
                    sname, ",".join(domain_by_short_name[sname]))

        self.normalize()

    def update(self):  # do nothing
        pass
