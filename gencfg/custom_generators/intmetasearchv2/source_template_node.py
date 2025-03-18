from collections import defaultdict, OrderedDict

from more_itertools import unique_everseen

import gaux.aux_shards
from core.db import CURDB
from custom_generators.intmetasearchv2.config_template import TSimpleNode, TConfigTemplateNode
from custom_generators.intmetasearchv2.source_node import TSourceNode

from core.instances import TIntl2Group, TMultishardGroup

import aux_utils
import itertools
from assign_weight import TAssignWeightInstance


class TSourceTemplateNode(TSimpleNode):
    """
        Mmeta/int search source node.
        On specific state this node is converted to list of nodes, corresponding to real backend sources
    """

    INTERNAL_KEYS = TSimpleNode.INTERNAL_KEYS + [
        '_intlookup',  # intlookups to get instances from
        '_snippet_intlookup',  # intlookup with snippet instances
        '_related_intlookup',  # add shards with group_number equal to brigade number as aux source
        '_yplookup',  # yp topology description
        '_custom_port',  # use custom port (for hamster and so on)
        '_itype',  # what to get from loaded intlookup (base/int/intl2). Could be automatically calculated
        '_options',  # list of options
    ]

    class EIType(object):
        BASE = 'base'
        EMBEDDING = 'embedding'
        INT = 'int'
        INTL2 = 'intl2'
        RAW = 'raw'

    __slots__ = [
        'intlookup', 'snippet_intlookup', 'yplookup', 'config_type', 'itype', 'options', 'shardid_and_tier_lst',
        '_related_intlookup_generator', 'current_shard_id',
    ]

    def __init__(self, path, name, value, kvstore, route_params):
        super(TSourceTemplateNode, self).__init__(path, name, value, kvstore, route_params)
        self.intlookup = None
        self.yplookup = None
        self.snippet_intlookup = None
        self._related_intlookup_generator = None
        self.shardid_and_tier_lst = None

    def postinit(self):
        TSimpleNode.postinit(self)

        self.config_type = self._route_params.get('config_type', None)
        self.itype = self._internal_fields.get('_itype', TSourceTemplateNode.EIType.BASE)
        self.options = OrderedDict(self._internal_fields.get('_options', {}))

    # Setup current_shard_id before running this method
    def add_source_node(self, position, intlookup, template_node, coordinator_instance, instances, snippet_instances, copy_params):
        result = []
        def generate_direct_intl2(intlookup, key):
            if type(intlookup) == TIntl2Group:
                for i, multishard in enumerate(intlookup.multishards):
                    generate_direct_int(multishard, "{}-{}".format(key, i))
            else:
                generate_direct_int(intlookup, key)

        def generate_direct_int(intlookup, key):
            if type(intlookup) == TMultishardGroup:
                for sub_shardid in xrange(self.intlookup.hosts_per_group):
                    base_instances = []
                    for int_group_id, int_group in enumerate(intlookup.brigades):
                        source_node_params = {
                            "brigade_id": int_group.data_hash,
                            "tier": self.shardid_and_tier_lst[self.current_shard_id][0],
                            "primus_list": self.shardid_and_tier_lst[self.current_shard_id][1] or "",
                            "shard_id": self.current_shard_id,
                        }
                        base_instances.extend(map(lambda x: TAssignWeightInstance(x, source_node_params),
                                                  int_group.basesearchers[sub_shardid]))
                    result.append(TSourceNode(template_node, coordinator_instance, base_instances, None, copy_params, "{}-{}".format(key, sub_shardid)))
                    self.current_shard_id += 1

            else:
                raise Exception("{} is not TMultishardGroup".format(intlookup))
        direct_key = "{}:{}".format(self.anchor, position)

        if self.options.get('_is_direct_fetch', False):
            generate_direct_intl2(intlookup, direct_key)
        else:
            result.append(TSourceNode(template_node, coordinator_instance, instances, snippet_instances, copy_params, direct_key))
        for node in result:
            yield node

    def copy(self, copy_params):
        if '_yplookup' in self._internal_fields:
            self.yplookup = CURDB.yplookups.get_yplookup_by_name(self._internal_fields['_yplookup'])

        if self._internal_fields.get('_intlookup'):
            self.intlookup = aux_utils.calculate_intlookup_expr(self._internal_fields['_intlookup'])
            if self.options.get('_remove_empty_shards', False):
                for intl2_group in self.intlookup.intl2_groups:
                    intl2_group.multishards = filter(lambda x: len(x.brigades) > 0, intl2_group.multishards)

        if '_snippet_intlookup' in self._internal_fields:
            self.snippet_intlookup = aux_utils.calculate_intlookup_expr(self._internal_fields['_snippet_intlookup'])

        if '_related_intlookup' in self._internal_fields:
            self._related_intlookup_generator = RelatedIntlookupGenerator(
                main_intlookup=self.intlookup,
                related_intlookup=aux_utils.calculate_intlookup_expr(self._internal_fields['_related_intlookup']),
                options=self.options
            )

        coordinator_instance = copy_params['coordinator_instance']  # instance to generate config for

        # generate some cache
        if self.options.get('_shardid_format'):
            shard_name = self.options.get('_shardid_format')
            if self.shardid_and_tier_lst is None and self.intlookup:
                self.shardid_and_tier_lst = [(self.intlookup.get_tier_for_shard(x),
                                              shard_name.format(x))
                                             for x in xrange(self.intlookup.get_shards_count())]
        else:
            if self.shardid_and_tier_lst is None and self.intlookup:
                self.shardid_and_tier_lst = [(self.intlookup.get_tier_for_shard(x),
                                              self.intlookup.get_primus_for_shard(x, for_searcherlookup=False))
                                             for x in xrange(self.intlookup.get_shards_count())]

        if self._internal_fields.get('_related_intlookup', False):
            custom_port = self._internal_fields.get('_custom_port', None)
            return self._related_intlookup_generator.generate(self, coordinator_instance, copy_params, custom_port)

        nodes = list(self._generate_source_nodes(coordinator_instance, copy_params))
        if not nodes:
            raise RuntimeError('Got zero instances from intlookup <%s> '
                               'when generating config type <%s> with instances type <%s>' %
                               (self.intlookup.file_name, self.config_type, self.itype))
        return nodes

    def _generate_source_nodes(self, coordinator_instance, copy_params):
        """
            Generate TSourceNodes based on config_type, instance and itype
        """
        if self.yplookup:
            if (self.config_type, self.itype) == (TConfigTemplateNode.EType.MMETA, TSourceTemplateNode.EIType.INTL2):
                for intl2_group in self.yplookup[0].intl2.sources:
                    sub_copy_params = {'base_search_count': len(intl2_group.primus_list)}
                    sub_copy_params.update(copy_params)
                    yield TSourceNode(self, coordinator_instance, [], None,
                                      sub_copy_params, yp_source=intl2_group)
                return
            elif (self.config_type, self.itype) == (TConfigTemplateNode.EType.MMETA, TSourceTemplateNode.EIType.INT):
                group_sizes = set([yplookup.base.group_size for yplookup in self.yplookup])
                assert len(group_sizes) == 1
                group_size = list(group_sizes)[0]

                sub_copy_params = {'base_search_count': group_size}
                sub_copy_params.update(copy_params)

                for yplookup in self.yplookup:
                    for intl1_group in yplookup.intl1.sources:
                        yield TSourceNode(self, coordinator_instance, [], None, sub_copy_params, yp_source=intl1_group)
                return
            elif (self.config_type, self.itype) == (TConfigTemplateNode.EType.MMETA, TSourceTemplateNode.EIType.BASE):
                group_sizes = set([item.base.group_size for item in self.yplookup])
                assert len(group_sizes) == 1
                group_size = list(group_sizes)[0]
                sub_copy_params = {'base_search_count': group_size}
                sub_copy_params.update(copy_params)

                # How to group sources inside single SearchSource tag
                source_id_fnc = lambda source: (source.shard_id, source.primus_list)
                all_sources = sorted([source for yplookup in self.yplookup for source in yplookup.base.sources], key=source_id_fnc)

                for _, sources in itertools.groupby(all_sources, source_id_fnc):
                    yield TSourceNode(self, None, [], None, sub_copy_params, yp_source=list(sources))
                return
            elif (self.config_type, self.itype) == (TConfigTemplateNode.EType.INTL2, TSourceTemplateNode.EIType.INT):
                if _is_yp(coordinator_instance):
                    for source in coordinator_instance.subsources:
                        yield TSourceNode(self, coordinator_instance, [], None, {'base_search_count': source.layer.group_size},
                                          access_key='{}:{}'.format(self.anchor, source.index), yp_source=source)
                    return
            elif (self.config_type, self.itype) == (TConfigTemplateNode.EType.INT, TSourceTemplateNode.EIType.BASE):
                if _is_yp(coordinator_instance):
                    group_sizes = set([item.base.group_size for item in self.yplookup])
                    assert len(group_sizes) == 1
                    group_size = list(group_sizes)[0]
                    sub_copy_params = {'base_search_count': group_size}
                    sub_copy_params.update(copy_params)

                    for source in coordinator_instance.subsources:
                        yield TSourceNode(self, coordinator_instance, [], None, sub_copy_params, yp_source=source)
                    return
            elif (self.config_type, self.itype) == (TConfigTemplateNode.EType.INTL2, TSourceTemplateNode.EIType.BASE):
                if hasattr(coordinator_instance, 'subsources'):
                    for subsource in coordinator_instance.subsources:
                        for subsubsource in subsource.subsources:
                            yield TSourceNode(self, coordinator_instance, [], None,
                                              {'base_search_count': subsubsource.layer.group_size},
                                              access_key='{}:{}-{}'.format(self.anchor, subsource.index, subsubsource.index),
                                              yp_source=subsubsource)
                    return
            elif (self.config_type, self.itype) == (TConfigTemplateNode.EType.INT, TSourceTemplateNode.EIType.EMBEDDING):
                if _is_yp(coordinator_instance):
                    yield TSourceNode(self, coordinator_instance, [], None, {},
                                      yp_source=self.yplookup[0].intl1.sources[coordinator_instance.index])
                    return
            # yp-located int; any other subsource
            elif self.config_type == TConfigTemplateNode.EType.INT and hasattr(coordinator_instance, 'subsources'):
                yield TSourceNode(self, coordinator_instance, [], None, {},
                                  yp_source=self.yplookup[0].base.sources[coordinator_instance.index])
                return
            else:
                assert False, 'Unsupported config pair: {} -> {}'.format(self.config_type, self.itype)

        if self.config_type == TConfigTemplateNode.EType.MMETA and self.itype == TSourceTemplateNode.EIType.INTL2:
            # check if we have intl2 instances
            shardid = 0
            for intl2_group_id, intl2_group in enumerate(self.intlookup.intl2_groups):
                if len(intl2_group.intl2searchers) == 0:
                    raise Exception("No intsl2 in intlookup <%s> at path <%s> while building INT config" % (
                                    self.intlookup.file_name, self.path))

                start_index = shardid
                finish_index = shardid + len(intl2_group.multishards) * self.intlookup.hosts_per_group
                source_node_params = {
                    "tier": ",".join(
                        unique_everseen(map(lambda (x, y): x, self.shardid_and_tier_lst[start_index:finish_index]))),
                    "primus_list": ",".join(map(lambda (x, y): y, self.shardid_and_tier_lst[start_index:finish_index])),
                    "intl2_group_id": intl2_group_id,
                }
                instances = map(lambda x: TAssignWeightInstance(x, source_node_params), intl2_group.intl2searchers)

                sub_copy_params = {'base_search_count': finish_index - start_index}
                sub_copy_params.update(copy_params)
                yield TSourceNode(self, coordinator_instance, instances, None, sub_copy_params)

                shardid += len(intl2_group.multishards) * self.intlookup.hosts_per_group
        elif self.config_type == TConfigTemplateNode.EType.MMETA and self.itype == TSourceTemplateNode.EIType.INT:
            shardid = 0
            for intl2_group_id, intl2_group in enumerate(self.intlookup.intl2_groups):
                for multishard_id, multishard in enumerate(intl2_group.multishards):
                    int_instances = []
                    if self.options.get('_add_snippet_prefix', False):
                        snippet_int_instances = []
                    else:
                        snippet_int_instances = None

                    for int_group in multishard.brigades:
                        if self.options.get('_allow_zeroint_groups', False) is False and len(
                                int_group.intsearchers) == 0:
                            raise Exception("Not found ints in intlookup <%s> when generating int search source" %
                                            self.intlookup.file_name)

                        start_index = shardid
                        finish_index = shardid + self.intlookup.hosts_per_group
                        source_node_params = {
                            "brigade_id": int_group.data_hash,
                            "instance_power": int_group.power / max(len(int_group.intsearchers), 1),
                            "tier": ",".join(unique_everseen(
                                map(lambda (x, y): x, self.shardid_and_tier_lst[start_index:finish_index]))),
                            "primus_list": ",".join(
                                map(lambda (x, y): y, self.shardid_and_tier_lst[start_index:finish_index])),
                            "multishard_id": multishard_id,
                        }
                        if len(filter(lambda x: x.host.ssd == 0, int_group.get_all_basesearchers())) == 0:
                            source_node_params["is_ssd_group"] = True
                        else:
                            source_node_params["is_ssd_group"] = False
                        int_instances.extend(
                            map(lambda x: TAssignWeightInstance(x, source_node_params), int_group.intsearchers))

                        if self.options.get('_add_snippet_prefix', False) and self.snippet_intlookup is None:
                            # check if group we have ssd machines only
                            if len(filter(lambda x: x.host.ssd == 0, int_group.get_all_basesearchers())) == 0:
                                source_node_params["instance_power"] = int_group.power
                            else:
                                source_node_params["instance_power"] = 0
                            snippet_int_instances.extend(
                                map(lambda x: TAssignWeightInstance(x, source_node_params), int_group.intsearchers))

                    if len(int_instances) == 0:
                        raise Exception("Not found ints in intlookup <%s> when generating int search source" % (
                                        self.intlookup.file_name))

                    # special case for having different snippet and search intlookups
                    # FIXME: code dublication
                    if self.options.get('_add_snippet_prefix', False) and self.snippet_intlookup is not None:
                        if self.intlookup.hosts_per_group != self.snippet_intlookup.hosts_per_group:
                            raise Exception('Intlookup <{}> has <{}> hosts per group, '
                                            'while <{}> has <{}> hosts per group'.format(
                                                self.intlookup.file_name, self.intlookup.hosts_per_group,
                                                self.snippet_intlookup.file_name, self.snippet_intlookup.hosts_per_group
                                            ))

                        snippet_multishard = self.snippet_intlookup.intl2_groups[intl2_group_id].multishards[
                            multishard_id]
                        for int_group in snippet_multishard.brigades:
                            source_node_params = {
                                "brigade_id": int_group.data_hash,
                                "instance_power": int_group.power,
                            }
                            if len(filter(lambda x: x.host.ssd == 0, int_group.get_all_basesearchers())) == 0:
                                source_node_params["is_ssd_group"] = True
                                source_node_params["instance_power"] = int_group.power
                            else:
                                source_node_params["is_ssd_group"] = False
                                source_node_params["instance_power"] = 0
                                # check if group we have ssd machines only
                            snippet_int_instances.extend(
                                map(lambda x: TAssignWeightInstance(x, source_node_params), int_group.intsearchers))

                    sub_copy_params = {'base_search_count': self.intlookup.hosts_per_group}
                    sub_copy_params.update(copy_params)
                    yield TSourceNode(self, coordinator_instance, int_instances, snippet_int_instances, sub_copy_params)

                    shardid += self.intlookup.hosts_per_group
        elif self.config_type == TConfigTemplateNode.EType.MMETA and self.itype == TSourceTemplateNode.EIType.BASE:
            """
                Almost same to mmetas, using ints, but get basesearchers as backends
            """
            shardid = 0
            for intl2_group in self.intlookup.intl2_groups:
                for multishard in intl2_group.multishards:
                    for sub_shardid in xrange(self.intlookup.hosts_per_group):
                        base_instances = []
                        for int_group in multishard.brigades:
                            source_node_params = {
                                "brigade_id": int_group.data_hash,
                                "tier": self.shardid_and_tier_lst[shardid + sub_shardid][0],
                                "primus_list": self.shardid_and_tier_lst[shardid + sub_shardid][1],
                            }
                            base_instances.extend(map(lambda x: TAssignWeightInstance(x, source_node_params),
                                                      int_group.basesearchers[sub_shardid]))

                        sub_copy_params = {'base_search_count': 1}
                        sub_copy_params.update(copy_params)
                        yield TSourceNode(self, coordinator_instance, base_instances, None, sub_copy_params)

                    shardid += self.intlookup.hosts_per_group
        elif self.config_type == TConfigTemplateNode.EType.INT and self.itype == TSourceTemplateNode.EIType.EMBEDDING:
            assert self.yplookup, 'itype=embedding is used only with yp sources'

            # assume that partition number equals to multishard position
            for partition, multishard in enumerate(self.intlookup.get_multishards()):
                if multishard.has_intsearch(coordinator_instance):
                    break

            yield TSourceNode(self, coordinator_instance, [], None, {},
                              yp_source=self.yplookup[0].intl1.sources[partition])
        elif self.config_type == TConfigTemplateNode.EType.INT and self.itype == TSourceTemplateNode.EIType.BASE:
            """
                Generate int config for <coordinator_instance>.
                First, search group containing this instance and than generate
                config, with this int and basesearchers in corresponding int group
            """
            # find correct multishard with coordinator_instance inside
            shardid = 0
            found = False
            for multishard in self.intlookup.get_multishards():
                if multishard.has_intsearch(coordinator_instance):
                    found = True
                    break
                shardid += self.intlookup.hosts_per_group
            if not found:
                raise Exception("Can not find int instance <%s> in intlookup <%s> "
                                "while generating TSourceNode at path <%s>" % (
                                coordinator_instance.name(), self.intlookup.file_name, self.path))


            for sub_shardid in xrange(self.intlookup.hosts_per_group):
                base_instances = []
                if self.options.get('_add_snippet_prefix', False):
                    snippet_base_instances = []
                else:
                    snippet_base_instances = None
                for int_group in multishard.brigades:
                    source_node_params = {
                        "brigade_id": int_group.data_hash,
                        "tier": self.shardid_and_tier_lst[shardid + sub_shardid][0],
                        "primus_list": self.shardid_and_tier_lst[shardid + sub_shardid][1],
                        "is_main": coordinator_instance in int_group.intsearchers,
                        "shard_id": shardid + sub_shardid,
                    }
                    base_instances.extend(map(lambda x: TAssignWeightInstance(x, source_node_params),
                                              int_group.basesearchers[sub_shardid]))

                    if self.options.get('_add_snippet_prefix', False):
                        # check if group we have ssd machines only
                        if len(filter(lambda x: x.host.ssd == 0, int_group.basesearchers[sub_shardid])) == 0:
                            source_node_params["is_ssd_group"] = True
                        else:
                            source_node_params["is_ssd_group"] = False
                        snippet_base_instances.extend(map(lambda x: TAssignWeightInstance(x, source_node_params),
                                                          int_group.basesearchers[sub_shardid]))

                sub_copy_params = {'base_search_count': 1}
                sub_copy_params.update(copy_params)
                yield TSourceNode(self, coordinator_instance, base_instances, snippet_base_instances, sub_copy_params)
        elif self.config_type == TConfigTemplateNode.EType.INTL2 and self.itype == TSourceTemplateNode.EIType.INT:
            """
                Generate intl2 config for <coordinator_instance>.
                First, search group containing this instance and than generate
                config, with this int and intsearchers in corresponding intl2 group
            """
            # find correct intl2_group with coordinator_instance inside
            shardid = 0
            found = False
            for intl2_group_id, intl2_group in enumerate(self.intlookup.intl2_groups):
                if coordinator_instance in intl2_group.intl2searchers:
                    found = True
                    break
                shardid += self.intlookup.hosts_per_group * len(intl2_group.multishards)
            if not found:
                raise Exception("Can not find int instance <%s> in intlookup <%s> while generating TSourceNode at path <%s>" % (
                                coordinator_instance.name(), self.intlookup.file_name, self.path))

            result = []
            for multishard_id, multishard in enumerate(intl2_group.multishards):
                int_instances = []
                if self.options.get('_add_snippet_prefix', False):
                    snippet_int_instances = []
                else:
                    snippet_int_instances = None

                start_index = shardid + multishard_id * self.intlookup.hosts_per_group
                end_index = shardid + (multishard_id + 1) * self.intlookup.hosts_per_group
                for int_group_number, int_group in enumerate(multishard.brigades):
                    source_node_params = {
                        "brigade_id": int_group.data_hash,
                        "tier": self.shardid_and_tier_lst[start_index][0],
                        "primus_list": ",".join(
                            map(lambda (x, y): y, self.shardid_and_tier_lst[start_index:end_index])),
                        "instance_power": int_group.power / max(len(int_group.intsearchers), 1),
                        "is_main": False,
                        "multishard_id": multishard_id,
                        "intl2_group_id": intl2_group_id,
                    }

                    if self.options.get('_add_snippet_prefix', False):
                        # check if group we have ssd machines only
                        snippet_source_node_params = source_node_params.copy()
                        if len(filter(lambda x: x.host.ssd == 0, int_group.get_all_basesearchers())) == 0:
                            snippet_source_node_params["instance_power"] = int_group.power
                            snippet_source_node_params["is_ssd_group"] = True
                        else:
                            snippet_source_node_params["instance_power"] = 0
                            snippet_source_node_params["is_ssd_group"] = False
                        snippet_int_instances.extend(
                            map(lambda x: TAssignWeightInstance(x, snippet_source_node_params), int_group.intsearchers))

                    int_instances.extend(
                        map(lambda x: TAssignWeightInstance(x, source_node_params), int_group.intsearchers))

                sub_copy_params = {'base_search_count': self.intlookup.hosts_per_group}
                sub_copy_params.update(copy_params)
                self.current_shard_id = start_index
                for node in self.add_source_node(multishard_id, multishard, self, coordinator_instance, int_instances, snippet_int_instances, sub_copy_params):
                    yield node
        elif self.config_type == TConfigTemplateNode.EType.MMETA and self.itype == TSourceTemplateNode.EIType.RAW:
            yield TSourceNode(self, coordinator_instance, [], None, {})
        else:
            raise Exception("Mode with config_type <%s> and itype <%s> not implemented yet (%s)" % (
                            self.config_type, self.itype, self.anchor))

    def render(self, strict=True):
        if strict:
            raise NotImplementedError(
                "Class <%s> is meta-class (should be converted to some other class during preparation). "
                "This method can not be called at all" % self.__class__)
        else:
            return TSimpleNode.render(self, strict=strict)


class RelatedIntlookupGenerator(object):
    def __init__(self, main_intlookup, related_intlookup, options=None):
        self.main_intlookup = main_intlookup
        self.related_intlookup = related_intlookup
        self.main_brigade_number_to_ints = self._map_intlookup_brigade_number_to_ints(main_intlookup, related_intlookup)
        if options.get('_shardid_format'):
            shard_name = options.get('_shardid_format')
            self.related_shardname_and_tier = map(
                lambda x: (
                    self.related_intlookup.get_tier_for_shard(x),
                    shard_name.format(x)
                ),
                xrange(self.related_intlookup.get_shards_count())
            )
        else:
            self.related_shardname_and_tier = map(
                lambda x: (
                    self.related_intlookup.get_tier_for_shard(x),
                    self.related_intlookup.get_primus_for_shard(x, for_searcherlookup=False)
                ),
                xrange(self.related_intlookup.get_shards_count())
            )

    @classmethod
    def iter_int_group_shard_name(cls, intlookup):
        shardid = 0
        for intl2_group in intlookup.intl2_groups:
            for multishard in intl2_group.multishards:
                for sub_shardid in xrange(intlookup.hosts_per_group):
                    for int_group in multishard.brigades:
                        shard_name = intlookup.get_primus_for_shard(shardid + sub_shardid, for_searcherlookup=False)
                        yield int_group, shard_name
                shardid += intlookup.hosts_per_group

    @classmethod
    def _map_intlookup_brigade_number_to_ints(cls, main_intlookup, related_intlookup):
        result = defaultdict(list)
        group_numbers = main_intlookup.shards_count() / related_intlookup.shards_count()
        for int_group, shard_name in cls.iter_int_group_shard_name(main_intlookup):
            partition_number_ = cls.main_intlookup_shard_partition_number(shard_name, group_numbers)
            result[partition_number_].extend(int_group.intsearchers)
        return dict(result)

    @staticmethod
    def main_intlookup_shard_partition_number(shard_template, group_numbers):
        return gaux.aux_shards.shard_in_partition_number(shard_template) / group_numbers

    def generate(self, parent_node, coordinator_instance, copy_params, custom_port):
        """
            Search source with related shards.
            Add shards & instances with group_number equal to instance brigade number.
        """
        result = []
        shardid = 0
        for intl2_group in self.related_intlookup.intl2_groups:
            for multishard in intl2_group.multishards:
                for sub_shardid in xrange(self.related_intlookup.hosts_per_group):
                    base_instances = []
                    tier, primus_list = self.related_shardname_and_tier[shardid + sub_shardid]
                    for int_group in multishard.brigades:
                        source_node_params = {
                            "brigade_id": int_group.data_hash,
                            "tier": tier,
                            "primus_list": primus_list,
                        }
                        instances = self.main_brigade_number_to_ints[shardid + sub_shardid]
                        if _is_yp(coordinator_instance):
                            if coordinator_instance.index == shardid:
                                base_instances.extend(map(
                                    lambda x: TAssignWeightInstance(x, source_node_params, custom_port=custom_port),
                                    int_group.basesearchers[sub_shardid]
                                ))
                        elif coordinator_instance in instances:
                            base_instances.extend(map(
                                lambda x: TAssignWeightInstance(x, source_node_params, custom_port=custom_port),
                                int_group.basesearchers[sub_shardid]
                            ))

                    sub_copy_params = {'base_search_count': 1}
                    sub_copy_params.update(copy_params)
                    if base_instances:
                        result.append(
                            TSourceNode(parent_node, coordinator_instance, base_instances, None, sub_copy_params)
                        )

                shardid += self.related_intlookup.hosts_per_group
        return result


def _is_yp(coordinator_instance):
    return hasattr(coordinator_instance, 'subsources')  # coordinator_instance is a SearchSource

