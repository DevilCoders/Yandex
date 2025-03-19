'''
    Client for infra.yandex-team.ru/timeline
'''

import json

from datetime import datetime
from yc_common.ya_clients.common import IntranetClient, retry_server_errors
from yc_common import logging, constants
from yc_common.models import Model, StringType, IntType, ListType, ModelType, BooleanType

log = logging.get_logger(__name__)


class TimeLineNamespace(Model):
    id = IntType(required=True)
    name = StringType(required=True)


class TimeLineServiceEnv(Model):
    id = IntType(required=True)
    name = StringType(required=True)
    calendar_id = IntType(required=False)
    myt = BooleanType(required=False)
    vla = BooleanType(required=False)
    iva = BooleanType(required=False)
    man = BooleanType(required=False)
    sas = BooleanType(required=False)
    is_juggler_active = BooleanType(required=False)
    is_calendar_active = BooleanType(required=False)
    juggler_host = StringType(required=False)
    default_juggler_host = StringType(required=False)


class TimeLineService(Model):
    id = IntType(required=True)
    name = StringType(required=True)
    environments = ListType(ModelType(TimeLineServiceEnv), required=True)
    admins = ListType(StringType, required=True)
    namespace = ModelType(TimeLineNamespace, required=True)


class _IntranetClient(IntranetClient):
    @retry_server_errors
    def put(self, query, data):
        return self._call("PUT", query, data)


class TimeLineClient(_IntranetClient):
    def __init__(self, base_url, token, client_id, timeout=30):
        super().__init__(base_url, auth_type="oauth", headers={"Content-Type": "application/json"}, client_id=client_id,
                         token=token, timeout=timeout)

    @retry_server_errors
    def get_service(self, id):
        return TimeLineService.from_api(self.get("services/mine/{}".format(id)))

    def create_event(self, service_id, env_id, title, description, ticket_id="", datacenters=None,
                     type_event="maintenance", severity="major"):
        data = {
            "title": title,
            "description": description,
            "environmentId": env_id,
            "serviceId": service_id,
            "type": type_event,
            "severity": severity,
            "startTime": int(datetime.now().timestamp()),
            "finishTime": int(datetime.now().timestamp()) + 120 * constants.MINUTE_SECONDS,
            "tickets": ticket_id
        }

        if datacenters:
            for dc in datacenters:
                data[dc] = True

        return self.post("events", json.dumps(data))

    @retry_server_errors
    def stop_event(self, id, description):
        data = {
            "finishTime": int(datetime.now().timestamp()),
            "description": description
        }

        return self.put("events/{}".format(id), json.dumps(data))


def get_timeline_client(cfg):
    return TimeLineClient(
        cfg.url,
        cfg.token,
        cfg.client_id
    )

