import uuid
from collections import defaultdict

from .processor_parameters import ProcessorParameters


class BlockType(object):
    data = 'data'
    operation = 'operation'


def get_block_code(guid, name):
    return str(uuid.uuid4())


class BaseBlock(object):
    _type = BlockType.operation

    name = ''
    guid = None
    parameters = []
    input_names = []
    output_names = []
    processor_type = None
    name_aliases = {}  # map from param/in/out name to list of permitted aliases
    is_strict = True  # fail on unknown param/in/out names
    counter_id = 0

    def __init__(self, guid=None, name=None, code=None, processor_type=None, **kwarg):
        self.name = name or self.name or type(self).__name__
        self.guid = guid or self.guid
        self.processor_type = processor_type or self.processor_type

        self.code = code or get_block_code(self.guid, self.name)
        self.counter_id = BaseBlock.counter_id
        BaseBlock.counter_id += 1

        self.inputs = BlockEndpoint()
        self.outputs = BlockEndpoint()
        self.execute_after = BlockConnection(self, 'execute_after', connection_type=BlockConnectionType.execution, inbound=True)
        self.execute_before = BlockConnection(self, 'execute_before', connection_type=BlockConnectionType.execution, inbound=False)
        self.is_strict = kwarg.pop('is_strict', self.is_strict)

        for param in self.parameters + ProcessorParameters[self.processor_type]:
            attr_name = fix_attr_name(param)
            setattr(self, attr_name, None)
            # alias attrs for parameters are more
            # problematic and less useful, so we don't generate them

        for name in self.input_names:
            attr_name = fix_attr_name(name)
            connection = BlockConnection(self, name, connection_type=BlockConnectionType.io, inbound=True)
            super(BlockEndpoint, self.inputs).__setattr__(attr_name, connection)
            for alias in self.name_aliases.get(name, []):
                alias_attr_name = fix_attr_name(alias)
                super(BlockEndpoint, self.inputs).__setattr__(alias_attr_name, connection)

        for name in self.output_names:
            attr_name = fix_attr_name(name)
            connection = BlockConnection(self, name, connection_type=BlockConnectionType.io, inbound=False)
            super(BlockEndpoint, self.outputs).__setattr__(attr_name, connection)
            for alias in self.name_aliases.get(name, []):
                alias_attr_name = fix_attr_name(alias)
                super(BlockEndpoint, self.outputs).__setattr__(alias_attr_name, connection)

        self.configure(**kwarg)

    def configure(self, **kwarg):
        is_strict = kwarg.pop('is_strict', self.is_strict)
        names = set(kwarg.keys())
        self._do_configure_parameters(names, is_strict, kwarg)
        self._do_configure_inputs(names, is_strict, kwarg)
        self._do_configure_execute_after(names, kwarg)
        assert not is_strict or len(names) == 0, (
            'block guid:{} configuration error, some names were not recognized: {}'
            .format(self.guid, sorted(names))
        )

        return self

    def configure_parameters(self, **kwarg):
        is_strict = kwarg.pop('is_strict', self.is_strict)
        names = set(kwarg.keys())
        self._do_configure_parameters(names, True, kwarg)

        assert not is_strict or len(names) == 0, \
            ('block parameters configuration error, some names were not recognized: {0}'.format(', '.join(names)))

        return self

    def configure_inputs(self, **kwarg):
        is_strict = kwarg.pop('is_strict', self.is_strict)
        names = set(kwarg.keys())
        self._do_configure_inputs(names, True, kwarg)

        assert not is_strict or len(names) == 0, \
            ('block inputs configuration error, some names were not recognized: {0}'.format(', '.join(names)))

        return self

    def _do_configure_execute_after(self, names, kwarg):
        if 'execute_after' not in kwarg:
            return self
        self.execute_after.disconnect_all()
        for block in _list(kwarg['execute_after']):
            self.execute_after.connect(block.execute_before)
        self._count_and_clear_aliases('execute_after', names)
        return self

    def _do_configure_parameters(self, names, check_type, kwarg):
        for param in set(self.parameters + ProcessorParameters[self.processor_type]):
            attr_name = fix_attr_name(param)
            alias = self._find_alias(param, names)
            if alias:
                arg_value = kwarg.pop(alias)
                if not _is_block_connection(arg_value):
                    setattr(self, attr_name, arg_value)

                    assert 1 == self._count_and_clear_aliases(param, names), \
                        ('block parameters configuration error, parameter "{0}" occurs more than once'.format(param))
                else:
                    kwarg[alias] = arg_value  # return it back
                    # assert not check_type, ('unexpected value type for parameter "{0}", {1}'.format(param, arg_value))

    def _do_configure_inputs(self, names, check_type, kwarg):
        for name in set(self.input_names):
            attr_name = fix_attr_name(name)
            alias = self._find_alias(name, names)
            if alias:
                arg_value = kwarg.pop(alias)
                if _is_block_connection(arg_value) or arg_value == [] or arg_value is None:
                    if arg_value:
                        connection = getattr(self.inputs, attr_name)
                        connection.disconnect_all()
                        connection.connect(arg_value)

                    assert 1 == self._count_and_clear_aliases(name, names), \
                        ('block inputs configuration error, input "{0}" occurs more than once'.format(name))
                else:
                    kwarg[alias] = arg_value  # return it back
                    # assert not check_type, ('unexpected value type for input "{0}", {1}'.format(name, arg_value))

    def _find_alias(self, param_name, params):
        attr_name = fix_attr_name(param_name)
        if attr_name in params:
            return attr_name

        aliases = self.name_aliases.get(param_name)
        if not aliases:
            return None

        if isinstance(aliases, str):
            aliases = (aliases,)

        for alias in aliases:
            alias_attr_name = fix_attr_name(alias)
            if alias_attr_name in params:
                return alias_attr_name

        return None

    def _count_and_clear_aliases(self, param_name, params):
        alias = self._find_alias(param_name, params)
        res = 0
        while alias:
            res += 1
            params.remove(alias)
            alias = self._find_alias(param_name, params)

        return res

    def _get_ordered_endpoints(self, names, endpoints):
        res = []
        for name in names:
            attr_name = fix_attr_name(name)
            value = endpoints.get(attr_name)
            assert value, ('missing endpoint, {0}, {1}'.format(name, attr_name))
            res.append((name, value))

        return res

    def get_ordered_inputs(self):
        return self._get_ordered_endpoints(self.input_names, vars(self.inputs))

    def get_ordered_outputs(self):
        return self._get_ordered_endpoints(self.output_names, vars(self.outputs))

    def get_execute_after(self):
        return self.execute_after

    def get_execute_before(self):
        return self.execute_before

    def __eq__(self, other):
        return isinstance(other, BaseBlock) and self.code == other.code

    def __hash__(self):
        return hash(self.code)


class BaseDataBlock(BaseBlock):
    _type = BlockType.data
    output_names = ['data']


class BlockEndpoint(object):

    def __setattr__(self, name, val):
        connection = getattr(self, name, None)
        connection.disconnect_all()
        connection.connect(val)


class BlockConnectionType(object):
    execution = 0
    io = 1
    dynamic = 2


class BlockConnection(object):

    def __init__(self, obj, name, links=None, connection_type=BlockConnectionType.io, inbound=False):
        self.obj = obj
        self.name = name
        self.links = set()
        self.connect(links)
        self.connection_type = connection_type
        self.inbound = inbound

    def _get_key(self):
        return self.obj.name, self.name, self.obj.code

    def connect(self, links):
        new_links = set(_list(links) or [])
        for link in new_links:
            link.links.add(self)
        self.links.update(new_links)

    def disconnect_all(self):
        for link in self.links:
            link.links.remove(self)
        self.links.clear()

    def get_ordered_links(self):
        res = list(self.links)
        res.sort(key=lambda link: link._get_key())
        return res


class BlocksWrapper(object):
    def __init__(self, connections=()):
        self.inputs = BlockEndpoint()
        self.outputs = BlockEndpoint()

        self.set_connections(connections)

    def set_connections(self, connections):
        """
        Each connection is specified by tuple (connection_name: str, connection: BlockConnection)
        """
        if len(connections) == 2 and isinstance(connections[1], BlockConnection):
            connections = [connections]
        for name, connection in connections:
            self.set_connection(name, connection)

    def set_connection(self, name, connection):
        assert isinstance(connection, BlockConnection)
        endpoint = self.inputs if connection.inbound else self.outputs
        super(type(endpoint), endpoint).__setattr__(name, connection)


def fix_attr_name(name):
    return ''.join(s if s.isalnum() else '_' for s in name)


def _list(data):
    return data if data is None or isinstance(data, list) else [data]


def _is_block_connection(value):
    return (isinstance(value, BlockConnection) or
            (isinstance(value, list) and len(value) and isinstance(value[0], BlockConnection)))


# Decorator for blocks
#
# If block class is decorated with this function instantiating class with same parameters in constructor
# will return single cached instance. This behaviour is very useful if you generate graph procedurally
# and some resource-demanding blocks have exactly same inputs
#
# Warning: it doesn't work well if you modify block inputs after its creation
#
# @param join_params - if blocks are different only in this params than still a single block will be created
#                      with param values being concatenation of values of different blocks
def singleton_block(join_params=[]):
    join_params = set(join_params + ['name'])

    def decorator(block_class):
        cache = defaultdict(list)

        def get(**params):
            fingerprint = ';'.join(sorted(params.keys()))
            fcache = cache[fingerprint]
            # (exclude joined params) for comparison purposes
            filtered_params = {key: val for (key, val) in params.items() if key not in join_params}
            for (cached_params, cached_instance, seen_join_params) in fcache:
                if cached_params == filtered_params:
                    # join params!
                    for key in (set(params.keys()) & join_params):
                        values = seen_join_params[key]
                        values.add(params[key])
                        joined_values = ' | '.join(sorted(values))
                        if len(joined_values) > 250:
                            joined_values = joined_values[:250] + '...'
                        setattr(cached_instance, key, joined_values)
                    return cached_instance
            res = block_class(**params)
            seen_join_params = {key: set([val]) for (key, val) in params.items() if key in join_params}
            fcache.append((filtered_params, res, seen_join_params))
            return res
        return get
    return decorator
