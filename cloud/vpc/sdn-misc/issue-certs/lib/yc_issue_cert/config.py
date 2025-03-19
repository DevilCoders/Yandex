
import yaml

from schematics.models import Model
from schematics.types import IntType, ListType, DictType, StringType, BooleanType, URLType
from schematics.types.compound import ModelType


class YcCrtProfile(Model):
    endpoint = URLType(required=True)
    ca_type = StringType(required=True, choices=["ca", "test-ca"])
    ca_name = StringType(required=True)


class YcCrtConfig(Model):
    profiles = DictType(ModelType(YcCrtProfile))
    desired_ttl_days = IntType(required=True, min_value=30)
    abc_service = IntType(required=True)


class SecretServiceConfig(Model):
    agent_base_path = StringType(required=True)
    allowed_base_roles = ListType(StringType())
    base_role_abc_groups = DictType(ListType(StringType()))


class ClusterConfig(Model):
    secret_profile = StringType(required=True)
    prefix = StringType()
    is_cloudvm = BooleanType(default=False)
    bootstrap_stand = StringType(required=False)
    zone_id = StringType(required=False)
    scope = StringType(required=False)

    hosts = DictType(ListType(StringType), required=True)
    clients = DictType(DictType(ListType(StringType)), required=True)


class SecretConfig(Model):
    type = StringType(required=True)
    scope = StringType(choices=["ROLE", "HOST"], required=True)


class SecretGroupConfig(Model):
    all_hosts_cert = BooleanType()

    base_roles = ListType(StringType)
    client_group = StringType()
    secrets = ListType(ModelType(SecretConfig))


class Config(Model):
    yc_crt = ModelType(YcCrtConfig)
    secret_service = ModelType(SecretServiceConfig)

    clusters = DictType(ModelType(ClusterConfig))
    secret_groups = DictType(ModelType(SecretGroupConfig))


def load_config(config_path: str):
    with open(config_path) as config_file:
        raw_config = yaml.safe_load(config_file)

    return Config(raw_config, strict=True)
