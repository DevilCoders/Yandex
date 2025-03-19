class InstancesClientError(RuntimeError):
    pass


class ConfigurationError(InstancesClientError):
    pass


class IPV6OnlyPublicIPError(InstancesClientError):
    pass
