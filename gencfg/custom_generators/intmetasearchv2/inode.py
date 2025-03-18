from collections import OrderedDict

import copy

import gaux.aux_utils


class INode(object):
    """
        Interface class
    """

    INTERNAL_KEYS = []

    __slots__ = [
        'path',  # path to current node: list of (nodename, anchor) from root
        'name',  # node name
        'value',  # node value (can be None for some node types)
        'anchor',
        # node anchor (used when we have more than one node with same nodename) (can be None for almost all nodes)
        'children',  # list of children nodes. Can be absend in some node types
        '_route_params',  # dict of route params (params, gathered while traversing)
        '_internal_fields',  # OrderedDict of internal keys
        '_data_fields',  # list of pairs of data keyes
    ]

    def __init__(self, parent_path, name, value, kvstore, route_params):
        """
            Constructor does not construct valid object.
            This is the first stage, when path, internal_fields, data_fields are filled.
            And than real initialization from what remains in internal_fields and data_fields.

            :param parent_path: (list of (str, str)) path to current node in tree
            :param name: current node name
            :param value: current node value (can be None)
            :param kvstore: list of pairs: (key, value). Value could be any complex yaml structure
            :param route_params: (dict) from all parent nodes, which can be used to correctly construct current node
        """

        self.name = name
        self.value = value
        self.anchor = dict(kvstore).get('_a', None)
        self.path = parent_path + [(self.name, self.anchor)]
        self.children = []

        self._route_params = route_params
        self._internal_fields = self.extract_internal_fields(kvstore)
        self._data_fields = self.extract_data_fields(kvstore)

        self.postinit()

        self.fill_child_nodes()

    def postinit(self):
        """
            Initialize internal structures after dividinig fields into internal and data
        """
        pass

    def extra_route_params(self):
        """
            Get extra route params from node
        """

        return dict()

    def render(self, strict=True):
        """
            Convert inode structure to multiline text string

            :param strict: if <strict> is set, check if we have nodes that should not be printed
            (meta nodes that should be converted to something else)
        """
        raise NotImplementedError("Method <render> is not implemented")

    def extract_internal_fields(self, kvstore):
        internal_fields = filter(lambda (x, y): (x.startswith('_')) or (x in self.__class__.INTERNAL_KEYS), kvstore)
        internal_keys = map(lambda (x, y): x, internal_fields)

        # check repeating keys
        repeating_keys = set(filter(lambda x: internal_keys.count(x) > 1, internal_keys))
        if len(repeating_keys) > 0:
            msg = ["\nKeys <%s> mentioned more than once at path %s" % (",".join(repeating_keys), self.path)]
            for repeating_key in repeating_keys:
                msg.append("   Values for key <%s>:" % repeating_key)
                for elem in filter(lambda (x, y): x == repeating_key, kvstore):
                    msg.append("        %s" % elem[1])
            raise Exception("\n".join(msg))

        return OrderedDict(internal_fields)

    def check_internal_fields(self):
        # check unknown to current node keys
        notfound_keys = filter(lambda x: x not in self.__class__.INTERNAL_KEYS, self._internal_fields.iterkeys())
        if len(notfound_keys) > 0:
            raise Exception("Found unknown internal fields <%s> for node of type <%s> at path %s" % (
                ",".join(notfound_keys), self.__class__.__name__, self.path))

    def extract_data_fields(self, kvstore):
        return filter(lambda (x, y): (not x.startswith('_')) and (x not in self.__class__.INTERNAL_KEYS), kvstore)

    def fill_child_nodes(self):
        """
            Fill child nodes. Function is common for every node type that support it
        """
        from custom_generators.intmetasearchv2.node import TLeafNode, TNokeyNode, TLineOptionsNode, TBuildTagNode, \
            TCollectionTemplateNode
        from custom_generators.intmetasearchv2.source_template_node import TSourceTemplateNode
        from custom_generators.intmetasearchv2.config_template import TSimpleNode
        from custom_generators.intmetasearchv2.config_template import TConfigTemplateNode

        route_params = self._route_params.copy()
        route_params.update(self.extra_route_params())
        for k, v in self._data_fields:
            # child node type heavily depends on what we have in data
            # FIXME: make better identification of node types
            if isinstance(v, (str, int, bool, float, type(None))):
                child = TLeafNode(self.path, k, v, [], route_params)
            elif isinstance(v, list):
                child_store = v
                if '_no_key' in map(lambda (x, y): x, child_store):
                    child = TNokeyNode(self.path, k, None, child_store, route_params)
                elif '_lua_name' in map(lambda (x, y): x, child_store):
                    child = TLineOptionsNode(self.path, k, None, child_store, route_params)
                elif '_build_tag' in map(lambda (x, y): x, child_store):
                    child = TBuildTagNode(self.path, k, None, child_store, route_params)
                elif '_config' in map(lambda (x, y): x, child_store):
                    child = TConfigTemplateNode(self.path, k, None, child_store, route_params)
                elif k in ['SearchSource', 'AuxSource']:
                    child = TSourceTemplateNode(self.path, k, None, child_store, route_params)
                elif k == 'Collection':
                    child = TCollectionTemplateNode(self.path, k, None, child_store, route_params)
                else:
                    child = TSimpleNode(self.path, k, None, child_store, route_params)
            else:
                raise Exception("Found wrong type <%s> at path <%s> when processing key <%s>" % (type(v), self.path, k))

            self.children.append(child)

    def render_childs(self, strict=True):
        childs_data = "\n".join(map(lambda x: gaux.aux_utils.indent(x.render(strict=strict)), self.children))
        return childs_data

    render_children = render_childs  # :/

    def recurse_process(self, func, data):
        newdata = func(self, data)

        for child in self.children:
            child.recurse_process(func, newdata)

    def copy(self, params):
        """
            Shallow copy of tree.
            Traverse deeply our structure and do not traverse things like self.some_instance.
            Overloaded for some classes, which are copied in strange way.
        """

        other = copy.copy(self)

        other.children = []
        for child in self.children:
            child_copy = child.copy(params)
            if isinstance(child_copy, list):  # copy generated list of nodes (for example TSourceTemplateNode)
                other.children.extend(child_copy)
            else:
                other.children.append(child_copy)

        return other

    def get_anchor_name(self, prefix=None):
        result = []
        for (nodename, anchor) in self.path[1:]:
            if anchor is None:
                result.append(nodename)
            else:
                result.append("%s[%s]" % (nodename, anchor))

        if prefix is None:
            fname = self.path[0][0]
        else:
            fname = self.path[0][0][len(prefix):]

        return "%s:%s" % (fname, ".".join(result))
