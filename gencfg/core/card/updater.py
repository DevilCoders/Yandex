# single point for all code for safe updating group card fields

# TODO: extended info should be class object not dictionary!!!

import os
from pkg_resources import require

require('PyYAML')
from collections import defaultdict

from core.db import CURDB
from core.card.node import ResolveCardPathError, LeafExtendedInfo, NodeExtendedInfo, Scheme, SchemeLeaf
import core.card.types as card_types


def to_json(value):
    if isinstance(value, (bool, str, int, float, list)) or value is None:
        return value
    return str(value)


class ChoicesFuncs(object):
    def __init__(self):
        pass

    @staticmethod
    def sort_choices(values):
        by_count = defaultdict(int)
        for value in values:
            by_count[value] += 1
        return sorted(set(values), cmp=lambda x, y: cmp((-by_count[x], x), (-by_count[y], y)))

    @staticmethod
    def itype():
        return ChoicesFuncs.sort_choices([x.name for x in CURDB.itypes.get_itypes()])

    @staticmethod
    def ctype():
        return ChoicesFuncs.sort_choices([x.name for x in CURDB.ctypes.get_ctypes()])

    @staticmethod
    def metaprj():
        return ChoicesFuncs.sort_choices([x.card.tags.metaprj for x in CURDB.groups.get_groups()])

    @staticmethod
    def tier():
        return sorted([tier.name for tier in CURDB.tiers.get_tiers()])

    @staticmethod
    def cpu_model():
        return CURDB.cpumodels.get_model_names()

    @staticmethod
    def location():
        return CURDB.hosts.get_all_locations()

    @staticmethod
    def dc():
        return CURDB.hosts.get_all_dcs()

    @staticmethod
    def os():
        return ['Linux']

    # ===================================== RX-219 START ========================================
    @staticmethod
    def security_segment():
        """Group instances segment

        Depending on host instances segment, different security rules are applied. Therefore,
        all instances on one host should be in one segment (except <infra> segment instances,
        which can live together with other segments)"""

        return ['normal', 'sox', 'personal', 'infra']
    # ===================================== RX-219 FINISH =======================================

    @staticmethod
    def legacy_instance_count():
        values = [x.card.legacy.funcs.instanceCount for x in CURDB.groups.get_groups()]
        values = [x for x in values if x]
        return ChoicesFuncs.sort_choices(values) + ['...']

    @staticmethod
    def legacy_instance_power():
        values = [x.card.legacy.funcs.instancePower for x in CURDB.groups.get_groups()]
        values = [x for x in values if x]
        return ChoicesFuncs.sort_choices(values) + ['...']

    @staticmethod
    def legacy_instance_port():
        return None
        # values = [x.legacy.funcs.instancePort for x in CURDB.groups.get_groups()]
        # values = [x for x in values if x]
        # return ChoicesFuncs.sort_choices(values) + ['...']


class CustomReadHandlers(object):
    def __init__(self):
        pass

    @staticmethod
    def reqs_intlookup(src, *args):
        if src.value and os.path.exists(os.path.join('intlookups', src.value)):
            src.readonly = True
        return src


class CardUpdater(object):
    def __init__(self):
        # cache
        self.group_infos = {}

    def recover_tree(self, src):
        root = src[0]
        src = src[1:]
        root["children"] = []
        path_to_node = {tuple(): root}

        for item in src:
            parent = path_to_node[tuple(item["path"][:-1])]
            parent['children'].append(item)
            if "value" not in item:
                item["children"] = []
                path_to_node[tuple(item["path"])] = item

        return root

    def create_fake_general_group(self, obj):
        # front-end requires to have all top level options in "General" group
        general = {"description": "General Options", "display_name": "General Options", "path": ["general"],
                   "is_interesting": True, "children": []}

        top_leaves = [x for x in obj["children"] if 'children' not in x]
        top_children = [x for x in obj["children"] if 'children' in x]
        general["children"] = top_leaves
        top_children = [general] + top_children
        obj["children"] = top_children
        return obj

    def remove_fake_general_group(self, obj):
        for key in obj.keys()[:]:
            if len(key) and key[0] == 'general':
                obj[tuple(key[1:])] = obj[key]
                del obj[key]
        return obj

    # TODO: fast fix
    def get_group_card_info(self, group):
        # group.legacy.funcs.instancePort
        x = self.get_group_card_info_old(group)
        x = self.recover_tree(x)
        x = self.create_fake_general_group(x)

        # add section names for ancors in web-interface
        for child in x["children"]:
            child["anchor_name"] = "_".join(child["path"])

        return x

    def set_instance_port_readonly(self, card_info):
        pass

    def set_group_card_info_readonly(self, card_info):
        nodes = [card_info]
        for node in nodes:
            if 'children' in node:
                nodes.extend(node['children'])
            if 'readonly' in node:
                node['readonly'] = True

    def get_empty_group_card_info(self, group):
        result = self.get_group_card_info(group)
        result['children'] = []
        return result

    def filter_interesting_nodes(self, node):
        self._filter_interesting_nodes(node)

    def _filter_interesting_nodes(self, node):
        assert (node['is_interesting'])
        node['children'] = [(x if 'children' not in x else self._filter_interesting_nodes(x))
                            for x in node['children'] if x['is_interesting']]
        return node

    def get_group_card_info_old(self, group):
        ordered_items = group.card.save_extended_info(group.parent.SCHEME)
        leaves = [item for item in ordered_items if isinstance(item, LeafExtendedInfo)]
        leaves = {tuple(leaf.path): leaf for leaf in leaves}

        # apply custom handlers
        for key, value in leaves.items()[:]:
            custom_handler = '_'.join(value.path)
            custom_handler = getattr(CustomReadHandlers, custom_handler, None)
            if custom_handler:
                leaves[key] = custom_handler(value, group)

        # translate values to string
        for leaf in leaves.values():
            leaf.value = to_json(leaf.value)
            leaf.default_value = to_json(leaf.default_value) if leaf.has_default else None

        # apply inheritance
        inherited_leaves = {path: leaf for (path, leaf) in leaves.items() if leaf.is_inherited}
        if group.card.master is not None:
            master_items = group.card.master.card.save_extended_info(group.parent.SCHEME)
            master_leaves = [item for item in master_items if isinstance(item, LeafExtendedInfo)]
            master_leaves = {tuple(leaf.path): leaf for leaf in master_leaves}
            for path, leaf in inherited_leaves.items():
                assert (path in master_leaves)
                leaf.inherited_value = to_json(master_leaves[path].value)

        # apply choices
        for leaf in leaves.values():
            if leaf.choices_func is None:
                """
                    Get choices info from corresponding card_type
                """
                if issubclass(leaf.card_type.__class__, card_types.ICardType):
                    choices = leaf.card_type.get_avail_values(group.parent.db)
                    if choices is not None:
                        leaf.choices = choices
                    #                continue
            else:
                choices_func = getattr(ChoicesFuncs, leaf.choices_func, None)
                if choices_func is None:
                    raise Exception('Undefined choices_func %s' % leaf.choices_func)
                leaf.choices = choices_func()

        for leaf in leaves.values():
            patch = Scheme.leaf_type_to_simple_form(leaf)
            for k, v in patch.items():
                setattr(leaf, k, v)

        # translate object to dictionary
        # filter all unused fileds
        leaf_filter = {'path', 'value', 'type', 'optional', 'is_list', 'types', 'type_str', 'description', 'readonly',
                       'default_value', 'inherited_value', 'display_name', 'is_interesting'}
        leaf_filter_optional = {'choices'}
        node_filter = {'path', 'description', 'display_name', 'is_interesting'}
        node_filter_optional = set()
        result = []
        for item in ordered_items:
            result.append({})
            if isinstance(item, NodeExtendedInfo):
                filt = node_filter
                filt_opt = node_filter_optional
            else:
                filt = leaf_filter
                filt_opt = leaf_filter_optional
            for key in filt:
                result[-1][key] = getattr(item, key)
            for key in filt_opt:
                result[-1][key] = getattr(item, key, None)

        return result

    def get_group_key_prop(self, group, key, prop):
        if group not in self.group_infos:
            info = self.get_group_card_info_old(group)
            info = {tuple(key['path']): key for key in info}
            self.group_infos[group] = info
        info = self.group_infos[group]
        return info[key][prop]

    def recover_general_node_in_key(self, key):
        if len(key) == 1:
            return tuple(['general'] + list(key))
        return key

    def update_group_card(self, group, updates, mode='api'):
        """
            Update group card

            :param group: group object
            :param updates: dict of things like ('properties', 'expires') -> 26, ('reqs', 'instances', 'power') -> 1234, ('general', 'owners') -> [owner1, owner2]
            :param mode: api or util (in api updating of readonly fields disabled
            :return: (bool has_updates, bool is_ok, dict of invalid values with explanation like: ('properties, 'expires') -> 'Something wrong')
        """

        assert (mode in ['api', 'util'])
        if mode == 'api':
            ignore_readonly = False
        else:
            ignore_readonly = True

        scheme = group.parent.get_scheme()._scheme

        updates = self.remove_fake_general_group(updates)

        invalid_values = {}

        # find paths not found in group card
        for key in updates.iterkeys():
            try:
                scheme_leaf_node = scheme.resolve_scheme_path(key)
                if not isinstance(scheme_leaf_node, SchemeLeaf):
                    invalid_values[key] = "Trying to update not-leaf node"
                    continue

                validate_info = scheme_leaf_node.card_type.validate_for_update(group, updates[key])
                if validate_info.status == card_types.EStatuses.STATUS_FAIL:
                    invalid_values[key] = validate_info.reason
            except ResolveCardPathError, e:
                key = self.recover_general_node_in_key(key)
                invalid_values[key] = str(e)
        if len(invalid_values) > 0:
            return False, False, invalid_values

        # remove all unchanged values, convert values to card_type format
        new_updates = dict()
        for key, value in updates.iteritems():
            scheme_leaf_node = scheme.resolve_scheme_path(key)
            if group.card.resolve_card_path(key) != scheme_leaf_node.card_type.validate_for_update(group, value):
                new_updates[key] = scheme_leaf_node.card_type.validate_for_update(group, value).value
        updates = new_updates
        if not updates:
            return False, True, {}

        # gather all triads (group, key, value) that is to be updated
        group_key_value = []
        for update_key, update_value in updates.items():
            group_key_value.append((group, update_key, update_value))
            for slave in group.getSlaves():
                for key, value in updates.items():
                    scheme_leaf_node = scheme.resolve_scheme_path(key)
                    leaf_value = slave.card.resolve_card_path(key)
                    is_inherited = scheme_leaf_node.inherit and (leaf_value == value)
                    if is_inherited:
                        group_key_value.append((slave, key, value))

        # checks
        for group, key, value in group_key_value:
            pretty_key = '->'.join(key)
            is_readonly = scheme.resolve_scheme_path(key).readonly
            if is_readonly and not ignore_readonly:
                raise Exception('Cannot change group %s key %s: key is readonly' % \
                                 (group.card.name, pretty_key))

        for group, key, value in group_key_value:
            group.card.set_card_value(key, value, group.parent.SCHEME)

        group.refresh_after_card_update()

        return True, True, {}
