from schematics import common as schematics_common
from schematics.types import StringType, IntType, ListType, ModelType, BooleanType

from yc_common.models import Model


class ClusterNetworkConfig(Model):
    name = StringType(required=True)
    host = StringType(required=True)
    port = IntType(required=True)


class ZoneNetworkConfig(Model):
    name = StringType(required=True)
    oct_clusters = ListType(ModelType(ClusterNetworkConfig), required=True)


class NetworkEndpointConfig(Model):
    protocol = StringType(default="http")
    schema_version = StringType()
    zones = ListType(ModelType(ZoneNetworkConfig), required=True)
    # FIXME: See https://st.yandex-team.ru/CLOUD-32984
    use_network_default_routing_policies = BooleanType(default=False, export_level=schematics_common.DROP)
