from yc_common.exceptions import Error


class SchemaError(Error):
    pass


class SchemaVersionNotAvailable(SchemaError):
    def __init__(self, version):
        self.version = version
        super().__init__("Schema version {} is not available", self.version)


class ResourceNotDefined(SchemaError):
    def __init__(self, resource_name):
        self.resource_name = resource_name
        super().__init__("Resource '{}' is not defined in the schema", self.resource_name)