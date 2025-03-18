import hashlib
from collections import defaultdict
import copy

from gaux.aux_portovm import may_be_guest_instance


class TPostActions(object):
    def __init__(self):
        pass

    # ====================================== Tag functions ==============================================
    @staticmethod
    def custom_tier(group, params, searcherlookup_generator):
        if params.enabled:
            assert (len(group.card.intlookups) == 0), "Group <%s> have more than 0 intlookups" % group.card.name
            assert (group.parent.db.tiers.has_tier(params.tier_name)), "Tier <%s> not found" % params.tier_name

            mytier = group.parent.db.tiers.get_tier(params.tier_name)
            my_instances = set(group.get_instances())

            searcherlookup_generator.slookup[(mytier.get_shard_id_for_searcherlookup(0), mytier.name)].extend(
                map(lambda x: x.host, my_instances))

            searcherlookup_generator.ilookup[('none', 'none')] = filter(lambda x: x not in my_instances,
                                                                        searcherlookup_generator.ilookup[
                                                                            ('none', 'none')])
            searcherlookup_generator.ilookup[(mytier.get_shard_id_for_searcherlookup(0), mytier.name)].extend(
                list(my_instances))

            searcherlookup_generator.itags_auto['a_tier_none'] = filter(lambda x: x not in my_instances,
                                                                        searcherlookup_generator.itags_auto[
                                                                            'a_tier_none'])
            searcherlookup_generator.itags_auto['a_tier_%s' % params.tier_name].extend(list(my_instances))

    @staticmethod
    def shardid_tag(group, params, searcherlookup_generator):
        if params.enabled:
            if group.card.searcherlookup_postactions.custom_tier.enabled:
                tier = group.parent.db.tiers.get_tier(group.card.searcherlookup_postactions.custom_tier.tier_name)

                if getattr(params, 'tag_format', None) is not None:
                    itags = [params.tag_format % {'shard_id': 0,
                                                  'slookup_shard_name': tier.get_shard_id_for_searcherlookup(0)}]
                elif getattr(params, 'tags_format', []):
                    itags = map(
                        lambda x: x % {'shard_id': 0, 'slookup_shard_name': tier.get_shard_id_for_searcherlookup(0)},
                        params.tags_format)
                else:
                    if params.write_primus_name:
                        s = tier.get_shard_id_for_searcherlookup(0)
                    else:
                        s = '0'
                    itags = ["%s%s" % (params.tag_prefix, s)]

                for itag in itags:
                    searcherlookup_generator.itags_auto[itag].extend(group.cached_busy_instances)
            else:
                for intlookup in map(lambda x: group.parent.db.intlookups.get_intlookup(x), group.card.intlookups):
                    for shard_id in range(intlookup.get_shards_count()):
                        tier_name, tier_shard_id = intlookup.get_tier_and_shard_id_for_shard(shard_id)
                        tier = group.parent.db.tiers.get_tier(tier_name)
                        slookup_shard_name = tier.get_shard_id_for_searcherlookup(tier_shard_id)

                        # FIXME: for OxygenExpPlatinumTier0 with primuses primus-PlatinumTier0-0-0 we somehow need last 0-0 as separate something
                        if len(slookup_shard_name.split('-')) == 5:
                            oxygen_hack_shard_id = '%s-%s' % (
                                slookup_shard_name.split('-')[2], slookup_shard_name.split('-')[3])
                        else:
                            oxygen_hack_shard_id = None

                        for instance in intlookup.get_base_instances_for_shard(shard_id):
                            if getattr(params, 'tag_format', None) is not None:
                                itags = [params.tag_format % {'shard_id': shard_id,
                                                              'slookup_shard_name': slookup_shard_name}]
                            elif getattr(params, 'tags_format', []):
                                itags = map(lambda x: x % {
                                    'shard_id': shard_id,
                                    'slookup_shard_name': slookup_shard_name,
                                    'oxygen_hack_shard_id': oxygen_hack_shard_id,
                                }, params.tags_format)
                            else:
                                if params.write_primus_name:
                                    itags = ["%s%s" % (params.tag_prefix, slookup_shard_name)]
                                else:
                                    itags = ["%s%s" % (params.tag_prefix, tier_shard_id)]

                            for itag in itags:
                                searcherlookup_generator.itags_auto[itag].append(instance)

    @staticmethod
    def gen_by_code_tags(group, params, searcherlookup_generator):
        my_instances = group.cached_busy_instances
        for elem in params:
            func = eval(elem.code)
            for instance in my_instances:
                searcherlookup_generator.itags_auto[func(instance)].append(instance)

    @staticmethod
    def host_memory_tag(group, params, searcherlookup_generator):
        if params.enabled:
            my_instances = group.cached_busy_instances
            for instance in my_instances:
                searcherlookup_generator.itags_auto['ENV_HOST_MEMORY_SIZE=%d' % instance.host.memory].append(instance)

    @staticmethod
    def copy_on_ssd_tag(group, params, searcherlookup_generator):
        if params.enabled:
            my_instances = group.cached_busy_instances
            # my_instances = filter(lambda x: (0 < x.host.ssd < x.host.disk) or (x.host.ssd_size > 0 and x.host.hdd_size > 0), my_instances)
            my_instances = filter(lambda x: (0 < x.host.ssd) or (x.host.ssd_size > 0 and x.host.hdd_size > 0), my_instances)
            searcherlookup_generator.itags_auto['itag_copy_on_ssd'].extend(my_instances)

    @staticmethod
    def gpu_device_tag(group, params, searcherlookup_generator):
        if params.enabled:
            import gaux.gpu
            for instance in group.cached_busy_instances:
                device_name = gaux.gpu.device_name(group, instance)
                if device_name:
                    searcherlookup_generator.itags_auto['gpu_device={}'.format(device_name)].append(instance)

    @staticmethod
    def pre_in_pre_tags(group, params, searcherlookup_generator, slave_group=None):
        # add tags, inherited from master group
        if group.card.master is not None and getattr(group.card.master.card, 'searcherlookup_postactions',
                                                     None) is not None and \
                        getattr(group.card.master.card.searcherlookup_postactions, 'pre_in_pre_tags', None) is not None:
            TPostActions.pre_in_pre_tags(group.card.master,
                                         group.card.master.card.searcherlookup_postactions.pre_in_pre_tags,
                                         searcherlookup_generator, group)

        # add own tags
        if not hasattr(group, "cached_busy_instances"):
            group.cached_busy_instances = group.get_kinda_busy_instances() # do not caclulate get_kinda_busy_instances multiple times
        my_instances = group.cached_busy_instances

        hosts_by_tag = dict()
        add_to_slaves = set()
        for elem in params:
            flt = eval(elem.filter)
            instances = set(filter(flt, my_instances))

            hosts = set(map(lambda x: x.host.name.partition('.')[0], instances))
            for other_tag in elem.exclude:
                if other_tag not in hosts_by_tag:
                    raise Exception("Group <%s> pre_in_pre tag <%s> error: tag <%s> not found in previous pre_in_pre tags" % (group.card.name, elem.seed, other_tag))
                hosts -= hosts_by_tag[other_tag]

            for other_tag in elem.intersect:
                if other_tag not in hosts_by_tag:
                    raise Exception("Group <%s> pre_in_pre tag <%s> error: tag <%s> not found in previous pre_in_pre tags" % (group.card.name, elem.seed, other_tag))
                hosts &= hosts_by_tag[other_tag]
            hosts = list(hosts)

            hosts.sort(key = lambda x: hashlib.md5(x + elem.seed).hexdigest())
            if len(hosts) < elem.count:
                raise Exception("Group <%s> pre_in_pre tag <%s> error: not enough hosts: have %s need %s" % (group.card.name, ",".join(elem.tags), len(hosts), elem.count))
            hosts = set(hosts[:elem.count])

            for tag in elem.tags:
                if tag in hosts_by_tag:
                    raise Exception("Group <%s> pre_in_pre tag <%s> error: tag <%s> already in all pre_in_pre tags" % (group.card.name, elem.seed, tag))
                hosts_by_tag[tag] = hosts
                if elem.affect_slave_groups:
                    add_to_slaves.add(tag)

        # slave group: all hosts calculated for master groups
        if slave_group is not None:
            slave_instances = slave_group.cached_busy_instances
            for tagname, hosts in hosts_by_tag.iteritems():
                if tagname in add_to_slaves:
                    filtered_instances = filter(lambda x: x.host.name.partition('.')[0] in hosts, slave_instances)
                    searcherlookup_generator.itags_auto[tagname].extend(filtered_instances)
        else:
            for tagname, hosts in hosts_by_tag.iteritems():
                filtered_instances = filter(lambda x: x.host.name.partition('.')[0] in hosts, my_instances)
                searcherlookup_generator.itags_auto[tagname].extend(filtered_instances)

    @staticmethod
    def int_with_snippet_reqs_tag(group, params, searcherlookup_generator):
        if params.enabled:
            def _is_good_snippet_brigade(brigade):
                return len(filter(lambda x: x.host.ssd == 0, brigade.get_all_basesearchers())) == 0

            for intlookup in map(lambda x: group.parent.db.intlookups.get_intlookup(x), group.card.intlookups):
                for brigade_group in intlookup.get_multishards():
                    for brigade in brigade_group.brigades:
                        if _is_good_snippet_brigade(brigade):
                            searcherlookup_generator.itags_auto['itag_int_with_snippet_reqs'].extend(
                                filter(lambda x: x.type == group.card.name, brigade.get_all_intsearchers()))

    @staticmethod
    def fixed_hosts_tags(group, params, searcherlookup_generator):
        for elem in params:
            hosts = set(group.parent.db.hosts.get_hosts_by_name(elem.hosts))
            notfound_hosts = filter(lambda x: not group.hasHost(x), hosts)
            if len(notfound_hosts):
                raise Exception("Group <%s> fixed_hosts_tags <%s> error: hosts %s not found in group" % (
                    group.card.name, elem.tag, ",".join(map(lambda x: x.name, notfound_hosts))))

            instances = filter(lambda x: x.host in hosts, group.cached_busy_instances)

            searcherlookup_generator.itags_auto[elem.tag].extend(instances)

    @staticmethod
    def aline_tag(group, params, searcherlookup_generator):
        if params.enabled:
            for instance in group.cached_busy_instances:
                searcherlookup_generator.itags_auto['a_line_%s' % instance.host.queue].append(instance)

    @staticmethod
    def conditional_tags(group, params, searcherlookup_generator):
        for elem in params:
            flt_func = eval(elem.filter)

            filtered = filter(lambda x: flt_func(x), group.cached_busy_instances)

            # temporary commeneted
            # if len(filtered) == 0:
            #     raise Exception("Group <%s> conditional_tags <%s> error: not found hosts satisfying condition <%s>" % (group.card.name, ",".join(elem.tags), elem.filter))

            for tag in elem.tags:
                searcherlookup_generator.itags_auto[tag].extend(filtered)

    @staticmethod
    def memory_limit_tags(group, params, searcherlookup_generator):
        return

    @staticmethod
    def replica_tags(group, params, searcherlookup_generator):
        if params.enabled:
            if len(group.card.intlookups) == 0:
                raise Exception("Group <%s> replica tags error: no intlookups in group" % group.card.name)

            by_replica_instances = defaultdict(list)
            for intlookup in group.card.intlookups:
                intlookup = group.parent.db.intlookups.get_intlookup(intlookup)
                for brigade_group in intlookup.get_multishards():
                    for replica_id, brigade in enumerate(brigade_group.brigades):
                        by_replica_instances[replica_id].extend(
                            filter(lambda x: x.type == group.card.name, brigade.get_all_instances()))

            if params.first_replica_tag is not None:
                searcherlookup_generator.itags_auto[params.first_replica_tag].extend(by_replica_instances[0])
            else:
                fmt = getattr(params, 'tag_format', None)
                if fmt is None:
                    fmt = 'itag_replica_{replica_id}'

                for id_, instances in by_replica_instances.iteritems():
                    searcherlookup_generator.itags_auto[fmt.format(replica_id=id_)].extend(instances)

    @staticmethod
    def snippet_ssd_instance_tag(group, params, searcherlookup_generator):
        """Mark instances, processing snippet requests, with special tag (request by elride@, vbiriukov@)"""

        if params.enabled:
            snippet_instances = []

            if group.card.tags.itype not in ['base', 'shardtool', 'none']:
                raise Exception("Trying to set tag itag_snippet_ssd_instance to non-base group <%s>" % group.card.name)
            if len(group.card.intlookups) == 0:
                return  # temporary
                raise Exception("Trying to set tag itag_snippet_ssd_instance to group <%s> without intlookups" % (
                    group.card.name))

            def _is_good_snippet_brigade(brigade):
                return len(filter(lambda x: x.host.ssd == 0, brigade.get_all_basesearchers())) == 0

            for intlookup in map(lambda x: group.parent.db.intlookups.get_intlookup(x), group.card.intlookups):
                for brigade_group in intlookup.get_multishards():
                    for brigade in brigade_group.brigades:
                        if _is_good_snippet_brigade(brigade):
                            snippet_instances.extend(brigade.get_all_basesearchers())

            searcherlookup_generator.itags_auto["itag_snippet_ssd_instance"].extend(snippet_instances)

    @staticmethod
    def enumerate_instances_tags(group, params, searcherlookup_generator):
        """Set OPTINSTANCE_NUM/OPT_INSTANCE_TOTAL tags (GENCFG-1307)"""
        if params.enabled:
            instances = group.get_kinda_busy_instances()
            instances.sort(key=lambda x: hash('{}:{}'.format(x.host.name, x.port)))
            for i, instance in enumerate(instances):
                searcherlookup_generator.itags_auto['ENV_INSTANCE_NUM={}'.format(i)].append(instance)
            searcherlookup_generator.itags_auto['ENV_INSTANCE_TOTAL={}'.format(len(instances))].extend(instances)

    # ================================== Finish tag functions ========================================
    @staticmethod
    def process(group, searcherlookup_generator):
        for tagname, tagparams in group.card.searcherlookup_postactions.items():
            func = getattr(TPostActions, tagname, None)

            if func is None:
                raise Exception("Not found function <%s> when processing postactions for group %s" % (
                    tagname, group.card.name))

            func(group, tagparams, searcherlookup_generator)
