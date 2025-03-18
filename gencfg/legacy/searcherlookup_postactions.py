import copy
import hashlib
import string
import os

import gencfg
from gaux.aux_colortext import red_text


class PostActionBase(object):
    def generate(self, searcherlookup_generator):
        raise Exception("Pure virtual method called")


# ==============================================================
# classes for backward compability
# ==============================================================
class PostActionGenerateSpecialTags(PostActionBase):
    NAME = 'special_tags'

    def __init__(self, tree, db):
        del tree, db
        pass

    def generate(self, searcherlookup_generator):
        pass


class PostActionGenerateFirstSecondReplicaTags(PostActionBase):
    NAME = 'first_second_replicas_tags'

    def __init__(self, tree, db):
        del tree, db
        pass

    def generate(self, searcherlookup_generator):
        pass


class PostActionReplaceProjectTags(PostActionBase):
    NAME = 'replace_project_tags'

    def __init__(self, tree, db):
        del tree, db
        pass

    def generate(self, searcherlookup_generator):
        pass


class PostActionsGenerateNightClusterTags(PostActionBase):
    NAME = 'night_cluster'

    def __init__(self, tree, db):
        del tree, db
        pass

    def generate(self, searcherlookup_generator):
        pass


class PostActionGenerateRealsearchTags(PostActionBase):
    NAME = 'realsearch_tags'

    def __init__(self, tree, db):
        del tree, db
        pass

    def generate(self, searcherlookup_generator):
        pass


class PostActionGenerateShardIdTags(PostActionBase):
    NAME = 'shardid_tags'

    def __init__(self, tree, db):
        del tree, db
        pass

    def generate(self, searcherlookup_generator):
        pass


class PostActionGenerateSasSpecialGroups(PostActionBase):
    NAME = 'sas_special_groups'

    def __init__(self, tree, db):
        del tree, db
        pass

    def generate(self, searcherlookup_generator):
        pass


class PostActionGenerateIpv6ExperimentTags(PostActionBase):
    NAME = 'sas_ipv6experiment_tag'

    def __init__(self, tree, db):
        del tree, db
        pass

    def generate(self, searcherlookup_generator):
        pass


# ================================================================
# used classes
# ================================================================
class PostActionGenerateConditionTags(PostActionBase):
    NAME = 'condition_tags'

    def __init__(self, tree, db):
        self.db = db
        self.tags = []
        for elem in tree.findall('tag'):
            tag = elem.find('tagname').text.strip()
            cond = eval(elem.find('cond').text)
            self.tags.append((tag, cond))

    def generate(self, searcherlookup_generator):
        for tag, cond in self.tags:
            instances = []
            for group in searcherlookup_generator.db.groups.get_groups():
                instances.extend(filter(cond, group.get_kinda_busy_instances()))
            searcherlookup_generator.itags_auto[tag].extend(instances)


class PostActionGenerateReplacedInstancesTags(PostActionBase):
    NAME = 'replaced_instances_tags'

    def __init__(self, tree, db):
        del db
        self.filename = tree.find('filename').text.strip()
        self.tagname = tree.find('tagname').text.strip()

    def generate(self, searcherlookup_generator):
        intlookup_infos = [x for x in searcherlookup_generator.intlookups if x.filename == self.filename]
        if len(intlookup_infos) != 1:
            raise Exception("Can not run post action ReplaceInstanceTags with filaname %s and tagname %s" % (
            self.filename, self.tagname))

        intlookup_info = intlookup_infos[0]
        if not len(intlookup_info.intlookup.replaced_instances):
            raise Exception("Intlookup %s does not have replaced instances" % intlookup_info.intlookup.file_name)

        searcherlookup_generator.itags_auto[self.tagname].extend(intlookup_info.intlookup.replaced_instances)


class PostActionGenerateAutoTags(PostActionBase):
    NAME = 'auto_tags'

    def __init__(self, tree, db):
        del tree
        self.db = db

    def generate(self, searcherlookup_generator):
        mapping = dict()

        for group in searcherlookup_generator.db.groups.get_groups():
            instances_data = []
            if len(group.intlookups):
                for intlookup in map(lambda x: searcherlookup_generator.db.intlookups.get_intlookup(x),
                                     group.intlookups):
                    for i in xrange(intlookup.brigade_groups_count * intlookup.hosts_per_group):
                        if intlookup.tiers is None:
                            tier = 'none'
                            primus = 'none'
                        else:
                            tier = intlookup.get_tier_for_shard(i)
                            primus = intlookup.get_primus_for_shard(i)

                        for instance in intlookup.get_base_instances_for_shard(i):
                            instances_data.append((instance, tier, primus))
                        if i % intlookup.hosts_per_group == 0:
                            for instance in intlookup.get_int_instances_for_shard(i):
                                instances_data.append((instance, tier, 'none'))
                instances_data = filter(lambda (x, y, z): x.type == group.name, instances_data)
            else:
                if getattr(group, 'searcherlookup_postactions',
                           None) is not None and group.searcherlookup_postactions.custom_tier.enabled:
                    mytier = self.db.tiers.get_tier(group.searcherlookup_postactions.custom_tier.tier_name)
                    myshard = mytier.get_shard_id_for_searcherlookup(0)
                    instances_data = map(lambda x: (x, mytier.name, myshard), group.get_instances())
                else:
                    instances_data = map(lambda x: (x, 'none', 'none'), group.get_instances())

            for instance, tier, primus in instances_data:
                tags = ['a_geo_%s' % instance.host.location,
                        'a_ctype_%s' % group.tags.ctype,
                        'a_itype_%s' % group.tags.itype]
                for prj in group.tags.prj:
                    tags.append('a_prj_%s' % prj)
                tags.append('a_dc_%s' % instance.host.dc)
                tags.append('a_tier_%s' % string.replace(tier, '_', '-'))

                if self.db.version >= "2.2.6":
                    tags.append('a_metaprj_%s' % group.tags.metaprj)
                    for itag in group.tags.__dict__.get('itag', []):
                        tags.append('itag_%s' % itag)

                tags.append(group.name)
                if group.name in mapping:
                    tags.append(mapping[group.name])
                for tag in tags:
                    searcherlookup_generator.itags_auto[tag].append(instance)

                if primus == 'none':
                    tier = 'none'
                searcherlookup_generator.slookup[(primus, tier)].append(instance.host)
                searcherlookup_generator.ilookup[(primus, tier)].append(instance)


class PostActionGenerateAgriTags(PostActionBase):
    NAME = 'agri_tags'

    def __init__(self, tree, db):
        self.intlookups = map(lambda x: db.intlookups.get_intlookup(x),
                              sorted(tree.find('intlookups').text.strip().split(',')))

    def generate(self, searcherlookup_generator):
        cnt = 0
        for intlookup in self.intlookups:
            for brigade_group in intlookup.brigade_groups:
                for brigade in brigade_group.brigades:
                    for srch in brigade.get_all_intsearchers() + brigade.get_all_basesearchers():
                        searcherlookup_generator.itags_auto['itag_agri_cnt_%s' % cnt].append(srch)
                    cnt += 1


class PostActionGenerateStageTags(PostActionBase):
    NAME = 'stage_tags'

    def __init__(self, tree, db):
        del tree, db
        pass

    def generate(self, searcherlookup_generator):
        if getattr(searcherlookup_generator, 'intlookups', None) is None:  # for backward compability
            return

        for intlookup_info in searcherlookup_generator.intlookups:
            for instance in intlookup_info.intlookup.get_used_instances():
                searcherlookup_generator.itags_auto['switch_stage%d' % instance.host.switch_type].append(instance)


class PostActionGenerateOsolTags(PostActionBase):
    NAME = 'osol_tags'

    def __init__(self, tree, db):
        del db
        self.groups = tree.find('groups').text.strip().split(',')

    def generate(self, searcherlookup_generator):
        for group in self.groups:
            hosts = filter(lambda x: x.model in ['E5645', 'E5-2660'],
                           searcherlookup_generator.db.groups.groups[group].getHosts())
            if not len(hosts):
                raise Exception("No proper hosts for group %s" % group)

            hosts.sort(cmp=lambda x, y: cmp(hashlib.sha224(x.name).hexdigest(), hashlib.sha224(y.name).hexdigest()))
            host = hosts[0]
            searcherlookup_generator.itags_auto['nalivka'].extend(
                searcherlookup_generator.db.groups.groups[group].get_host_instances(host))


def _set_tag_on_hosts(hosts, tagname, searcherlookup_generator):
    groups = set(sum(map(lambda x: searcherlookup_generator.db.groups.get_host_groups(x), hosts), []))
    hosts = set(hosts)
    all_instances = filter(lambda x: x.host in hosts, sum(map(lambda x: x.get_kinda_busy_instances(), groups), []))
    searcherlookup_generator.itags_auto[tagname].extend(all_instances)


class PostActionReplaceAutoTags(PostActionBase):
    NAME = 'replace_auto_tags'

    def __init__(self, tree, db):
        self.prj, self.itype, self.groupname, self.oldgroupname = None, None, None, None
        if tree.find('prj') is not None:
            self.prj = string.replace(tree.find('prj').text.strip(), '_', '-')
        if tree.find('itype') is not None:
            self.itype = string.replace(tree.find('itype').text.strip(), '_', '-')
        if tree.find('oldgroupname') is not None:
            self.oldgroupname = tree.find('oldgroupname').text.strip()
        if tree.find('groupname') is not None:
            self.groupname = tree.find('groupname').text.strip()
        self.intlookups = map(lambda x: db.intlookups.get_intlookup(x), tree.find('intlookups').text.strip().split(','))

    def generate(self, searcherlookup_generator):
        instances = set(sum(map(lambda x: x.get_used_instances(), self.intlookups), []))
        groups = map(lambda x: searcherlookup_generator.db.groups.get_group(x), set(map(lambda x: x.type, instances)))
        if self.prj:
            oldprjs = set(sum(map(lambda x: x.tags.prj, groups), []))
            for oldprj in oldprjs:
                searcherlookup_generator.itags_auto['a_prj_%s' % oldprj] = list(
                    set(searcherlookup_generator.itags_auto['a_prj_%s' % oldprj]) - instances)
            searcherlookup_generator.itags_auto['a_prj_%s' % self.prj].extend(list(instances))
        if self.itype:
            searcherlookup_generator.itags_auto['a_itype_%s' % self.itype].extend(list(instances))
            olditypes = set(map(lambda x: x.tags.itype, groups))
            for olditype in olditypes:
                searcherlookup_generator.itags_auto['a_itype_%s' % olditype] = list(
                    set(searcherlookup_generator.itags_auto['a_itype_%s' % olditype]) - instances)
        if self.groupname:  # and (self.oldgroupname is None or instance.type == self.oldgroupname):
            if self.oldgroupname:
                instances = set(filter(lambda x: x.type == self.oldgroupname, instances))
            searcherlookup_generator.itags_auto[self.groupname].extend(instances)
            for group in groups:
                searcherlookup_generator.itags_auto[group.name] = list(
                    set(searcherlookup_generator.itags_auto[group.name]) - instances)
            if self.oldgroupname:
                searcherlookup_generator.itags_auto[self.oldgroupname] = list(
                    set(searcherlookup_generator.itags_auto[self.oldgroupname]) - instances)


class PostActionGenerateTopologyTags(PostActionBase):
    NAME = 'topology_tags'

    def __init__(self, tree, db):
        del tree, db
        pass

    def generate(self, searcherlookup_generator):
        tag_name = (searcherlookup_generator.db.get_repo().get_current_tag() or 'unknown').replace('/', '-')

        sandbox_task_id = os.environ.get('SANDBOX_TASK_ID')

        for group in searcherlookup_generator.db.groups.get_groups():
            searcherlookup_generator.itags_auto['a_topology_%s' % tag_name].extend(group.get_kinda_busy_instances())
            if sandbox_task_id is not None:
                searcherlookup_generator.itags_auto['a_sandbox_task_%s' % sandbox_task_id].extend(
                    group.get_kinda_busy_instances())


class PostActionGeneratePrestableInPrestableTags(PostActionBase):
    NAME = 'prestable_in_prestable_tags'

    def __init__(self, tree, db):
        self.db = db
        self.data = []
        for text in map(lambda x: x.text.strip(), tree.findall('group')):
            group, N = db.groups.get_group(text.split(',')[0]), int(text.split(',')[1])
            self.data.append((group, N))

        if self.db.version in ['0.5', '0.6']:
            self.tags = [tree.find('tagname').text.strip()]
        else:
            self.tags = tree.find('tags').text.strip().split(',')

        self.exclude_tags = []
        if tree.find('exclude') is not None:
            self.exclude_tags = tree.find('exclude').text.strip().split(',')

        self.include_tags = None
        if tree.find('include') is not None:
            self.include_tags = tree.find('include').text.strip().split(',')

        self.flt = lambda x: True
        if tree.find('filter') is not None:
            self.flt = eval(tree.find('filter').text.strip())

    def generate(self, searcherlookup_generator):
        # check if all excluded tags found in result
        if self.db.version >= "2.2.7":
            for tag in self.exclude_tags:
                if len(searcherlookup_generator.itags_auto[tag]) == 0:
                    print red_text("Not found instances for exclude tag <<%s>>" % tag)

        if self.include_tags is not None:
            include_instances = set(sum(map(lambda x: searcherlookup_generator.itags_auto[x], self.include_tags), []))
        exclude_instances = set(sum(map(lambda x: searcherlookup_generator.itags_auto[x], self.exclude_tags), []))
        for group, N in self.data:
            instances = sum(map(lambda x: x.get_kinda_busy_instances(), [group] + group.slaves), [])
            instances = filter(lambda x: x not in exclude_instances, instances)
            instances = filter(lambda x: self.flt(x), instances)
            if self.include_tags is not None:
                instances = filter(lambda x: x in include_instances, instances)

            hosts = sorted(list(set(map(lambda x: x.host.name.split('.')[0], instances))))
            hosts.sort(cmp=lambda x, y: cmp(hashlib.md5(x + self.tags[0]).hexdigest(),
                                            hashlib.md5(y + self.tags[0]).hexdigest()))

            if self.db.version >= "2.2.6":
                if len(hosts) < N:
                    raise Exception("Not enough hosts for prestable_in_prestable_tags (set tags %s, group %s, have %s, needed %s)" % \
                                     (",".join(self.tags), group.name, len(hosts), N))

            hosts = set(hosts[:N])

            for instance in filter(lambda x: x.host.name.split('.')[0] in hosts, instances):
                for tag in self.tags:
                    searcherlookup_generator.itags_auto[tag].append(instance)


class PostActionGenerateAliasesTags(PostActionBase):
    NAME = 'group_aliases'

    def __init__(self, tree, db):
        self.data = []
        for elem in tree.findall('alias'):
            destname = elem.find('dest').text.strip()
            srcgroups = map(lambda x: db.groups.get_group(x), elem.find('src').text.strip().split(','))
            self.data.append((destname, srcgroups))

    def generate(self, searcherlookup_generator):
        for destname, srcgroups in self.data:
            for srcgroup in srcgroups:
                instances = srcgroup.get_kinda_busy_instances()
                for instance in instances:
                    searcherlookup_generator.itags_auto[destname].append(instance)


class PostActionAddAsearchPseudoInstancesTags(PostActionBase):
    NAME = 'add_asearch_pseudo_instances'

    def __init__(self, tree, db):
        del tree
        self.db = db

    def generate(self, searcherlookup_generator):
        if '2.0' <= self.db.version <= '2.2.3':  # not supported generation of ALL_SEARCH in old tag (whn ALL_SEARCH was not ordinary group)
            return

        for instance in searcherlookup_generator.db.groups.get_group('ALL_SEARCH').get_instances():
            searcherlookup_generator.itags_auto['a_search_all'].append(instance)
            searcherlookup_generator.itags_auto['a_line_%s' % instance.host.queue].append(instance)


class AddAlineTags(PostActionBase):
    NAME = 'aline_tag'

    def __init__(self, tree, db):
        self.groups = map(lambda x: db.groups.get_group(x), tree.find('groups').text.strip().split(','))

    def generate(self, searcherlookup_generator):
        for group in self.groups:
            for instance in group.get_kinda_busy_instances():
                searcherlookup_generator.itags_auto['a_line_%s' % instance.host.queue].append(instance)


class PostActionAddFilteredHostTags(PostActionBase):
    NAME = 'add_filtered_host_tags'

    def __init__(self, tree, db):
        del db
        self.flt = eval(tree.find('filter').text.strip())
        self.tag = tree.find('tag').text.strip()

    def generate(self, searcherlookup_generator):
        hosts = filter(lambda x: self.flt(x), searcherlookup_generator.db.hosts.get_all_hosts())
        _set_tag_on_hosts(hosts, self.tag, searcherlookup_generator)


class PostActionTurSnippetsWorkaroundTags(PostActionBase):
    NAME = 'tur_snippets_workaround'

    def __init__(self, tree, db):
        self.flt = eval(tree.find('flt').text.strip())
        self.tag = tree.find('tag').text.strip()
        self.intlookups = tree.find('intlookups').text.strip().split(',')
        self.db = db

    def generate(self, searcherlookup_generator):
        all_intlookups = map(lambda x: self.db.intlookups.get_intlookup(x), self.intlookups)
        all_instances = sum(map(lambda x: x.get_base_instances(), all_intlookups), [])
        filtered_instances = filter(self.flt, all_instances)
        searcherlookup_generator.itags_auto[self.tag].extend(filtered_instances)


class PostActionGenerateReplicaTags(PostActionBase):
    NAME = 'replica_tags'

    def __init__(self, tree, db):
        self.intlookups = map(lambda x: db.intlookups.get_intlookup(x), tree.find('intlookups').text.strip().split(','))

        if tree.find('tags') is not None:
            self.tagname_by_replica = {}
            for subtree in tree.findall('tags/tag'):
                replica = int(subtree.find('replica').text)
                tagname = subtree.find('tagname').text.strip()

                if replica in self.tagname_by_replica:
                    raise Exception("Replica <%s> found twice" % replica)

                self.tagname_by_replica[replica] = tagname
        else:
            self.tagname_by_replica = None

    def generate(self, searcherlookup_generator):
        for intlookup in self.intlookups:
            for brigade_group in intlookup.brigade_groups:
                for replica_id, brigade in enumerate(brigade_group.brigades):
                    if self.tagname_by_replica is not None:
                        tagname = self.tagname_by_replica.get(replica_id, None)
                        if tagname is None:
                            continue
                    else:
                        tagname = 'itag_replica_%s' % replica_id

                    searcherlookup_generator.itags_auto[tagname].extend(brigade.get_all_basesearchers())


class PostActionIntlookupTags(PostActionBase):
    NAME = 'intlookup_tags'

    def __init__(self, tree, db):
        del tree
        self.db = db

    def generate(self, searcherlookup_generator):
        for intlookup in self.db.intlookups.get_intlookups():
            for tag in intlookup.tags:
                searcherlookup_generator.itags_auto['itag_%s' % tag].extend(intlookup.get_used_instances())


class ConditionalTags(PostActionBase):
    NAME = 'conditional_tags'

    def __init__(self, tree, db):
        self.db = db

        self.tags = tree.find('tags').text.strip().split(',')
        self.instance_filter = eval(tree.find('instance_filter').text.strip())
        self.group_filter = eval(tree.find('group_filter').text.strip())

    def generate(self, searcherlookup_generator):
        filtered_groups = filter(self.group_filter, self.db.groups.get_groups())
        all_instances = sum(map(lambda x: x.get_kinda_busy_instances(), filtered_groups), [])
        filtered_instances = filter(self.instance_filter, all_instances)

        for tagname in self.tags:
            searcherlookup_generator.itags_auto[tagname].extend(filtered_instances)


class InstancePowerTags(PostActionBase):
    NAME = 'instance_power_tags'

    def __init__(self, tree, db):
        del tree
        self.db = db

    def generate(self, searcherlookup_generator):
        for group in self.db.groups.get_groups():
            if group.properties.fake_group or group.properties.background_group:
                continue
            for instance in group.get_kinda_busy_instances():
                searcherlookup_generator.itags_auto['itag_instance_power_%d' % int(instance.power)].append(instance)


class MemoryLimitTags(PostActionBase):
    NAME = 'memory_limit_tags'

    def __init__(self, tree, db):
        self.db = db

        self.group_filter = lambda x: True
        if tree.find('group_filter') is not None:
            self.group_filter = eval(tree.find('group_filter').text.strip())

        if tree.find('tags/tag') is None:
            raise Exception("Not found childs <tag> when processing memory_limit_tags")
        self.tags = []
        for subtree in tree.findall('tags/tag'):
            tagname = subtree.find('name').text.strip()
            valuefunc = eval(subtree.find('func').text.strip())
            checkerfunc = lambda x, y: True
            if subtree.find('checker') is not None:
                checkerfunc = eval(subtree.find('checker').text.strip())
            self.tags.append((tagname, valuefunc, checkerfunc))

    def generate(self, searcherlookup_generator):
        for group in filter(self.group_filter, self.db.groups.get_groups()):
            for instance in group.get_kinda_busy_instances():
                for tagname, valuefunc, checkerfunc in self.tags:
                    if not checkerfunc(instance, self.db):
                        raise Exception("Failed checker for tagname %s on instance %s:%s" % (
                        tagname, instance.host.name, instance.port))

                    value = valuefunc(instance, self.db)
                    searcherlookup_generator.itags_auto['%s%s' % (tagname, value)].append(instance)


class BlinovTags(PostActionBase):
    NAME = 'blinov_tag'

    def __init__(self, tree, db):
        self.db = db

        self.formula = tree.find('formula').text.strip()
        self.tag = tree.find('tag').text.strip()

        assert (self.tag.startswith('itag_'))

    def generate(self, searcherlookup_generator):
        import utils.blinov2.main, utils.blinov2.transport

        blinov_parser = utils.blinov2.main.get_parser()
        blinov_options = {
            'formula': self.formula,
            'result_type': 'result',
            'transport': utils.blinov2.transport.TMultiTransport(
                utils.blinov2.transport.TGencfgGroupsTransport('default', iterate_instances=True)),
        }
        blinov_options = blinov_parser.parse_json(blinov_options, convert_var_types=False)

        found_any = False
        for instance in utils.blinov2.main.main(blinov_options):
            found_any = True
            searcherlookup_generator.itags_auto[self.tag].append(instance)

        return found_any


class AvdonkinReshuffle(PostActionBase):
    NAME = 'avdonkin_reshuffle'

    def __init__(self, tree, db):
        self.db = db

        self.tier = db.tiers.get_tier(tree.find('tier').text.strip())
        self.mapping = dict()
        for subtree in tree.findall('shifts/shift'):
            self.mapping[subtree.find('primus').text.strip()] = subtree.find('set_after').text.strip()

    def generate(self, searcherlookup_generator):
        old_primuses = map(lambda x: self.tier.get_shard_id_for_searcherlookup(x), range(self.tier.get_shards_count()))
        new_primuses = copy.copy(old_primuses)

        for primus, set_after in self.mapping.iteritems():
            if primus not in new_primuses:
                raise Exception("Primus %s not found in tier %s" % (primus, self.tier.name))
            if set_after not in new_primuses:
                raise Exception("Primus %s not fuond in tier %s" % (set_after, self.tier.name))
            old_index = new_primuses.index(primus)
            new_index = new_primuses.index(set_after)

            new_primuses.pop(old_index)
            if old_index < new_index:
                new_primuses.insert(new_index, primus)
            else:
                new_primuses.insert(new_index + 1, primus)

        new_slookup = dict()
        new_ilookup = dict()
        for old_primus, new_primus in zip(old_primuses, new_primuses):
            new_slookup[new_primus] = searcherlookup_generator.slookup[(old_primus, self.tier.name)]
            new_ilookup[new_primus] = searcherlookup_generator.ilookup[(old_primus, self.tier.name)]

        for primus in new_slookup:
            searcherlookup_generator.slookup[(primus, self.tier.name)] = new_slookup[primus]
            searcherlookup_generator.ilookup[(primus, self.tier.name)] = new_ilookup[primus]
