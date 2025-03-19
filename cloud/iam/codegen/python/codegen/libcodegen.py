import re

import yaml


class Identifier:
    def __init__(self, parts):
        self.parts = parts

    @property
    def snake_case(self):
        return '_'.join((s.lower() for s in self.parts))

    @property
    def snake_upper_case(self):
        return '_'.join((s.upper() for s in self.parts))

    @property
    def camel_case(self):
        if not self.parts:
            return ''
        if len(self.parts) == 1:
            return self.parts[0].lower()
        return ''.join(
            [self.parts[0].lower()] +
            [s.capitalize() for s in self.parts[1:]]
        )

    @property
    def pascal_case(self):
        return ''.join((s.capitalize() for s in self.parts))


QUOTA_NAME_REGEX = re.compile("(?P<service>[a-zA-Z-]+)\.(?P<quotaType>[a-zA-Z0-9-]+)\.(?P<metricType>.+)$")
RESOURCE_NAME_REGEX = re.compile("(?P<system>root|resourceType)|(?P<service>[a-zA-Z0-9-]+)\.(?P<suffix>[a-zA-Z-]+)$")
PERMISSION_NAME_REGEX = re.compile("(?P<service>[a-zA-Z0-9-]+)\.(?P<suffix>[a-zA-Z0-9-.]+)$")


def camel_to_snake(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()


class Quota:
    def __init__(self, quota_name):
        match = QUOTA_NAME_REGEX.match(quota_name)
        if match is None:
            raise Exception("Can't parse the quota name: '" + quota_name + "'")
        parts = match.groupdict()
        self.service_name = parts["service"]
        self.service = Identifier(self.service_name.split("-"))
        quotaType = parts["quotaType"].replace("-", "_")
        metricType = parts["metricType"]

        parts = camel_to_snake(quotaType).split('_')
        parts.append(metricType)
        self.type = Identifier(parts)
        self.name = quota_name


class Resource:
    def __init__(self, resource_name):
        match = RESOURCE_NAME_REGEX.match(resource_name)
        if match is None:
            raise Exception("Can't parse the resource name: '" + resource_name + "'")
        parts = match.groupdict()
        if parts["service"]:
            self.service_name = parts["service"]
            suffix = parts['suffix'].replace("-", "_")
        else:
            self.service_name = "system"
            suffix = parts['system']
        self.service = Identifier(self.service_name.split("-"))

        parts = camel_to_snake(suffix).split('_')
        self.type = Identifier(parts)
        self.name = resource_name


class Permission:
    def __init__(self, permission):
        self.id = Identifier(camel_to_snake(permission.replace(".", "_").replace("-", "_")).split("_"))
        self.name = permission


class Environment:
    def __init__(self, quotas, resources, permissions):
        self.quotas = quotas
        self.resources = resources
        self.permissions = permissions
        self.service_quotas = {}
        self.service_resources = {}
        for quota in quotas:
            self.service_quotas.setdefault(quota.service_name, []).append(quota)
        for resource in resources:
            self.service_resources.setdefault(resource.service_name, []).append(resource)


def load_environment(model_file):
    with open(model_file, encoding='utf-8') as f:
        model = yaml.load(f, yaml.BaseLoader)
        quotas = [Quota(quota_name) for quota_name in model['quotas']]
        resources = [Resource(resource_name) for resource_name in model['resources']]
        permissions = [Permission(permission) for permission in model['permissions']]
        return Environment(quotas, resources, permissions)
