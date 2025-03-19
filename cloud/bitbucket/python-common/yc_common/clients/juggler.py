""" Juggler client """
import requests

from typing import List, Tuple

from yc_common import config, logging
from yc_common.clients.api import ApiClient, ApiError
from yc_common.exceptions import Error
from yc_common.models import Model, ListType, ModelType, StringType, IntType, FloatType, DictType, BooleanType

log = logging.get_logger(__name__)


class CommonFilter(Model):
    host = StringType(serialize_when_none=False)
    instance = StringType(serialize_when_none=False)
    namespace = StringType(serialize_when_none=False)
    service = StringType(serialize_when_none=False)
    tags = ListType(StringType, serialize_when_none=False)


class DowntimeFilter(CommonFilter):
    downtime_id = StringType(serialize_when_none=False)
    source = StringType(serialize_when_none=False)
    user = StringType(serialize_when_none=False)


class CheckStatusFilter(CommonFilter):
    recipients = StringType()
    responsibles = StringType()
    namespace = StringType()


class DowntimeInfoCommon(Model):
    description = StringType()
    downtime_id = StringType()
    end_time = FloatType()
    start_time = FloatType()
    filters = ListType(ModelType(CommonFilter))
    source = StringType()


class DowntimeInfo(DowntimeInfoCommon):
    user = StringType()


class StatusInfo(Model):
    count = IntType()
    status = StringType()


class CheckInfo(Model):
    meta = StringType()
    description = StringType()
    host = StringType()
    service = StringType()
    status = StringType()
    change_time = FloatType()
    aggregration_time = FloatType()
    downtime_ids = ListType(StringType)


class GetDowntimesRequest(Model):
    filters = ListType(ModelType(DowntimeFilter), serialize_when_none=False)
    exclude_future = BooleanType(serialize_when_none=False)
    page = IntType(default=0)
    page_size = IntType(default=0)


class SetDowntimeRequest(DowntimeInfoCommon):
    pass


class DelDowntimesRequest(Model):
    downtime_ids = ListType(StringType)


class GetCheckStatusRequest(Model):
    filters = ListType(ModelType(CheckStatusFilter), serialize_when_none=False)
    limit = IntType(default=0)
    statuses = ListType(StringType)


class ResponseMeta(Model):
    _backend = StringType()


class Error(Model):
    field_id = StringType()
    message = StringType(required=True)
    meta = ModelType(ResponseMeta)


class _ApiClient(ApiClient):
    def _parse_error(self, response):
        error, _ = self._parse_json_response(response, model=Error)
        raise ApiError(response.status_code, response.status_code, error["message"])


class JugglerClient(_ApiClient):
    """JugglerClient"""
    class ResultCommon(Model):
        meta = DictType(StringType)

    def __init__(self, juggler_url, oauth_token):
        super().__init__(url=juggler_url, extra_headers={"Authorization": "OAuth {}".format(oauth_token)})

    def get_downtimes(self, request: GetDowntimesRequest=GetDowntimesRequest()) -> List[DowntimeInfo]:
        class Result(self.ResultCommon):
            items = ListType(ModelType(DowntimeInfo))
            total = IntType()

        v = self.post(path="/v2/downtimes/get_downtimes", request=request, model=Result)
        # TODO: Juggler can't return more than 1k items What shall we do if result contains more than 1k items or page_size is less then total items..
        return v.items

    def set_downtime(self, request: SetDowntimeRequest) -> str:
        class Result(self.ResultCommon):
            downtime_id = StringType()

        v = self.post(path="/v2/downtimes/set_downtimes", request=request, model=Result)
        return v.downtime_id

    def del_downtimes(self, request: DelDowntimesRequest) -> List[DowntimeInfo]:
        class Result(self.ResultCommon):
            downtimes = ListType(ModelType(DowntimeInfo))

        v = self.post(path="/v2/downtimes/remove_downtimes", request=request, model=Result)
        return v.downtimes

    def get_check_status(self, request: GetCheckStatusRequest) -> Tuple[List[CheckInfo], List[StatusInfo]]:
        class Result(self.ResultCommon):
            statuses = ListType(ModelType(StatusInfo))
            items = ListType(ModelType(CheckInfo))

        v = self.post(path="/v2/checks/get_checks_state", request=request, model=Result)
        return v.items, v.statuses

