"""
    Every config (or template or something else) is represented as a tree. Here we describe various tree node types.
    There are several node types:
        - internal nodes (config parts), which can be rendered as text;
        - config nodes, which should be rendered into file;
        - root node, containing all other nodes
"""

import os
import sys

from custom_generators.intmetasearchv2.config_template import TSimpleNode
from custom_generators.intmetasearchv2.inode import INode
from custom_generators.intmetasearchv2.source_node import TSourceNode

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

import re
import copy

import gaux.aux_utils
import gaux.aux_resolver
import gaux.aux_hbf
import gaux.aux_shards
from core.db import CURDB
from core.instances import Instance

INDENT = '   '  # default indent when constructing configs


def find_source_node(node, found_nodes):
    """
        Auxiliary function to search source nodes. Used in INode.recurse_process
    """
    if isinstance(node, TSourceNode):
        found_nodes.append(node)
    return found_nodes


class TLeafNode(INode):
    __slots__ = ['delimiter']

    def __init__(self, path, name, value, kvstore, route_params):
        super(TLeafNode, self).__init__(path, name, value, kvstore, route_params)

        if len(self.children) > 0:
            raise Exception("Class <%s> at path <%s> should not have children (have <%s>)" % (
            self.__class__, path, ",".join(map(lambda x: x.name, self.children))))

        self.delimiter = route_params.get('leaf_node_delimiter', ' ')

    def copy(self, params):
        """
            We want to override copy method in order to calculate BaseSearchCount and other stuff when SearchSource is copied
        """

        other = copy.copy(self)

        if isinstance(other.value, str):
            # check for named placeholders in value
            placeholders = re.findall('\%\((\w+)\)\w', other.value)
            if len(placeholders) > 0:
                for placeholder in placeholders:
                    if placeholder not in params:
                        raise Exception("Looks like value <%s> at path <%s> contain placeholder <%s> which is not in params" % (
                            other.value, other.path, placeholder))
                # unfold placeholders
                other.value = other.value % params

        return other

    def render(self, strict=True):
        return "%s%s%s" % (self.name, self.delimiter, self.value)


class TBuildTagNode(INode):
    """
        Node, generating current tag
    """

    INTERNAL_KEYS = [
        '_build_tag',
    ]

    __slots__ = ['build_tag']

    def __init__(self, path, name, value, kvstore, route_params):
        super(TBuildTagNode, self).__init__(path, name, value, kvstore, route_params)

        if len(self.children) > 0:
            raise Exception("Class <%s> at path <%s> should not have children (have <%s>)" % (
            self.__class__, path, ",".join(map(lambda x: x.name, self.children))))

    def postinit(self):
        self.build_tag = self._internal_fields.get('_build_tag', False)
        if not self.build_tag:
            raise Exception("Invalid value for build_tag <%s> at path <%s>" % (self.build_tag, self.path))

    def render(self, strict=True):
        current_tag = CURDB.get_repo().get_current_tag()
        if not current_tag:
            current_tag = "trunk"
        return "%s %s" % (self.name, current_tag)


class TNokeyNode(INode):
    INTERNAL_KEYS = [
        '_a',
        '_no_key',
        '_value',
    ]

    __slots__ = []

    def __init__(self, path, name, value, kvstore, route_params):
        super(TNokeyNode, self).__init__(path, name, value, kvstore, route_params)

    def postinit(self):
        self.value = self._internal_fields.get('_value', None)

        # peform some checking
        self.check_internal_fields()
        if '_no_key' not in self._internal_fields:
            raise Exception("Key '_no_key' not found at path %s" % (self.path))
        if self._internal_fields['_no_key'] != 1:
            raise Exception("Key '_no_key' value <%s> is invalid (should be <1>)" % (self._internal_fields['_no_key']))
        if '_value' not in self._internal_fields:
            raise Exception("Key '_value' not found at path %s" % (self.path))

        if len(self._data_fields) > 0:
            raise Exception("Found child keys <%s> at path <%s> in _no_key leaf node" % (
            ",".join(map(lambda (x, y): x, self._data_fields)), self.path))

    def render(self, strict=True):
        return "%s" % (self.value,)


class TLineOptionsNode(INode):
    """
        Class, describing something complex, which should be rendered as single line. E. g.:
            - source options: <Options ${ SourceOptions or 'AllowDynamicWeights=0, BaseSearchCount=1, MaxAttempts=4, ProtocolType=proto' }>
            - rearrange options: <ReArrangeOptions FilterBanned(Bans={DocIdBanSrc:{docidban:0,news_docidban:1},CategBanSrc:{categban:0})>
    """

    INTERNAL_KEYS = [
        '_a',  # anchor
        '_commented',  # comment
        '_lua_name',  # option name, if specified add lua expression
        '_delimiter',  # delimiter between options
        '_kvdelimiter',  # delimiter between key and value for every option
    ]

    __slots__ = ['commented', 'lua_name', 'delimiter', 'kvdelimiter']

    def __init__(self, path, name, value, kvstore, route_params):
        super(TLineOptionsNode, self).__init__(path, name, value, kvstore, route_params)

    def postinit(self):
        self.commented = self._internal_fields.get('_commented', False)
        self.lua_name = self._internal_fields.get('_lua_name', None)
        self.delimiter = self._internal_fields.get('_delimiter', ' ')
        self.kvdelimiter = self._internal_fields.get('_kvdelimiter', '=')

    def extra_route_params(self):
        return {
            'leaf_node_delimiter': self.kvdelimiter,
        }

    def render(self, strict=True):
        childs_data = []
        for child in self.children:
            child_data = child.render(strict=strict)
            if child_data.find('\n') >= 0:  # child data should be single line
                raise Exception("Path <%s> rendered as multiline: <%s>" % (child.path, child_data))
            childs_data.append(child_data)
        childs_data = self.delimiter.join(childs_data)

        if self.lua_name is not None:
            return "%s ${%s or '%s'}" % (self.name, self.lua_name, childs_data)
        else:
            return "%s %s" % (self.name, childs_data)


class TCollectionTemplateNode(TSimpleNode):
    """
        Node, used to add DNSCache to collection
    """

    __slots__ = []

    INTERNAL_KEYS = TSimpleNode.INTERNAL_KEYS + [
        '_no_dns_cache',  # set to true if we do not need DNSCache
        '_relaxed_dns_cache',  # silently skip not resolved hostnames
    ]

    def __init__(self, path, name, value, kvstore, route_params):
        super(TCollectionTemplateNode, self).__init__(path, name, value, kvstore, route_params)

    def copy(self, copy_params):
        copy_node = TSimpleNode.copy(self, copy_params)
        copy_node.__class__ = TSimpleNode  # FIXME: do not sure if it is good or not

        # insert ServiceName
        if 'coordinator_instance' in copy_params and isinstance(copy_params['coordinator_instance'], Instance):
            service_name = copy_params['coordinator_instance'].type
        elif 'coordinator_instance' in copy_params and hasattr(copy_params['coordinator_instance'], 'service_name'):
            service_name = copy_params['coordinator_instance'].service_name
        else:
            service_name = 'UNKNOWN'

        copy_node.children.insert(0, TLeafNode(self.path, 'ServiceName', service_name, [], dict()))

        # now recursively traverse and find all TSourceNode
        source_nodes = []
        copy_node.recurse_process(find_source_node, source_nodes)

        # resolve addrs
        relaxed_resolve = self._internal_fields.get('_relaxed_dns_cache', False)
        addrs = ' '.join('{}={}'.format(host, ip) for host, ip in resolve_addrs(source_nodes, relaxed_resolve))

        if self._internal_fields.get('_no_dns_cache', False):
            return [copy_node]
        else:
            # generate node, representing DNSCache
            dns_cache_dict = [
                ('_rtype', TSimpleNode.ERType.XML),
                ('DNSCache', "${ DNSCache and DNSCache or '%s'}" % addrs),
            ]
            dns_cache_node = TSimpleNode(self.path[:-1], 'DNSCache', None, dns_cache_dict, dict())

            return [copy_node, dns_cache_node]

    def render(self, strict=True):
        if strict:
            raise NotImplementedError(
                "Class <%s>, found at path <%s> is meta-class (should be converted to some other class during preparation). This method can not be called at all"
                % (self.path, self.__class__))
        else:
            return TSimpleNode.render(self, strict=strict)


def resolve_addrs(source_nodes, relaxed=False):
    addrs = {}
    for source_node in source_nodes:
        for instance in source_node.instances + (source_node.snippet_instances or []):
            # ======================== GENCFG-1360 START =======================================================
            if hasattr(instance.instance, 'hbf_mtn_addr') and instance.instance.hbf_mtn_addr is not None:
                addrs[instance.instance.host.name] = instance.instance.hbf_mtn_addr
            else:
                try:
                    addrs[instance.instance.host.name] = gaux.aux_resolver.resolve_host(instance.instance.host.name, source_node.ipv, use_curdb=True)
                except RuntimeError:
                    if relaxed:
                        pass
            # ======================== GENCFG-1360 FINISH ======================================================
    return sorted(addrs.items())


class TRenderConfigsResult(object):
    """
        Class with statistics and information on rendered configs (to be transferred through multiprocessing.Queue).
    """

    __slots__ = ['thread_id', 'config_nodes_count', 'generated_configs']

    def __init__(self, thread_id):
        self.thread_id = thread_id
        self.config_nodes_count = 0
        self.generated_configs = []
