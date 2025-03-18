import copy
from collections import OrderedDict

import gaux.aux_shards
import gaux.aux_utils
from core.db import CURDB
from custom_generators.intmetasearchv2.assign_weight import TAssignWeight, generate_cgi_search_prefix
from custom_generators.intmetasearchv2.config_template import TSimpleNode
from custom_generators.intmetasearchv2.inode import INode
from gaux.aux_weights import adjust_weights_with_snippet_load


class TSourceNode(INode):
    """
        Search source
    """
    INTERNAL_KEYS = TSimpleNode.INTERNAL_KEYS + [
        '_options',  # assign weight method
    ]

    OPTIONS_KEYS = [
        '_assign_weight',
        '_lua_port_tpl',

        # snippet params
        '_add_snippet_prefix',
        '_snippet_assign_weight',
        '_snippet_request_ratio',  # ratio of cpu usage for snippet request compared to all requests cpu usage
        '_snippet_lua_port_tpl',  # port template for cgi snippet prefix

        '_source_collection',  # custom path (<yandsearch> by default)
        '_proto',
        '_name',

        # tier an primus list options
        '_primus_list',  # to override primus list by custom string
        '_tier',  # to override tier list by custom string
        '_load_all_tier_primuses',  # if set add to primus list all tier primuses
        '_shardid_format', # redefine shard name in primus list

        '_ipv',  # to override default resolving scheme
        '_attempts',  # custom attempts count when generating line CgiSearchPrefix
        '_allow_zeroint_groups',  # allow int groups without single int (R1 backup)
        '_remove_empty_shards',  # remove empty brigade groups from intlookup

        # new stuff
        '_workload',  # e.g. hamster
        '_use_service_discovery',  # generate sd:// url instead of basesearch list
        '_endpoint_set_prefix',  # endpoint set prefix for SD
        '_dc_group_required',  # try infer dc and add as group or fail
        '_is_direct_fetch',  # This AuxSearch is for direct queries to base
    ]

    slots = ['instances', 'snippet_instances', 'options', 'assign_weight', 'ipv']

    def __init__(self, template_node, coordinator_instance, instances, snippet_instances, copy_params, access_key=None, yp_source=None):
        """
            This node is constructed from list of instances, which are replicas of specific shard.
            Options is a class, describing weight funcs and other stuff.

            :param template_node(TSourceTemplateNode): template node, used to create this one
            :param instances(list of TAssignWeightInstance): list of base replicas
            :param snippet_instances( list of TAssignWeightInstance): list of snippet replicas
            :param copy_params(dict): dict with param like <_source_collection>
            :param access_key(str): key that is needed to access appropriate direct base sources
        """
        super(TSourceNode, self).__init__(template_node.path, template_node.name, template_node.value, [], dict())

        self._internal_fields = template_node._internal_fields.copy()
        for key in ['intlookup', 'itype']:
            self._internal_fields.pop(key, None)
        self._data_fields = copy.copy(template_node._data_fields)
        self.is_direct_fetch = template_node.options.get('_is_direct_fetch', False)
        self.coordinator_instance = coordinator_instance
        self.instances = instances
        self.snippet_instances = snippet_instances
        self.yplookup = template_node.yplookup
        self.yp_source = yp_source
        self.access_key = access_key

        self.options = OrderedDict(self._internal_fields.get('_options', []))
        notfound_options_keys = set(self.options.keys()) - set(TSourceNode.OPTIONS_KEYS)
        if len(notfound_options_keys) > 0:
            raise Exception("Found unknown _options keys <%s> at path <%s>" % (
                            ",".join(notfound_options_keys), self.path))

        # adjust instances weights according to _snippet_request_ratio param
        if self.options.get('_add_snippet_prefix', False) and self.options.get('_snippet_request_ratio', 0.0) != 0:
            before_correction_weights = map(lambda x: (x.instance_power, x.is_ssd_group), self.instances)
            after_correction_weights = adjust_weights_with_snippet_load(before_correction_weights,
                                                                        self.options['_snippet_request_ratio'])
            for instance, new_weight in zip(self.instances, after_correction_weights):
                instance.instance_power = new_weight

        self.options['coordinator_instance'] = self.coordinator_instance

        self.assign_weight = getattr(TAssignWeight, self.options.get('_assign_weight', 'default'))
        self.snippet_assign_weight = getattr(TAssignWeight,
                                             self.options.get('_snippet_assign_weight', 'default_snippet'))

        self.children = map(lambda x: x.copy(copy_params), template_node.children)

        if '_ipv' in self.options:
            self.ipv = map(lambda x: int(x), self.options['_ipv'])
        else:
            self.ipv = [6]

    def render(self, strict=True):
        fields = self._render_fields()
        render_name = self._internal_fields.get('_name', self.name)
        return '<{}>\n{}\n{}\n</{}>'.format(render_name, self.render_children(strict),
                                            gaux.aux_utils.indent('\n'.join(fields)), render_name)

    def _render_fields(self):
        fields = []

        # add CgiSearchPrefix
        instances_groups = set(map(lambda x: x.instance.type, self.instances))
        if instances_groups:
            fields.append("# CgiSearchPrefix groups: %s" % ",".join(sorted(instances_groups)))
        fields.append("# Assign Weight method: %s" % self.options.get('_assign_weight', 'default'))
        weighted_instances = self.assign_weight(self.instances, self.options)
        self.options['port_offset'] = self._workload_port_offset(self.options.get('_workload'), instances_groups)
        cgi_search_prefix = generate_cgi_search_prefix(weighted_instances, self.options)

        if self.access_key:
            fields.append("DirectAccessKey {}".format(self.access_key))
        if self.is_direct_fetch:
            fields.append("IsDirectFetch 1")

        if self.yplookup and self.yp_source:
            def _sd_expression(yp_source):
                source_collection = self.options.get('_source_collection', '')
                if source_collection:
                    return yp_source.sd_expression + '/' + source_collection
                else:
                    return yp_source.sd_expression

            if isinstance(self.yp_source, list):
                sd_expression = " ".join(_sd_expression(yp_source) for yp_source in self.yp_source)
            else:
                sd_expression = _sd_expression(self.yp_source)
            fields.append('CgiSearchPrefix {}'.format(sd_expression))
        elif self.options.get('_use_service_discovery'):
            fields.append("# CgiSearchPrefix {}".format(cgi_search_prefix))
            fields.append(
                "# INTL2 id: {}, Multishard id: {}, shard_id: {}".format(
                    self.instances[0].intl2_group_id,
                    self.instances[0].multishard_id,
                    self.instances[0].shard_id
                )
            )

            if self.instances[0].shard_id is not None:  # INT config, basesearch discovery
                fields.append("CgiSearchPrefix {}".format(
                    " ".join(
                        [yplookup.base_shard_prefix(self.instances[0].shard_id) for yplookup in self.yplookup]
                    )
                ))
            # INT L2 config, INT discovery
            elif self.instances[0].intl2_group_id is not None and self.instances[0].multishard_id is not None:
                fields.append(
                    "CgiSearchPrefix sd://sas@{}-{}-{}/yandsearch".format(
                        self.options['_endpoint_set_prefix'],
                        self.instances[0].intl2_group_id,
                        self.instances[0].multishard_id
                    )
                )
            elif self.instances[0].intl2_group_id is not None:  # MMETA config, INT L2 discovery
                fields.append(
                    "CgiSearchPrefix sd://sas@{}-{}/yandsearch".format(
                        self.options['_endpoint_set_prefix'],
                        self.instances[0].intl2_group_id
                    )
                )
            elif self.instances[0].multishard_id is not None:  # MMETA config, INT discovery
                fields.append(
                    "CgiSearchPrefix sd://sas@{}-{}/yandsearch".format(
                        self.options['_endpoint_set_prefix'],
                        self.instances[0].multishard_id
                    )
                )
            else:
                raise Exception(
                    'Could not make sd:// url with intl2_group_id={}, multishard_id={} and shard_id={}'.format(
                        self.instances[0].intl2_group_id,
                        self.instances[0].multishard_id,
                        self.instances[0].shard_id
                    )
                )
        else:
            if cgi_search_prefix:
                fields.append("CgiSearchPrefix {}".format(cgi_search_prefix))

        if self.primus_list:
            fields.append('PrimusList {}'.format(self.primus_list))
        if self.tier:
            fields.append('Tier {}'.format(self.tier))

        # add CgiSnippetsPrefix
        if self.snippet_instances is not None:
            snippet_options = self.options.copy()
            if '_lua_port_tpl' in snippet_options:
                del snippet_options['_lua_port_tpl']
            if '_snippet_lua_port_tpl' in snippet_options:
                snippet_options['_lua_port_tpl'] = snippet_options['_snippet_lua_port_tpl']

            snippet_weighted_instances = self.snippet_assign_weight(self.snippet_instances, snippet_options)
            cgi_snippet_prefix = generate_cgi_search_prefix(snippet_weighted_instances, snippet_options)
            fields.append("CgiSnippetPrefix %s" % cgi_snippet_prefix)
        return fields

    @staticmethod
    def _workload_port_offset(workload, groups):
        if not workload:
            return None
        if len(groups) > 1:
            raise RuntimeError('Workload is not supported for more than one group')

        group = list(groups)[0].replace('_GUEST', '')
        for w in CURDB.groups.get_group(group).card.workloads:
            if w.name == workload:
                return w.port_offset
        else:
            raise RuntimeError('Unknown workload [{}] in group [{}]'.format(workload, group))

    @property
    def tier(self):
        if '_tier' in self.options:
            return self.options['_tier']
        if self.instances:
            return self.instances[0].tier
        if self.yp_source and not isinstance(self.yp_source, list) and self.yp_source.tier:
            return self.yp_source.tier.name
        if self.yplookup:
            different_tiers = set([yplookup.base.tier.name for yplookup in self.yplookup])
            assert len(different_tiers) == 1
            tier_name = list(different_tiers)[0]
            return tier_name

    @property
    def primus_list(self):
        if self.options.get('_load_all_tier_primuses', False):
            return ','.join(CURDB.tiers.get_tier(self.tier).shard_ids)
        if '_primus_list' in self.options:
            return self.options['_primus_list']
        if self.instances:
            return self.instances[0].primus_list
        if self.yp_source:
            if isinstance(self.yp_source, list):
                primus_list = list(set([primus for yp_source in self.yp_source for primus in yp_source.primus_list]))
            else:
                primus_list = self.yp_source.primus_list
            return ','.join(sorted(primus_list))
