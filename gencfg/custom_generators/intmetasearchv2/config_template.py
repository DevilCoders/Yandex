import copy

import gaux.aux_utils
from core.db import CURDB
from aux_utils import may_be_guest_instance, calculate_intlookup_expr
from inode import INode


class TSimpleNode(INode):
    INTERNAL_KEYS = [
        '_a',  # anchor name
        '_rtype',  # some sections are rendered differently (some rendered as xml node, some as python node
        '_xml_attributes',  # attributes for xml renderer
        '_commented',  # attribute to comment conten of current node
        '_name',  # attribute to set custom name instead of default one
    ]

    class ERType(object):
        """
            Enum with render types
                - some nodes rendered as xml section, e.g. <Collection autostart="must" meta="yes" id="yandsearch"> ...
                - some nodes rendered as indented entity
                - some nodes want child content to be rendered in single line
        """
        XML = 'xml'
        REARRITEM = 'rearritem'
        REARRLIST = 'rearrlist'
        INDENT = 'default'
        ALL = [XML, INDENT, REARRITEM, REARRLIST]

    __slots__ = ['rtype', 'xml_attributes', 'commented']

    def __init__(self, path, name, value, kvstore, route_params):
        """
            Initialize node from dict, loaded from yaml.
            Dict keys divided into two parts:
                - service fields
                - children fields
        """
        super(TSimpleNode, self).__init__(path, name, value, kvstore, route_params)

    def postinit(self):
        if self.value is not None:
            raise Exception("Found non-none value <%s> for TSimpleNode at path %s" % (self.value, self.path))

        # fill internal keys
        self.check_internal_fields()

        self.rtype = self._internal_fields.get('_rtype', 'default')
        if self.rtype not in TSimpleNode.ERType.ALL:
            raise Exception("Unknown render type <%s> for simple node at path %s" % (self.rtype, self.path))
        if self.rtype == TSimpleNode.ERType.XML:
            self.xml_attributes = self._internal_fields.get('_xml_attributes', {})
        elif 'xml_attributes' in self._internal_fields:
            raise Exception("Found _xml_attributes for simple node with type <%s> at path %s" % (self.rtype, self.path))

        self.commented = bool(self._internal_fields.get('_commented', False))
        if self.commented and self.rtype == TSimpleNode.ERType.REARRITEM:
            raise Exception("Set flag _commented along with _rtype rearritme at pat %s" % (self.path))

    def _render_xml_attr(self):
        if len(self.xml_attributes) > 0:
            return " " + " ".join(map(lambda (k, v): '%s="%s"' % (k, v), self.xml_attributes))
        else:
            return ""

    def render(self, strict=True):
        """
            Convert node (and all its childs) to string representation
        """
        print_name = self._internal_fields.get('_name', self.name)
        if self.rtype == TSimpleNode.ERType.XML:
            result = "<%s%s>\n%s\n</%s>" % \
                     (print_name, self._render_xml_attr(), self.render_childs(strict=strict), print_name)
        elif self.rtype == TSimpleNode.ERType.INDENT:
            result = "%s:\n%s" % (print_name, self.render_childs(strict=strict))
        elif self.rtype == TSimpleNode.ERType.REARRITEM:  # Fusion(Child1Name=Child1Value,Child2Name=Child2Value,...)
            childs_data = map(lambda x: "%s=%s" % (x.name, x.value), self.children)
            if len(childs_data):
                result = "%s(%s)" % (print_name, ",".join(childs_data))
            else:
                result = "%s" % (print_name,)
        elif self.rtype == TSimpleNode.ERType.REARRLIST:  # OptionName ${OptionName or 'Child1 Child2 ... ChildN'}
            childs_data = map(lambda x: x.render(strict=strict), self.children)
            result = "%s ${%s or '%s'}" % (print_name, print_name, " ".join(childs_data))
        else:
            raise Exception("Should never happen")

        # apply comment
        if self.commented:
            comment_prefix = "#"
        else:
            comment_prefix = ""
        return gaux.aux_utils.indent(result, comment_prefix)


class TConfigTemplateNode(INode):
    """
        Search config template. Always defined at top level.
    """

    INTERNAL_KEYS = TSimpleNode.INTERNAL_KEYS + [
        '_config',  # key, that identify node type as TConfigTemplateNode
        '_config_type',  # config type: mmeta/int/intl2 (see EType)
        '_instances',  # config is generated for these instances (can be specified as intlookup or group name)
        '_filename',
        '_yplookup'
        # when we generate config for mmetas, we might require the same name for all configs, rather than
        # a separate config for each instance (all mmeta instances have the same config)
    ]

    class EType(object):
        MMETA = 'mmeta'
        INT = 'int'
        INTL2 = 'intl2'
        ALL = [MMETA, INT, INTL2]

    __slots__ = ['config_type', 'instances', 'filename', 'yplookup']

    def __init__(self, path, name, value, kvstore, route_params):
        self.yplookup = None
        super(TConfigTemplateNode, self).__init__(path, name, value, kvstore, route_params)

    def postinit(self):
        self.check_internal_fields()

        self.config_type = self._internal_fields.get('_config_type', TConfigTemplateNode.EType.MMETA)
        self.filename = self._internal_fields.get('_filename', None)
        self.yplookup = self._internal_fields.get('_yplookup')
        if self.yplookup:
            self.yplookup = CURDB.yplookups.get_yplookup_by_name(self.yplookup)

        if self.anchor is None:
            if '_instances' in self._internal_fields:
                self.anchor = self._internal_fields['_instances']
            elif '_filename' in self._internal_fields:
                self.anchor = self._internal_fields['_filename']
            elif self.yplookup:
                different_anchors = set([yplookup.name for yplookup in self.yplookup])
                assert len(different_anchors) == 1
                self.anchor = list(different_anchors)[0]
            else:
                raise Exception("Specify _a, _instances or _filename option in config at path <%s>" % self.path)

        if self._internal_fields.get('_instances', None) is None and not self.filename and not self.yplookup:
            raise Exception("Not specified <_instances> or <_filename> or <_yplookup> at path <%s>" % self.path)

    def extra_route_params(self):
        return {
            'config_type': self.config_type,  # SourceTemplateNode is built based on this thing
        }

    def generate_config_nodes(self):
        """Generate bunch of configs from current template (one config per instance)"""

        self.instances = self._load_instances(self._internal_fields.get('_instances'))
        result = [TConfigNode(self, instances, fnames) for instances, fnames in self.instances]

        if self.filename is not None:
            for fname in self.filename.split(','):
                result.append(TConfigNode(self, None if len(self.instances) == 0 else self.instances[0][0], [fname]))

        return result

    def _load_instances(self, expression):
        """
            Calculate list of instances configs needed for from simple expression.
            Return list of pairs: (instances, list of filenames).
        """
        if self.yplookup:
            result = list()
            for yplookup in self.yplookup:
                if self.config_type == TConfigTemplateNode.EType.INTL2:
                    layer = yplookup.intl2
                elif self.config_type == TConfigTemplateNode.EType.INT:
                    layer = yplookup.intl1
                else:
                    layer = yplookup.base
                result.extend([([source], [source.config_filename]) for source in layer.sources])
            return result

        if expression is None:
            return []

        result = []
        if self.config_type == TConfigTemplateNode.EType.MMETA:
            groups_or_intlookups = [s.strip() for s in expression.split('+')]
            for groupname in groups_or_intlookups:
                if not CURDB.groups.has_group(groupname):
                    raise Exception("Group <%s> not found while parsing <%s> (at path <%s>)" %
                                    (groupname, expression, self.path))
                group = CURDB.groups.get_group(groupname)
                instances = group.get_kinda_busy_instances()
                result.append((instances, [generate_fname(x, use_short_hostname(group)) for x in instances]))
        else:
            intlookup = calculate_intlookup_expr(expression)
            if self.config_type == TConfigTemplateNode.EType.INTL2:
                for intl2_group in intlookup.intl2_groups:
                    result.append((intl2_group.intl2searchers,
                                   [generate_fname(x, use_short_hostname(CURDB.groups.get_group(x.type)))
                                    for x in intl2_group.intl2searchers]))
            elif self.config_type == TConfigTemplateNode.EType.INT:
                for int_group in intlookup.get_int_groups():
                    if len(int_group.intsearchers) > 0:
                        result.append((int_group.intsearchers,
                                       [generate_fname(x, use_short_hostname(CURDB.groups.get_group(x.type)))
                                        for x in int_group.intsearchers]))
            else:
                raise Exception("Unknown config type {}".format(self.config_type))

        return result

    def render(self, strict=True):
        raise NotImplementedError(
            "Node <%s> is a meta-node (should be converted to other nodes during preparation). "
            "This method should not be called at all" % self.__class__)


def generate_fname(instance, short_name=True):
    modified_instance = may_be_guest_instance(instance)
    hostname = modified_instance.host.name.partition('.')[0] if short_name else modified_instance.host.name
    return '{}:{}.cfg'.format(hostname, modified_instance.port)


def use_short_hostname(group):
    return not group.card.properties.mtn.use_mtn_in_config


class TConfigNode(TSimpleNode):
    """
        Config node. Created from template node
    """

    INTERNAL_KEYS = TSimpleNode.INTERNAL_KEYS

    __slots__ = ['instances', 'filenames']

    def __init__(self, template_node, instances, filenames):
        super(TSimpleNode, self).__init__(template_node.path, template_node.name, template_node.value, [], dict())

        self._internal_fields = template_node._internal_fields.copy()
        for key in ['_config_type', '_instances', '_filename']:
            self._internal_fields.pop(key, None)
        self._data_fields = copy.copy(template_node._data_fields)

        self.children = []
        for child in template_node.children:
            if instances is not None:
                if len(instances) == 0:
                    raise Exception("Not found instances of type <%s> in <%s> (you might forgot to add such instances to intlookup)" % (
                        template_node._internal_fields['_config_type'], template_node._internal_fields['_instances']))
                coordinator_instance = instances[0]
            else:
                coordinator_instance = None
            child_copy = child.copy({'coordinator_instance': coordinator_instance})
            if isinstance(child_copy, list):
                self.children.extend(child_copy)
            else:
                self.children.append(child_copy)

        self.instances = instances
        self.filenames = filenames

        if self.instances is not None:
            self.anchor = self.filenames[0]  # TODO NB!!
            # self.anchor = self.instances[0].name()
        else:
            self.anchor = self.filenames[0]

    def postinit(self):
        self.check_internal_fields()

        TSimpleNode.postinit(self)

    def render(self, strict=True):
        return "\n".join(map(lambda x: x.render(strict=strict), self.children))

    def render_separate(self):
        rendered = self.render(strict=True)
        return map(lambda x: "# %s\n%s" % (x, rendered), self.filenames)
