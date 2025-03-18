import logging
import six

from library.python.monitoring.solo.util.diff import get_yasm_diff
from library.python.monitoring.solo.objects.yasm import YasmAlert, YasmAlertsTemplate, YasmPanel, YasmPanelsTemplate

if six.PY3:
    from urllib.parse import urljoin
elif six.PY2:
    from urlparse import urljoin

YASM_API_URL = "http://yasm.yandex-team.ru/srvambry/"

logger = logging.getLogger(__name__)


class YasmAction:
    RETRIEVE = "get"
    CREATE = "create"
    UPDATE = "update"
    DELETE = "delete"


API_MAPPING = {
    YasmAlert.__OBJECT_TYPE__: {
        YasmAction.RETRIEVE: "/alerts/get?name={object_id}&with_checks=true",
        YasmAction.CREATE: "/alerts/create",
        YasmAction.UPDATE: "/alerts/update?name={object_id}",
        YasmAction.DELETE: "/alerts/delete?name={object_id}"
    },
    YasmPanel.__OBJECT_TYPE__: {
        YasmAction.RETRIEVE: "/get?key={object_id}",
        YasmAction.CREATE: "/upsert",
        YasmAction.UPDATE: "/upsert",
        YasmAction.DELETE: "/delete?name={object_id}&user={user}"
    },
    YasmAlertsTemplate.__OBJECT_TYPE__: {
        YasmAction.RETRIEVE: "/tmpl/alerts/get?key={object_id}",
        YasmAction.CREATE: "/tmpl/alerts/create",
        YasmAction.UPDATE: "/tmpl/alerts/update?key={object_id}",
        YasmAction.DELETE: "/tmpl/alerts/delete?key={object_id}"
    },
    YasmPanelsTemplate.__OBJECT_TYPE__: {
        YasmAction.RETRIEVE: "/tmpl/panels/get?key={object_id}",
        YasmAction.CREATE: "/tmpl/panels/create",
        YasmAction.UPDATE: "/tmpl/panels/update?key={object_id}",
        YasmAction.DELETE: "/tmpl/panels/delete?key={object_id}"
    }
}

OBJECT_TYPE_TO_CLASS = {
    YasmAlert.__OBJECT_TYPE__: YasmAlert,
    YasmPanel.__OBJECT_TYPE__: YasmPanel,
    YasmAlertsTemplate.__OBJECT_TYPE__: YasmAlertsTemplate,
    YasmPanelsTemplate.__OBJECT_TYPE__: YasmPanelsTemplate,
}


class YasmBaseHandler(object):

    def diff(self, resource):
        return get_yasm_diff(resource.local_state, resource.provider_state)

    def finish(self):
        pass

    def _get_url(self, resource, action=YasmAction.RETRIEVE):
        if resource.local_state:
            object_type = resource.local_state.__OBJECT_TYPE__
            _id = resource.local_state.id
            _user = getattr(resource.local_state, "user", None)
        else:
            object_type = resource.provider_id["object_type"]
            _id = resource.provider_id["id"]
            _user = resource.provider_id["user"]

        route = API_MAPPING[object_type][action].format(object_id=_id, user=_user)
        return urljoin(YASM_API_URL, route.lstrip("/"))

    def _fill_resource_provider_id(self, resource):
        resource.provider_id = {
            "id": resource.local_state.id,
            "user": getattr(resource.local_state, "user", None),
            "object_type": resource.local_state.__OBJECT_TYPE__,
            "handler_type": self.__class__.__name__
        }

    def prepare_data_for_upsert(self, resource):
        if isinstance(resource.local_state, YasmPanel):
            return {
                "keys": {"key": resource.local_state.id,
                         "user": resource.local_state.user},
                "values": resource.local_state.to_json()
            }
        return resource.local_state.to_json()
