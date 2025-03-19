from yc_common.clients.contrail.resource import RootCollection
from yc_common.clients.contrail.schema.exceptions import ResourceNotDefined
from yc_common.clients.contrail.utils import to_json


class Schema(object):
    def __init__(self, version):
        self._schema = {}
        self._version = version

    @property
    def version(self):
        return self._version

    def resource(self, resource_name):
        try:
            return self._schema[resource_name]
        except KeyError:
            raise ResourceNotDefined(resource_name)

    def all_resources(self):
        return self._schema.keys()

    def _get_or_add_resource(self, resource_name):
        if resource_name not in self._schema:
            self._schema[resource_name] = ResourceSchema()
        return self._schema[resource_name]


class ResourceProperty(object):

    def __init__(self, name, is_list=False, is_map=False):
        if is_list is True:
            self.default = []
        elif is_map is True:
            self.default = {}
        else:
            self.default = None
        self.name = name
        self.key = name.replace('-', '_')

    def __unicode__(self):
        return "{}".format(self.name)

    def __repr__(self):
        return '{}({})'.format(self.__class__.__name__, self.key)


class ResourceSchema(object):

    def __init__(self):
        self.children = []
        self.parent = None
        self.refs = []
        self.back_refs = []
        self.properties = []

    def json(self):
        data = {'children': self.children,
                'parent': self.parent,
                'refs': self.refs,
                'back_refs': self.back_refs,
                'properties': self.properties}
        return to_json(data)


class DummySchema(object):

    def __init__(self, context):
        self._dummy_resorce_schema = DummyResourceSchema(context)

    @property
    def version(self):
        return "dummy"

    def resource(self, resource_name):
        if resource_name not in self.all_resources():
            raise ResourceNotDefined(resource_name)
        return self._dummy_resorce_schema

    def all_resources(self):
        return self._dummy_resorce_schema.children


class DummyResourceSchema(ResourceSchema):

    def __init__(self, context):
        ResourceSchema.__init__(self)
        # add all resource types to all link types
        # so that LinkedResources can find linked
        # resources in the json representation
        self.children = self.refs = self.back_refs = \
            [c.type for c in RootCollection(context, fetch=True)]