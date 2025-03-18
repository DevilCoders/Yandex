import six

from library.python.monitoring.solo.objects.solomon.v2 import Alert, MultiAlert, Channel, Cluster, Dashboard, Graph, Project, Service, Shard, Menu
from library.python.monitoring.solo.util.diff import get_jsonobject_diff

if six.PY3:
    from urllib.parse import urljoin
elif six.PY2:
    from urlparse import urljoin

API_V2_MAPPING = {
    Alert.__OBJECT_TYPE__: "/projects/{project_id}/alerts/{object_id}",
    MultiAlert.__OBJECT_TYPE__: "/projects/{project_id}/alerts/{object_id}",
    Channel.__OBJECT_TYPE__: "/projects/{project_id}/notificationChannels/{object_id}",
    Cluster.__OBJECT_TYPE__: "/projects/{project_id}/clusters/{object_id}",
    Dashboard.__OBJECT_TYPE__: "/projects/{project_id}/dashboards/{object_id}",
    Graph.__OBJECT_TYPE__: "/projects/{project_id}/graphs/{object_id}",
    Menu.__OBJECT_TYPE__: "/projects/{project_id}/menu",
    Project.__OBJECT_TYPE__: "/projects/{object_id}",
    Service.__OBJECT_TYPE__: "/projects/{project_id}/services/{object_id}",
    Shard.__OBJECT_TYPE__: "/projects/{project_id}/shards/{object_id}"
}

OBJECT_TYPE_TO_CLASS = {
    Alert.__OBJECT_TYPE__: Alert,
    MultiAlert.__OBJECT_TYPE__: MultiAlert,
    Channel.__OBJECT_TYPE__: Channel,
    Cluster.__OBJECT_TYPE__: Cluster,
    Dashboard.__OBJECT_TYPE__: Dashboard,
    Graph.__OBJECT_TYPE__: Graph,
    Project.__OBJECT_TYPE__: Project,
    Service.__OBJECT_TYPE__: Service,
    Shard.__OBJECT_TYPE__: Shard,
    Menu.__OBJECT_TYPE__: Menu
}


class SolomonV2BaseHandler(object):

    def __init__(self, endpoint):
        self.endpoint = urljoin(endpoint, "/api/v2/")

    def diff(self, resource):
        return get_jsonobject_diff(resource.local_state, resource.provider_state)

    def _get_url(self, resource):
        if resource.local_state:
            object_type = resource.local_state.__OBJECT_TYPE__
            project_id = resource.local_state.project_id
            _id = resource.local_state.id
        else:
            object_type = resource.provider_id["object_type"]
            project_id = resource.provider_id["project_id"]
            _id = resource.provider_id["id"]
        route = API_V2_MAPPING[object_type].format(project_id=project_id, object_id=_id)
        return urljoin(self.endpoint, route.lstrip("/"))

    def _fill_resource_provider_id(self, resource):
        resource.provider_id = {
            "id": resource.local_state.id,
            "project_id": resource.local_state.project_id,
            "object_type": resource.local_state.__OBJECT_TYPE__,
            "handler_type": self.__class__.__name__
        }
