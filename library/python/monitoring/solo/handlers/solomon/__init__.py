import logging

from library.python.monitoring.solo.handlers.solomon.v2 import SolomonV2Handler
from library.python.monitoring.solo.handlers.solomon.v3 import SolomonV3Handler
from library.python.monitoring.solo.objects.solomon.v2 import Alert as AlertV2, \
    MultiAlert as MultiAlertV2, Channel as ChannelV2, Cluster as ClusterV2, Graph as GraphV2, \
    Dashboard as DashboardV2, Project as ProjectV2, Service as ServiceV2, Shard as ShardV2, \
    Menu as MenuV2
from library.python.monitoring.solo.objects.solomon.v3 import Alert as AlertV3, \
    Channel as ChannelV3, Cluster as ClusterV3, Dashboard as DashboardV3, \
    Project as ProjectV3, Service as ServiceV3, Shard as ShardV3

SOLOMON_OBJECTS_CREATION_ORDER = [
    (ProjectV2, ProjectV3),
    (ClusterV2, ClusterV3),
    (ServiceV2, ServiceV3),
    (ShardV2, ShardV3),
    (ChannelV2, ChannelV3),
    (GraphV2, None),
    (AlertV2, AlertV3),
    (MultiAlertV2, None),
    (DashboardV2, DashboardV3),
    (MenuV2, None)  # TODO add QuickLinks
]

logger = logging.getLogger(__name__)


def group_by_solomon_handlers(configuration, resources):
    handler_v2, handler_v3 = None, None
    for v2_obj_type, v3_obj_type in SOLOMON_OBJECTS_CREATION_ORDER:

        if v2_obj_type is not None:
            def filter_v2(_resource):
                if v2_obj_type is AlertV2 and isinstance(_resource.local_state, MultiAlertV2):
                    return False
                return isinstance(_resource.local_state, v2_obj_type) or \
                    (_resource.provider_id and
                     _resource.provider_id.get("object_type", None) == v2_obj_type.__OBJECT_TYPE__ and
                     _resource.provider_id.get("handler_type", None) == SolomonV2Handler.__name__)

            resources_v2 = list(filter(filter_v2, resources))
            if resources_v2:
                if handler_v2 is None:
                    handler_v2 = SolomonV2Handler(configuration["solomon_token"], configuration["solomon_endpoint"])
                yield handler_v2, v2_obj_type.__name__, resources_v2

        if v3_obj_type is not None:
            def filter_v3(_resource):

                flag = isinstance(_resource.local_state, v3_obj_type) or \
                    (_resource.provider_id and
                     _resource.provider_id.get("object_type", None) == v3_obj_type.DESCRIPTOR.name and
                     _resource.provider_id.get("handler_type", None) == SolomonV3Handler.__name__)
                return flag

            resources_v3 = list(filter(filter_v3, resources))
            if resources_v3:
                if handler_v3 is None:
                    handler_v3 = SolomonV3Handler(configuration["solomon_token"], configuration["solomon_endpoint"])
                yield handler_v3, v3_obj_type.__name__, resources_v3
