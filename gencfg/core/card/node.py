#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'contrib')))
sys.path.append(os.path.abspath(os.path.dirname(__file__)))

import re
import copy
from collections import OrderedDict
import traceback
import simplejson

# TODO: use LibYaml to speed up PyYaml (for more details read PyYaml documentation)
from pkg_resources import require

require('PyYAML')
import yaml

import ordered_dict_yaml

from gaux.aux_utils import raise_extended_exception, plain_obj_unicode_to_str
import core.card.types as card_types
from core.exceptions import TValidateCardNodeError


# TODO: safe eval
# TODO: replace get_private_copy() with deepcopy()

# ================================== CardNode ==================================

class ECardFileProto(object):
    YAML = 'yaml'  # default protocol for card nodes
    JSON = 'json'  # faster protocotl for card nodes
    ALL = [YAML, JSON]


def split_node_description(text):
    items = []
    quotes = 0
    cur_item = ''
    for symbol in text + ';':
        if symbol == '"':
            quotes += 1
            cur_item += symbol
        elif symbol == ';':
            if quotes % 2 == 0:
                items.append(cur_item.strip(' '))
                cur_item = ''
            else:
                cur_item += symbol
        else:
            cur_item += symbol

    return items


class ResolveCardPathError(Exception):
    pass


class TMdDoc(object):
    """
        Class load markdown formad doc from doc file and construct html documentation for requested section. Example of file content:
== file1.yaml:path.to.somehting ==
some markdown text
== file2.yaml:path.to.some.else ==
some other markdown text
    """

    class EFormat(object):
        MD = 0
        HTML = 1

    # make class singleton
    _instance = None

    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super(TMdDoc, cls).__new__(cls, *args)
        return cls._instance

    def __init__(self, fname):
        import markdown

        p = re.compile("^== (.*) ==$", re.MULTILINE)

        self.mddoc = OrderedDict()
        self.htmldoc = OrderedDict()

        cur_section = None
        cur_doc = []
        for line in open(fname).readlines():
            m = p.match(line)
            if m:
                if cur_section is not None:
                    assert (cur_section not in self.mddoc), "Section %s mentioned twice in documentation"
                    self.mddoc[cur_section] = "\n".join(cur_doc)
                    self.htmldoc[cur_section] = markdown.markdown(unicode(self.mddoc[cur_section], 'utf8')).encode('utf8')
                cur_section = (m.group(1).partition(':')[0].strip(), m.group(1).partition(':')[2].strip())
                cur_doc = []
            else:
                cur_doc.append(line)
        assert (cur_section not in self.mddoc), "Section %s mentioned twice in documentation"
        self.mddoc[cur_section] = "\n".join(cur_doc)
        self.htmldoc[cur_section] = markdown.markdown(unicode(self.mddoc[cur_section], 'utf8')).encode('utf8')

    def get_doc(self, fname, path, fmt):
        if fmt == TMdDoc.EFormat.MD:
            return self.mddoc.get((fname, path), None)
        elif fmt == TMdDoc.EFormat.HTML:
            return self.htmldoc.get((fname, path), None)
        else:
            raise Exception("Unknown format <%s> for TMdDoc" % fmt)


class LeafExtendedInfo(object):
    def __init__(self):
        self.path = None
        self.value = None
        self.types = None
        self.type_str = None
        self.display_name = None
        self.description = None
        self.readonly = False

        self.has_default_value = False
        self.default_value = None

        self.is_inherited = False
        self.inherited_value = None

        # means this field is has not default or inherited value
        self.is_interesting = None


class NodeExtendedInfo(object):
    def __init__(self):
        self.path = None
        self.display_name = None
        self.description = None
        self.is_interesting = None


class CardNode(object):
    # provides dict-like interface and class interface
    def __init__(self):
        pass

    @staticmethod
    def create_from_dict(d):
        if isinstance(d, list):
            return [CardNode.create_from_dict(x) for x in d]
        elif isinstance(d, dict):
            node = CardNode()
            for k, v in d.iteritems():
                node[k] = CardNode.create_from_dict(v)
            return node
        else:
            return d

    @staticmethod
    def load_json(filename, scheme):
        from core.card.json_utils import fix_card_node_json_for_load, add_defaults_for_load

        with open(filename) as f:
            jsoned = simplejson.loads(f.read())
        jsoned = plain_obj_unicode_to_str(jsoned)

        add_defaults_for_load(scheme, jsoned)
        fix_card_node_json_for_load(scheme, jsoned)

        return CardNode.create_from_dict(jsoned)

    def custom_str(self, filtered):
        opts = CardWriter.Opts()
        opts.filtered = filtered
        return CardWriter(opts)

    def add_field(self, key, value):
        self[key] = value

    def add_node(self, key, node=None):
        self[key] = node if node is not None else CardNode()

    def save_extended_info(self, scheme):
        opts = CardWriterExt.Opts()
        opts.extended_info = True
        opts.write_slaves = False
        result = CardWriterExt(scheme, opts).process(self)
        return result

    def save_to_obj(self, scheme):
        return CardWriterExt(scheme).process(self)

    def save_to_str(self, scheme):
        try:
            obj = self.save_to_obj(scheme)
            result = CardNodeTextRender().process(obj)
        except Exception as e:
            msg = 'An error occured while converting to text group card for group %s:' % self.name
            raise_extended_exception(e, msg)
        else:
            return result

    def save_to_file(self, scheme, filename, proto=ECardFileProto.YAML):
        try:
            if proto == ECardFileProto.YAML:
                obj = self.save_to_obj(scheme)
                with open(filename, 'w') as f:
                    print >> f, CardNodeTextRender().process(obj)
                    f.close()
            elif proto == ECardFileProto.JSON:
                from core.card.json_utils import fix_card_node_json_for_save, remove_defaults_for_save

                jsoned = self.as_dict()
                fix_card_node_json_for_save(scheme, jsoned)
                remove_defaults_for_save(scheme, jsoned)

                slaves_jsoned = []
                for slave in jsoned['slaves']:
                    slave_jsoned = slave.card.as_dict()
                    fix_card_node_json_for_save(scheme, slave_jsoned)
                    remove_defaults_for_save(scheme, slave_jsoned)
                    slaves_jsoned.append(slave_jsoned)
                jsoned['slaves'] = slaves_jsoned

                with open(filename, 'w') as f:
                    print >> f, simplejson.dumps(jsoned, indent=4, sort_keys=True)
        except Exception as e:
            msg = 'An error occured while saving card for group %s to file:' % self.name
            raise_extended_exception(e, msg)

    def load_from_file(self, scheme, filename, proto=ECardFileProto.YAML):
        def custom_str_constructor(loader, node):
            return loader.construct_scalar(node).encode('utf8')

        try:
            if proto == ECardFileProto.YAML:
                yaml.add_constructor(u'tag:yaml.org,2002:str', custom_str_constructor)
                result = CardReaderExt(scheme).process_file(filename)
                self.replace_self(result)
            elif proto == ECardFileProto.JSON:
                from core.card.json_utils import fix_card_node_json_for_load, add_defaults_for_load

                jsoned = simplejson.loads(open(filename).read())
                jsoned = plain_obj_unicode_to_str(jsoned)

                add_defaults_for_load(scheme, jsoned)
                fix_card_node_json_for_load(scheme, jsoned)

                for slave in jsoned['slaves']:
                    add_defaults_for_load(scheme, slave)
                    fix_card_node_json_for_load(scheme, slave)

                self.replace_self(CardNode.create_from_dict(jsoned))
        except Exception as e:
            msg = 'An error occured while loading group card from file %s:' % filename
            raise_extended_exception(e, msg)

    def replace_self(self, other):
        children = list(self)
        for child in children:
            del self[child]
        for child in other:
            self[child] = other[child]

    def _resolve_type_and_get_value(self, path, value, scheme):
        scheme_node = scheme.get_cached()
        for edge in path:
            if not isinstance(scheme_node, SchemeNode) or edge not in scheme_node:
                raise Exception('No such path %s in scheme' % '->'.join(path))
            scheme_node = scheme_node[edge]
        if not isinstance(scheme_node, SchemeLeaf):
            raise Exception('Path %s leads to node, not leaf!' % '->'.join(path))

        validate_info = scheme_node.card_type.validate(value)
        return validate_info.status, validate_info.value

    def check_card_value(self, path, value, scheme):
        success, _ = self._resolve_type_and_get_value(path, value, scheme)
        return success

    def set_card_value(self, path, value, scheme):
        try:
            node = self.resolve_card_path(path[:-1])
        except ResolveCardPathError:
            raise Exception('Could not set group %s card value: invalid path %s' % (self.name, '->'.join(path)))

        status, parsed_value = self._resolve_type_and_get_value(path, value, scheme)
        if status == card_types.EStatuses.STATUS_FAIL:
            raise Exception('Could not set group %s card value: invalid value "%s" for key %s' % (self.name, value, '->'.join(path)))

        node[path[-1]] = parsed_value

    def get_card_value(self, path):
        try:
            result = self.resolve_card_path(path)
        except ResolveCardPathError:
            raise Exception('Could not get group %s card value: invalid path %s' % (self.name, '->'.join(path)))
        return result

    def resolve_card_path(self, path):
        node = self
        for edge in path:
            if not isinstance(node, CardNode) or edge not in node:
                raise ResolveCardPathError, "Group %s: not found edge <%s> in path <%s>" % (
                    self.name, edge, ",".join(path))
            node = node[edge]
        return node

    # ** dict functions
    def __contains__(self, item):
        return item in self.__dict__

    def __getitem__(self, item):
        return self.__dict__[item]

    def __setitem__(self, key, value):
        self.__dict__[key] = value

    def __delitem__(self, key):
        del self.__dict__[key]

    def __iter__(self):
        return self.__dict__.__iter__()

    def __deepcopy__(self, memo):
        result = CardNode()
        for key in self:
            result[key] = copy.deepcopy(self[key], memo)
        return result

    def get(self, key, default=None):
        return self.__dict__.get(key, default)

    def as_dict(self):
        result = OrderedDict()
        for k, v in self.__dict__.iteritems():
            if isinstance(v, CardNode):
                v = v.as_dict()
            elif isinstance(v, list):
                v = map(lambda x: x.as_dict() if isinstance(x, CardNode) else x, v)
            result[k] = v

        return result

    def items(self):
        return self.__dict__.items()

        # def get(self, key, default):
        #    if key not in self:
        #        return default
        #    return self[key]


# ================================== Scheme ==================================

class SchemeLeaf(object):
    def __init__(self):
        self.name = None
        self.has_default_value = None
        self.default = None
        self.inherit = None
        self.types = None
        self.card_type = None
        self.make_copy = None
        self.description = None
        self.readonly = False
        self.choices_func = None
        self.display_name = None
        self.always_interesting = False

    def get_types(self):
        return self.types

    def has_default(self):
        return self.has_default_value

    def get_default(self):
        assert self.has_default_value
        if self.make_copy:
            return copy.deepcopy(self.default)
        else:
            return self.default

    def is_inherited(self):
        return self.inherit

    def set_default(self, value, make_copy):
        self.has_default_value = True
        self.default = value
        self.make_copy = make_copy

    def get_description(self):
        return self.description

    def is_readonly(self):
        return self.readonly

    def get_display_name(self):
        return self.display_name

    def is_always_interesting(self):
        return self.always_interesting

    def check(self):
        if self.display_name is None:
            raise Exception('Leaf %s has empty display name' % self.name)


class SchemeNode(OrderedDict):
    def __init__(self, children):
        OrderedDict.__init__(self)
        for key, value in children.items():
            self[key] = value
        self.none_by_default = False  # seems to be deprecated
        self.name = None
        self.display_name = None
        self.description = None

    def is_list_node(self):
        return '_list' in self

    def is_none_by_default(self):
        return self.none_by_default

    def get_description(self):
        return self.description

    def get_display_name(self):
        return self.display_name

    def resolve_scheme_path(self, path):
        """
            Traverse tree

            :param path: tuple of nodes like ('properties', 'expires')
        """
        node = self
        for edge in path:
            if not isinstance(node, (SchemeNode, SchemeLeaf)) or edge not in node:
                raise ResolveCardPathError, "Scheme %s: not found edge <%s> in path <%s>" % (
                    self.name, edge, ",".join(path))
            node = node[edge]
        return node

    def check(self):
        if self.display_name is None:
            raise Exception('Node %s has empty display name' % self.name)


class Scheme(object):
    # shared scheme
    _scheme = None
    node_key = '_node'
    list_key = '_list'
    slaves_node = 'slaves'
    scheme_node = 'scheme'
    internal_keys = [node_key, slaves_node, scheme_node]
    special_values = {'empty set': set(), 'None': None, 'True': True, 'False': False}

    def __init__(self, scheme_file, dbversion, subpath=None, md_doc_file=None):
        self.dbversion = dbversion
        self.scheme_file = scheme_file
        self.subpath = subpath  # needed if we want to create scheme from part of yaml file

        try:
            self._yamled_scheme = yaml.load(open(self.scheme_file).read())
        except Exception, e:
            raise

        self._yamled_scheme = plain_obj_unicode_to_str(self._yamled_scheme)

        self._md_doc = None

        self._scheme = self.load_from_yaml()

    def get_cached(self):
        return self._scheme

    def get_noncached(self):
        return self.load_from_yaml()

    def load_from_yaml(self):
        scheme = self._yamled_scheme
        if self.subpath is not None:
            for k in self.subpath:
                scheme = scheme[k]

        # make sure, new scheme has nothing common with self._yamled_scheme (does not contain any changable fields from _yamlded_scheme)
        scheme = self._build_node('<root>', [], scheme)
        assert (isinstance(scheme, SchemeNode))
        return scheme

    def reset(self):
        self._scheme = None

    def _build_node(self, node_name, path, src):
        children = OrderedDict()
        for key, value in src.items():
            if key in Scheme.internal_keys:
                continue
            if isinstance(value, OrderedDict):
                children[key] = self._build_node(key, path + [key], value)
            else:
                children[key] = self._build_leaf(key, path + [key], value)

        result = SchemeNode(children)
        # TODO: save path, not only name
        result.name = node_name

        text = src[Scheme.node_key]

        items = split_node_description(text)

        for item in items:
            if item.startswith('displayed as'):
                Scheme.node_parse_display_name(item, result)
            else:
                Scheme.node_parse_description(item, result)

        if result.description is None and self._md_doc is not None:
            result.description = self._md_doc.get_doc(os.path.basename(self.scheme_file), ".".join(path),
                                                      TMdDoc.EFormat.HTML)

        if self.dbversion >= '0.6':
            result.check()
        return result

    def _build_leaf(self, leaf_name, path, text):
        if not isinstance(text, str):
            f = open('output.txt', 'w')
            print >> f, str(type(text))
            print >> f, text
            f.close()
        assert (isinstance(text, str)), "Leaf <%s> should have type <%s> while having <%s>" % (
            leaf_name, type(text), str)

        text = text.strip()
        items = split_node_description(text)
        result = SchemeLeaf()
        # TODO: save path, not only name
        result.name = leaf_name

        parsed_type = False
        parsed_default_value = False
        for item in items:
            if item.endswith('type'):
                Scheme.leaf_parse_type(item, result)
                parsed_type = True
            elif item.endswith('by default') or item == 'no default value':
                Scheme.leaf_parse_default_value(item, result)
                parsed_default_value = True
            elif item.endswith('inherited'):
                Scheme.leaf_parse_inheritance(item, result)
            elif item.endswith('readonly'):
                Scheme.leaf_parse_readonly(item, result)
            elif item.endswith('choice'):
                Scheme.leaf_parse_choice(item, result)
            elif item.startswith('displayed as'):
                Scheme.leaf_parse_display_name(item, result)
            elif item.strip() == "always displayed":
                result.always_interesting = True
            else:
                Scheme.leaf_parse_description(item, result)

        if result.description is None and self._md_doc is not None:
            result.description = self._md_doc.get_doc(os.path.basename(self.scheme_file), ".".join(path),
                                                      TMdDoc.EFormat.HTML)

        if not parsed_type:
            raise Exception('No type given for the leaf "%s"!' % leaf_name)
        if not parsed_default_value:
            raise Exception('No default value given for the leaf "%s"!' % leaf_name)

        result.make_copy = True

        if self.dbversion >= '0.6':
            result.check()

        return result

    @staticmethod
    def node_parse_display_name(item, node):
        assert (item.startswith('displayed as'))
        node.display_name = item[len('displayed as'):].strip()

    @staticmethod
    def node_parse_description(item, node):
        node.description = item

    @staticmethod
    def leaf_parse_type(item, leaf):
        assert (item.endswith(' type'))

        tokens = item.split()[:-1]

        """
            Common format:  [list of] [None or] [positive|nonnegative] <basic_type>
            Examples:
                bool
                positive float
                list of string
                list of None or nonnegative int
        """

        # process basic type
        BASIC_TYPES_MAPPING = {
            ('int',): (card_types.IntType, 'int'),
            ('float',): (card_types.FloatType, 'float'),
            ('string',): (card_types.StringType, 'string'),
            ('bool',): (card_types.BoolType, 'bool'),
            ('byte', 'size'): (card_types.ByteSizeType, 'byte size'),
            ('function',): (card_types.FunctionType, 'function'),
            ('date',): (card_types.DateType, 'date'),
            ('GroupOwner',): (card_types.GroupOwnerType, 'login'),
            ('User',): (card_types.UserType, 'login'),
            ('Metaprj',): (card_types.MetaprjType, 'string'),
            ('Ctype',): (card_types.CtypeType, 'string'),
            ('Itype',): (card_types.ItypeType, 'string'),
            ('Prj',): (card_types.PrjType, 'string'),
            ('InstancePortFunc',): (card_types.InstancePortFuncType, 'string'),
            ('Tier',): (card_types.TierType, 'string'),
            ('ReqsHostsMaxPerSwitch',): (card_types.ReqsHostsMaxPerSwitch, 'int'),
            ('ReqsHostsHaveIpv6Addr',): (card_types.ReqsHostsHaveIpv6Addr, 'bool'),
            ('ReqsHostsHaveIpv4Addr',): (card_types.ReqsHostsHaveIpv4Addr, 'bool'),
            ('ReqsHostsNetcardRegexp',): (card_types.ReqsHostsNetcardRegexp, 'string'),
            ('ReqsHostsNdisks',): (card_types.ReqsHostsNdisks, 'int'),
            ('HbfMacros',): (card_types.HbfMacrosType, 'hbf_macros'),
        }
        found_base_type = False
        for k, (tp, type_str) in BASIC_TYPES_MAPPING.iteritems():
            if tokens[-len(k):] == list(k):
                card_type = tp()
                leaf.type_str = ' '.join(tokens[:-len(k)] + [type_str])
                tokens = tokens[:-len(k)]
                leaf.types = [leaf.type_str]
                found_base_type = True
        if not found_base_type:
            raise Exception("Unknown basic type <%s>" % tokens[-1])

        # drop unused stuff: function or <basic_type>
        if len(tokens) > 0 and tokens[-2:] == ['function', 'or']:
            tokens.pop()
            tokens.pop()

        # process [(positive|negative)]
        if len(tokens) > 0 and tokens[-1] in ['positive', 'nonnegative']:
            attr = tokens.pop()
            if attr == 'positive':
                card_type = card_types.PositiveType(card_type)
            elif attr == 'nonnegative':
                card_type = card_types.NonNegativeType(card_type)
            else:
                raise Exception("OOPS")

        # process [None or]
        if len(tokens) > 0 and tokens[-2:] == ['None', 'or']:
            tokens.pop()
            tokens.pop()
            card_type = card_types.NoneOrType(card_type)

        # process [list of]
        if len(tokens) > 0 and tokens[-2:] == ['list', 'of']:
            tokens.pop()
            tokens.pop()
            card_type = card_types.ListOfType(card_type)

        if len(tokens) > 0:
            raise Exception("Found unparsed tokens <%s> while parsing leaf type" % tokens)

        leaf.card_type = card_type

    @staticmethod
    def leaf_type_to_simple_form(leaf):
        types_map = {
            'None or date': dict(type='date', optional=True, is_list=False),
            'None or function': dict(type='string', optional=True, is_list=False),
            'None or string': dict(type='string', optional=True, is_list=False),
            'None or nonnegative int': dict(type='nonnegative int', optional=True, is_list=False),
            'None or hbf_macros': dict(type='hbf_macros', optional=True, is_list=False),
            'bool': dict(type='bool', optional=False, is_list=False),
            'byte size': dict(type='byte size', optional=False, is_list=False),
            'function or string': dict(type='string', optional=False, is_list=False),
            'list of string': dict(type='string', optional=False, is_list=True),
            'nonnegative int': dict(type='nonnegative int', optional=False, is_list=False),
            'positive int': dict(type='positive int', optional=False, is_list=False),
            'string': dict(type='string', optional=False, is_list=False),
            'int': dict(type='int', optional=False, is_list=False),
            'float': dict(type='float', optional=False, is_list=False),
            'None or float': dict(type='float', optional=True, is_list=False),
            'None or int': dict(type='int', optional=True, is_list=False),
            'list of login': dict(type='login', optional=False, is_list=True),
            'list of int': dict(type='int', optional=False, is_list=True),
            'hbf_macros': dict(type='hbf_macros', optional=False, is_list=False),
        }
        assert leaf.type_str in types_map, "Type '%s' is missing in internal table" % leaf.type_str
        return types_map[leaf.type_str]

    @staticmethod
    def leaf_parse_default_value(item, leaf):
        assert (item.endswith('by default') or item == 'no default value')
        if item == 'no default value':
            leaf.has_default_value = False
            leaf.default = None
        else:
            assert (item.endswith('by default'))
            default = item[:-len('by default')].strip()
            if default in Scheme.special_values:
                default = Scheme.special_values[default]
            else:
                default = yaml.load(default)
            if leaf.types is None:
                raise Exception('Default value should be given after type in leaf "%s"' % leaf.name)

            validate_info = leaf.card_type.validate(default)
            if validate_info.status == card_types.EStatuses.STATUS_FAIL:
                raise Exception(validate_info.reason)
            default = validate_info.value

            # CardReader.parse_leaf_value(default, leaf.types, ['scheme', '...'])
            leaf.default = default
            leaf.has_default_value = True

    @staticmethod
    def leaf_parse_description(item, leaf):
        leaf.description = item

    @staticmethod
    def leaf_parse_inheritance(item, leaf):
        assert (item in ['inherited', 'not inherited'])
        leaf.inherit = item == 'inherited'

    @staticmethod
    def leaf_parse_readonly(item, leaf):
        assert (item.endswith('readonly'))
        leaf.readonly = True

    @staticmethod
    def leaf_parse_choice(item, leaf):
        assert (item.endswith('choice'))
        leaf.choices_func = item[:-len('choice')].strip()

    @staticmethod
    def leaf_parse_display_name(item, leaf):
        assert (item.startswith('displayed as'))
        leaf.display_name = item[len('displayed as'):].strip()


# ================================== Types ==================================

class Percentage(object):
    def __init__(self, text, value):
        self.fraction = value
        self.text = text

    def __str__(self):
        return self.text

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.fraction == other.fraction


# ================================== CardReader ==================================

class CardReader(object):
    # Converts data structure to CardNode object

    # Main functions:
    #   - validate using scheme
    #   - insert default nodes/values
    def __init__(self, scheme):
        self.scheme = scheme

    #        self.scheme = scheme if scheme is not None else Scheme.get()

    def process(self, obj, base_path=None):
        path = copy.copy(base_path) if base_path is not None else []
        return self._process_node(obj, self.scheme, path)

    def _process_node(self, obj, scheme, path):
        if isinstance(obj, dict):
            for field in obj:
                if field not in scheme:
                    raise Exception('Undefined field "%s" in "%s"' % (field, CardReader._path_to_str(path)))

            result = CardNode()
            for field in scheme:
                if field in Scheme.internal_keys:
                    continue

                if field not in obj:
                    path.append(field)
                    if isinstance(scheme[field], SchemeNode):
                        if scheme[field].is_list_node():
                            result[field] = []
                        else:
                            result[field] = self._default_node(scheme[field], path)
                    else:
                        result[field] = self._default_leaf(scheme[field], path)
                    path.pop()
                    continue

                assert (field in obj)
                if (isinstance(scheme[field], SchemeLeaf) and isinstance(obj[field], dict)) or \
                        (isinstance(scheme[field], SchemeNode) and not isinstance(obj[field], (dict, list))):
                    raise Exception('Mismatch type of value "%s" in "%s"' % (field, CardReader._path_to_str(path)))
                path.append(field)
                if isinstance(scheme[field], SchemeNode):
                    result[field] = self._process_node(obj[field], scheme[field], path)
                else:
                    result[field] = self._process_leaf(obj[field], scheme[field], path)
                path.pop()
        elif isinstance(obj, list):
            if '_list' not in scheme:
                raise Exception("Not key _list in list branch")
            result = []
            for obj_elem in obj:
                path.append('[]')
                result.append(self._process_node(obj_elem, scheme['_list'], path))
                path.pop()
            return result
        else:
            raise Exception("Unknown type <%s> in path %s" % (type(obj), '/'.join(path)))

        return result

    def _process_leaf(self, obj, scheme, path):
        assert (isinstance(scheme, SchemeLeaf))

        validate_info = scheme.card_type.validate(obj)

        if validate_info.status == card_types.EStatuses.STATUS_FAIL:
            raise Exception('Wrong value "%s" of "%s" in "%s", should be %s (error message <%s>)' % \
                            (obj, path[-1], CardReader._path_to_str(path[:-1]), ' or '.join(scheme.types),
                             validate_info.reason))

        return validate_info.value

    #        return CardReader.parse_leaf_value(obj, scheme.get_types(), path)

    #    @classmethod
    #    def parse_leaf_value(cls, obj, types, path):
    #        success, value = TypeChecker.resolve_type_and_get_value(obj, types)
    #        if not success:
    #            raise Exception('Wrong value "%s" of "%s" in "%s", should be %s' % \
    #                        (obj, path[-1], CardReader._path_to_str(path[:-1]), ' or '.join(types)))
    #        return value

    def _default_node(self, scheme, path):
        assert (isinstance(scheme, SchemeNode))
        if scheme.is_none_by_default():
            return None
        else:
            result = CardNode()
            for field in scheme:
                path.append(field)
                if isinstance(scheme[field], SchemeNode):
                    if scheme[field].is_list_node():
                        result[field] = []
                    else:
                        result[field] = self._default_node(scheme[field], path)
                else:
                    result[field] = self._default_leaf(scheme[field], path)
                path.pop()
            return result

    def _default_leaf(self, scheme, path):
        assert (isinstance(scheme, SchemeLeaf))
        if not scheme.has_default():
            raise Exception('Value "%s" is missing in "%s"; no default value in scheme' % \
                            (path[-1], CardReader._path_to_str(path[:-1])))
        return scheme.get_default()

    @classmethod
    def _path_to_str(cls, path):
        return '->'.join(['root'] + path)


# ================================== CardWriter ==================================

class CardWriter(object):
    class Opts(object):
        def __init__(self):
            # filter out non-scheme nodes
            self.filtered = False
            # remove default values
            self.remove_defaults = False
            # scheme (can be parent scheme)
            # None means using default scheme
            self.scheme = None
            # put extended leaf information
            self.extended_info = False

    # Class functions:
    #   - preserve scheme-given order of fields converting to OrderedDict
    #   - filter out default/inherited values that should not be presented in text

    def __init__(self, opts=None):
        if opts is None:
            opts = CardWriter.Opts()
        self.opts = opts
        self.scheme = opts.scheme

    def process(self, obj):
        def_mask = self._build_def_mask(obj, self.scheme)

        if def_mask is True:
            return

        if not self.opts.extended_info:
            use_def_mask = self.opts.remove_defaults
            result = self._process_node(obj, self.scheme, def_mask if use_def_mask else None)
        else:
            result = self._process_node(obj, self.scheme, None)
            output = []
            self._extended_info(result, self.scheme, def_mask, [], output)
            result = output

        return result

    def _process_node(self, obj, scheme, def_mask):
        assert (isinstance(obj, CardNode) or isinstance(obj, list) or obj is None)
        assert (def_mask is None or isinstance(def_mask, (dict, list)))

        result = OrderedDict()

        if scheme is not None:
            if scheme.is_list_node():
                assert (isinstance(obj, list) or obj is None)
                result = []
                for i in range(len(obj)):
                    result.append(self._process_node(obj[i], scheme['_list'], def_mask[i] if def_mask else None))
            else:
                for key in scheme:
                    if key in Scheme.internal_keys:
                        continue

                    if def_mask is not None and def_mask[key] is True:
                        continue
                    elif isinstance(scheme[key], SchemeNode):
                        assert (isinstance(obj[key], (CardNode, list)) or obj[key] is None)

                        if obj[key] is None:
                            continue
                        assert (isinstance(key, str))
                        result[key] = self._process_node(obj[key], scheme[key], def_mask[key] if def_mask else None)
                    elif isinstance(scheme[key], SchemeLeaf):
                        assert (key in obj)
                        validate_info = scheme[key].card_type.validate(obj[key])
                        if validate_info.status == card_types.EStatuses.STATUS_OK:
                            result[key] = validate_info.value
                        else:
                            raise TValidateCardNodeError(validate_info.reason)
                    else:
                        raise Exception("Can not process node of type <%s> in CardWriter" % scheme[key].__class__)

        if not self.opts.filtered:
            if scheme.is_list_node():
                assert (isinstance(obj, list) or obj is None)
                # do nothing
            else:
                for key in obj:
                    if scheme is not None and key in scheme:
                        # has already worked out
                        continue

                    if def_mask is not None and def_mask[key] is True:
                        continue

                    if isinstance(obj[key], CardNode):
                        result[key] = self._process_node(obj[key], None, def_mask[key] if def_mask else None)
                    else:
                        result[key] = obj[key]

        return result

    def _build_def_mask(self, obj, scheme):
        # def mask is a multi-level dictionary (dict of dict of dict ...)
        # if def_mask[x] == True then obj[x] has a default value (obj[x] can Node or Leaf)
        # if def_mask[x] == False then obj[x] has not a default value (obj[x] is Leaf)
        # else def_mask[x] is dict
        if scheme.is_list_node():
            assert (isinstance(obj, list))
            if len(obj) == 0:
                return True
            else:
                result = []
                for elem in obj:
                    result.append(self._build_def_mask(elem, scheme['_list']))
                return result
        else:
            result = {}
            for field in obj:
                if field not in scheme:
                    result[field] = None
                    continue

                if isinstance(scheme[field], SchemeNode):
                    if obj[field] is None:
                        assert (scheme[field].is_none_by_default())
                        result[field] = True
                    else:
                        result[field] = self._build_def_mask(obj[field], scheme[field])
                else:
                    assert (isinstance(scheme[field], SchemeLeaf))
                    result[field] = scheme[field].has_default() and scheme[field].get_default() == obj[field]

            for field in scheme:
                if field not in obj:
                    # assume that object is validated with scheme
                    assert False, "Field <%s> found in scheme, but not found in obj %s (found keys %s)" % (
                        field, obj, ",".join(map(lambda (x, y): x, obj.items())))

            if all([x is True for x in result.values()]):
                # if all children are defaults => then mark all node as default
                return True
            else:
                return result

    def _extended_info(self, obj, scheme, def_mask, path, result):
        node = NodeExtendedInfo()
        node.path = copy.copy(path)
        node.display_name = scheme.get_display_name()
        node.description = scheme.get_description()
        node.is_interesting = def_mask is not True
        result.append(node)

        for field in obj:
            assert (field in scheme)
            if isinstance(obj[field], OrderedDict) or (
                        isinstance(scheme[field], SchemeNode) and scheme[field].is_list_node()):

                next_def_mask = None
                if def_mask is True:
                    next_def_mask = True
                elif def_mask and field in def_mask:
                    next_def_mask = def_mask[field]

                path.append(field)
                if isinstance(scheme[field], SchemeNode) and scheme[field].is_list_node():
                    # FIXME: currently frontend does not support list fields, so omit such things
                    pass
                # assert(isinstance(obj[field], list))
                #                    for elem in obj[field]:
                #                        self._extended_info(elem, scheme[field]['_list'], next_def_mask, path, result)
                else:
                    self._extended_info(obj[field], scheme[field], next_def_mask, path, result)
                path.pop()
            else:

                is_interesting = True
                if not scheme[field].is_always_interesting():
                    if def_mask is True:
                        is_interesting = False
                    elif def_mask and field in def_mask:
                        is_interesting = not def_mask[field]

                assert (is_interesting in [True, False])
                assert (field in scheme)
                assert (not isinstance(obj[field], OrderedDict))

                leaf = LeafExtendedInfo()
                leaf.path = path + [field]
                leaf.value = obj[field]
                leaf.types = scheme[field].get_types()
                leaf.card_type = scheme[field].card_type
                leaf.type_str = scheme[field].type_str
                leaf.readonly = scheme[field].is_readonly()
                leaf.description = scheme[field].get_description()
                leaf.is_inherited = scheme[field].is_inherited()
                leaf.has_default = scheme[field].has_default()
                leaf.default_value = scheme[field].get_default() if leaf.has_default else None
                leaf.choices_func = scheme[field].choices_func
                leaf.display_name = scheme[field].get_display_name()
                leaf.is_interesting = is_interesting
                result.append(leaf)


# ================================== CardNodeTextRender ==================================

class CardNodeTextRender(object):
    # Main function is to convert CardWriter result to valid nice looking yaml text
    # OrderedDict is converted in yaml dict, order is preserved from original OrderedDict
    indent_str = ' ' * 4

    def __init__(self):
        pass

    def process(self, root):
        #        assert(isinstance(root, OrderedDict))
        self.lines = []
        self.indent = 0
        self._process_node(root)
        assert (not self.indent)
        return '\n'.join(self.lines)

    def _append(self, line):
        assert (isinstance(line, str))
        self.lines.append(self.indent * self.indent_str + line)

    def _is_leaf(self, node):
        if isinstance(node, OrderedDict):
            return not bool(node)
        if isinstance(node, list):
            # simple check
            # do not handle list of lists and list of heterogeneous objects, list of multiline strings, ...
            return not node or not isinstance(node[0], OrderedDict)
        return True

    def _is_simple(self, node):
        if not isinstance(node, list) and not isinstance(node, OrderedDict):
            return True
        if isinstance(node, list):
            for child in node:
                if isinstance(child, list) or isinstance(child, OrderedDict):
                    return False
        return True

    def _process_node(self, node):
        assert (not self._is_leaf(node))
        assert (isinstance(node, list) or isinstance(node, OrderedDict))
        assert node

        first_lines = []
        children = node.values() if isinstance(node, OrderedDict) else node
        indent = 1 if isinstance(node, OrderedDict) else 0

        self.indent += indent
        for child in children:
            first_lines.append(len(self.lines))
            if self._is_leaf(child):
                self._process_leaf(child)
            else:
                self._process_node(child)
        first_lines.append(len(self.lines))
        self.indent -= indent

        new_lines = []
        if isinstance(node, OrderedDict):
            for key, child, first, last in zip(node, children, first_lines[:-1], first_lines[1:]):
                assert (first < last)
                if first + 1 == last and not isinstance(child, OrderedDict) and self._is_simple(child):
                    # 'not isinstance(child, OrderedDict)' -- this is a really weird condition?!. but seems to be useful
                    new_lines.append('%s%s: %s' % (self.indent_str * self.indent, key, self.lines[first].lstrip()))
                else:
                    new_lines.append('%s%s:' % (self.indent_str * self.indent, key))
                    new_lines.extend(self.lines[first:last])
        else:
            for first, last in zip(first_lines[:-1], first_lines[1:]):
                assert (first < last)
                prefix = '-' + self.indent_str[1:]
                next_prefix = self.indent_str
                for line in range(first, last):
                    new_lines.append('%s%s%s' % (
                        self.indent_str * self.indent, prefix, self.lines[line][len(self.indent_str) * self.indent:]))
                    prefix = next_prefix
        assert new_lines
        del self.lines[first_lines[0]:]
        self.lines.extend(new_lines)

    def _process_leaf(self, obj):
        obj = self._process_value(obj)
        for line in obj.split('\n'):
            self._append(line)

    def _process_value(self, obj):
        # TODO: need yaml serialization class for each object
        if obj is None:
            return 'null'
        elif isinstance(obj, bool):
            return 'True' if obj else 'False'
        elif isinstance(obj, int) or isinstance(obj, float):
            return str(obj)
        elif isinstance(obj, card_types.ByteSize) or isinstance(obj, Percentage) or isinstance(obj, card_types.Function) or\
             isinstance(obj, card_types.Date):
            # escape string
            return self._process_value(str(obj))
        elif isinstance(obj, str):
            if '\n' in obj:
                return self._multiline_str(obj)
            else:
                return self._singleline_str(obj)
        elif isinstance(obj, list):
            items = [self._process_value(x) for x in obj]
            assert (all('\n' not in item for item in items))
            return '[%s]' % ', '.join(items)
        elif isinstance(obj, set):
            items = [self._process_value(x) for x in sorted(obj)]
            assert (all('\n' not in item for item in items))
            return '!!set {%s}' % ', '.join(items)
        elif isinstance(obj, OrderedDict):
            assert (not obj)  # in all other cases we should call _process_node(obj)
            return '{}'
        else:
            assert False, "Can not process object of type <%s>" % type(obj)
        return None

    def _multiline_str(self, obj):
        # printed line not ending with '\n' when read again will gain '\n' in the end
        if obj.endswith('\n'):
            obj = obj[:-1]
        obj = '\n'.join([self.indent_str + x for x in obj.split('\n')])
        return '|\n' + obj

    DIGITS_WITH_EXTRA = set('-.0123456789')

    def _singleline_str(self, obj):
        # TODO: get normal serializer from pyyaml!
        if not obj:
            obj = '""'
        elif all(x in CardNodeTextRender.DIGITS_WITH_EXTRA for x in obj):
            obj = '"%s"' % obj
        elif (set(obj) & set(':')) or obj.startswith(('\'', '"', '!', '%', '?', '|', '>', ' ', '[', '{')) or obj.endswith(' '):
            obj = '"%s"' % obj.replace('\\', '\\\\').replace('"', '\\"')
        elif re.match('^0x[\dABCDEFabcdef]+$', obj):
            obj = '"%s"' % obj
        elif re.match('^\d+$', obj):
            obj = '"%s"' % obj
        return obj


# ================================== CardReaderExt ==================================

class CardNodeIOExtBase(object):
    def __init__(self):
        pass

    def _make_inherit_scheme(self, node, scheme):
        for field in node:
            if field not in scheme:
                continue
            if isinstance(scheme[field], SchemeNode):
                if node[field] is not None:
                    self._make_inherit_scheme(node[field], scheme[field])
            else:
                if scheme[field].is_inherited():
                    scheme[field].set_default(node[field], make_copy=False)


class CardReaderExt(CardNodeIOExtBase):
    # Reads master and slaves
    # Implements inheritance
    def __init__(self, scheme):
        CardNodeIOExtBase.__init__(self)
        self.scheme = scheme

    def process_file(self, filename):
        yamled = yaml.load(open(filename).read())
        assert (isinstance(yamled, dict))
        return self.process_obj(yamled)

    def process_obj(self, obj):
        assert (isinstance(obj, dict))
        slaves = []
        if Scheme.slaves_node in obj:
            assert (isinstance(obj[Scheme.slaves_node], list))
            slaves = obj[Scheme.slaves_node]
            del obj[Scheme.slaves_node]

        master = CardReader(self.scheme.get_cached()).process(obj)
        master.add_field('slaves', [])
        if slaves:
            inherit_scheme = self.scheme.get_noncached()
            self._make_inherit_scheme(master, inherit_scheme)
            for n, slave_node in enumerate(slaves):
                reader = CardReader(scheme=inherit_scheme)
                slave = reader.process(slave_node, [Scheme.slaves_node + '[%d]' % n])
                slave.add_field('slaves', [])
                master.slaves.append(slave)

        return master

    def process_slave_obj(self, obj, master_obj, path=None):
        assert (isinstance(obj, dict))

        inherit_scheme = self.scheme.get_noncached()
        self._make_inherit_scheme(master_obj, inherit_scheme)
        reader = CardReader(scheme=inherit_scheme)
        slave = reader.process(obj, path)
        slave.add_field('slaves', [])
        return slave


# ================================== CardWriterExt ==================================

class CardWriterExt(CardNodeIOExtBase):
    class Opts(CardWriter.Opts):
        def __init__(self):
            CardWriter.Opts.__init__(self)
            # append slaves
            self.filtered = True
            self.remove_defaults = True
            self.write_slaves = True

    # Converts master and all it's slaves to renderable object using CardWriter
    # Implements inheritance
    def __init__(self, scheme, opts=None):
        CardNodeIOExtBase.__init__(self)
        self.scheme = scheme
        if opts is None:
            self.opts = CardWriterExt.Opts()
        else:
            self.opts = opts

    def process(self, master):
        opts = copy.copy(self.opts)
        opts.scheme = self.scheme.get_cached()
        root = CardWriter(opts).process(master)
        assert ('slaves' not in root)
        if self.opts.write_slaves and master.slaves:
            root['slaves'] = []
            inherit_scheme = self.scheme.get_noncached()
            self._make_inherit_scheme(master, inherit_scheme)
            for slave in master.slaves:
                assert (slave.slaves == [])
                opts = copy.copy(self.opts)
                opts.scheme = inherit_scheme
                root['slaves'].append(CardWriter(opts).process(slave.card))
        return root


def load_card_node(datafile, schemefile=None, inplace_content=False, cacher=None):
    """
        Function load card node (yaml file in special format) to our internal card node format.

        :datafile (str): file with yaml content
        :schemefile (str): file with schema for content file (like jsonschema for json, but for yaml)
        :inplace_content (bool): load from datafile as string rather than from file with this name
        :cacher (core.cacher.TCacher): caching layer, which allows not to recaclulate complex object
    """

    if schemefile is None:
        firstline = open(datafile).readline().strip()
        m = re.search('\s*#\s+scheme\s+(\S+)\s*', firstline)
        if not m:
            raise Exception("Can not detect scheme file")
        schemefile = m.group(1)

    # try load from cache
    if (cacher is not None) and (not inplace_content):
        result = cacher.try_load([datafile, schemefile])
        if result is not None:
            return result

    scheme = Scheme(schemefile, '2.1')

    if inplace_content:
        content = datafile
    else:
        content = open(datafile).read()
        if content == '':
            raise Exception, "File <%s> is empty" % datafile

    data = yaml.load(content)

    cardreader = CardReader(scheme.get_cached())

    result = cardreader.process(data)

    # save to cache
    if (cacher is not None) and (not inplace_content):

        cacher.save([datafile, schemefile], result)

    return result


def load_card_node_from_dict(data, scheme):
    cardreader = CardReader(scheme.get_cached())
    return cardreader.process(data)


def save_card_node(cardnode, datafile, schemefile):
    scheme = Scheme(schemefile, '2.1')

    cardwriteropts = CardWriter.Opts()
    cardwriteropts.scheme = scheme.get_cached()
    cardwriteropts.filtered = True
    cardwriteropts.remove_defaults = True

    cardwriter = CardWriter(cardwriteropts)
    textrender = CardNodeTextRender()

    with open(datafile, 'w') as f:
        #        f.write("# scheme %s\n" % schemefile)
        f.write(textrender.process(cardwriter.process(cardnode)))
