import six
from google.protobuf.json_format import MessageToDict
from solomon.protos.api.v3 import alert_service_pb2, channel_service_pb2, cluster_service_pb2, dashboard_service_pb2, project_service_pb2, \
    service_service_pb2, shard_service_pb2

from library.python.monitoring.solo.objects.solomon.v3 import Alert, Channel, Cluster, Dashboard, Project, Service, Shard
from library.python.monitoring.solo.util.diff import get_jsonobject_diff

if six.PY3:
    from urllib.parse import urljoin
elif six.PY2:
    from urlparse import urljoin

API_V3_MAPPING = {
    Alert.DESCRIPTOR.name: "/projects/{project_id}/alerts/{object_id}",
    Channel.DESCRIPTOR.name: "/projects/{project_id}/channels/{object_id}",
    Cluster.DESCRIPTOR.name: "/projects/{project_id}/clusters/{object_id}",
    Dashboard.DESCRIPTOR.name: "/projects/{project_id}/dashboards/{object_id}",
    Project.DESCRIPTOR.name: "/projects/{object_id}",
    Service.DESCRIPTOR.name: "/projects/{project_id}/services/{object_id}",
    Shard.DESCRIPTOR.name: "/projects/{project_id}/shards/{object_id}"
}

OBJECT_TYPE_TO_CLASS = {
    Alert.DESCRIPTOR.name: Alert,
    Channel.DESCRIPTOR.name: Channel,
    Cluster.DESCRIPTOR.name: Cluster,
    Dashboard.DESCRIPTOR.name: Dashboard,
    Project.DESCRIPTOR.name: Project,
    Service.DESCRIPTOR.name: Service,
    Shard.DESCRIPTOR.name: Shard
}

OBJECT_TYPE_TO_SERVICE_MODULE = {
    Alert.DESCRIPTOR.name: alert_service_pb2,
    Channel.DESCRIPTOR.name: channel_service_pb2,
    Cluster.DESCRIPTOR.name: cluster_service_pb2,
    Dashboard.DESCRIPTOR.name: dashboard_service_pb2,
    Project.DESCRIPTOR.name: project_service_pb2,
    Service.DESCRIPTOR.name: service_service_pb2,
    Shard.DESCRIPTOR.name: shard_service_pb2
}

OBJECT_TYPES_CREATED_WITH_ID = {
    Project.DESCRIPTOR.name,
    Cluster.DESCRIPTOR.name,
    Service.DESCRIPTOR.name,
    Shard.DESCRIPTOR.name,
    Alert.DESCRIPTOR.name,
    Channel.DESCRIPTOR.name
}


class SolomonV3BaseHandler(object):

    def __init__(self, endpoint):
        self.endpoint = urljoin(endpoint, "/api/v3/")

    def diff(self, resource):
        # TODO: better logic separation for diff functions
        local_state = MessageToDict(resource.local_state) if resource.local_state else None
        provider_state = MessageToDict(resource.provider_state) if resource.provider_state else None
        if provider_state and local_state:
            provider_state["id"] = local_state["id"]
        return get_jsonobject_diff(local_state, provider_state)

    def _get_request(self, request_type_str, resource):
        object_type = resource.provider_id["object_type"]
        request_type = getattr(OBJECT_TYPE_TO_SERVICE_MODULE[object_type], "{0}{1}Request".format(request_type_str, object_type))
        request = {}
        if request_type_str == "Delete":
            request["{0}_id".format(object_type.lower())] = resource.provider_id["id"]
            request["project_id"] = resource.provider_id["project_id"]
        else:
            for field in request_type.DESCRIPTOR.fields:
                if field.name in resource.local_state.DESCRIPTOR.fields_by_name.keys():
                    request[field.name] = getattr(resource.local_state, field.name)
            if object_type in OBJECT_TYPES_CREATED_WITH_ID:
                request["{0}_id".format(object_type.lower())] = resource.provider_id["id"]
            request["project_id"] = resource.provider_id["project_id"]
        request = request_type(**request)
        return request

    def _get_create_request(self, resource):
        return self._get_request("Create", resource)

    def _get_update_request(self, resource):
        return self._get_request("Update", resource)

    def _get_delete_request(self, resource):
        return self._get_request("Delete", resource)

    def _get_url(self, resource):
        route = API_V3_MAPPING[resource.provider_id["object_type"]].format(
            project_id=resource.provider_id["project_id"],
            object_id=resource.provider_id.get("id")
        )
        return urljoin(self.endpoint, route.lstrip("/"))
