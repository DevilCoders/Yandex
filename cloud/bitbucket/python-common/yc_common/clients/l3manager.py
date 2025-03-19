import functools
import time
from typing import List

from yc_common import config
from yc_common.clients.api import ApiClient
from yc_common.exceptions import ApiError
from yc_common.models import Model, StringType, IntType, ModelType, BooleanType, ListType, DateTimeType, \
    SchemalessDictType

DEFAULT_LIMIT = 300


def retry_exceptions(func):
    @functools.wraps(func)
    def decorator(*args, **kwargs):
        max_tries = 3
        sleep_time = 2

        cur_try = 0

        for cur_try in range(1, max_tries+1):
            try:
                return func(*args, **kwargs)
            except Exception:
                if cur_try == max_tries:
                    raise

                time.sleep(sleep_time)

    return decorator


class L3ManagerEndpointConfig(Model):
    token = StringType(required=True)
    base_url = StringType(default="https://l3-api.tt.yandex-team.ru/api/v1/")
    timeout = IntType(default=300)


class Error(Model):
    result = StringType(required=True)
    message = StringType(required=True)


class _ApiClient(ApiClient):
    def _parse_error(self, response):
        error, _ = self._parse_json_response(response, model=Error)
        raise ApiError(response.status_code, response.status_code, error["message"])


# model names taken from l3mgr sources:
# https://github.yandex-team.ru/mnt-traf/l3mgr/blob/master/l3mgr/models.py


class LoadBalancer(Model):
    id = IntType(required=True)
    url = StringType(required=True)
    name = StringType(required=True)
    fqdn = StringType(required=True)
    location = ListType(StringType, required=True)
    test_env = BooleanType(required=True)
    full = BooleanType(required=True)
    state = StringType(required=True)


class VirtualServerState(Model):
    description = StringType(required=True)
    timestamp = DateTimeType(required=True)
    state = StringType(required=True)
    lb = ModelType(LoadBalancer, required=True)


class VirtualServer(Model):
    id = IntType(required=True)
    url = StringType(required=True)
    active_count = IntType()
    total_count = IntType()
    editable = BooleanType()
    ext_id = StringType(required=True)
    group = ListType(StringType)
    ip = StringType(required=True)
    port = IntType(required=True)
    protocol = StringType(required=True)
    lb = ListType(ModelType(LoadBalancer), required=True)
    config = SchemalessDictType(required=True)
    status = ListType(ModelType(VirtualServerState), required=True)


class RealServer(Model):
    id = IntType(required=True)
    fqdn = StringType(required=True)
    ip = StringType(required=True)
    group = StringType(required=True)
    config = SchemalessDictType(required=True)
    location = ListType(StringType, required=True)


class Configuration(Model):
    id = IntType(required=True)
    url = StringType(required=True)
    comment = StringType(required=True)
    description = StringType(required=True)
    history = ListType(IntType)
    #    service = ModelType(Service)
    state = StringType(required=True)
    timestamp = DateTimeType(required=True)
    vs_id = ListType(IntType)


class Service(Model):
    id = IntType(required=True)
    url = StringType(required=True)
    abc = StringType()
    action = ListType(StringType)
    archive = BooleanType(required=True)
    config = ModelType(Configuration)
    fqdn = StringType(required=True)
    state = StringType(required=True)
    vs = ListType(ModelType(VirtualServer))


class RealServerState(Model):
    fwmark = IntType(required=True)
    description = StringType(required=True)
    timestamp = DateTimeType(required=True)
    state = StringType(required=True)
    lb = ModelType(LoadBalancer, required=True)
    rs = ModelType(RealServer, required=True)


class L3ManagerClient:

    def __init__(self, base_url, token, timeout, retry_temporary_errors=None):
        self.__client = _ApiClient(base_url, extra_headers={"Authorization": "OAuth {}".format(token)}, timeout=timeout)
        self.__client._set_json_requests(False)
        self.retry_temporary_errors = retry_temporary_errors

    @retry_exceptions
    def list_balancers(self, params={"_limit": DEFAULT_LIMIT}) -> List[LoadBalancer]:
        class Result(Model):
            objects = ListType(ModelType(LoadBalancer), required=True)
        return self.__client.get("/balancer",
                                 params=params,
                                 model=Result,
                                 retry_temporary_errors=self.retry_temporary_errors).objects

    @retry_exceptions
    def list_services(self, params={"_limit": DEFAULT_LIMIT}) -> List[Service]:
        class Result(Model):
            objects = ListType(ModelType(Service), required=True)
        return self.__client.get("/service",
                                 params=params,
                                 model=Result,
                                 retry_temporary_errors=self.retry_temporary_errors).objects

    @retry_exceptions
    def list_service_configurations(self, service_id, params={"_limit": DEFAULT_LIMIT}) -> List[Configuration]:
        class Result(Model):
            objects = ListType(ModelType(Configuration), required=True)
        return self.__client.get("/service/{}/config".format(service_id),
                                 params=params,
                                 model=Result,
                                 retry_temporary_errors=self.retry_temporary_errors).objects

    @retry_exceptions
    def list_service_virtual_servers(self, service_id, params={"_limit": DEFAULT_LIMIT}) -> List[VirtualServer]:
        class Result(Model):
            objects = ListType(ModelType(VirtualServer), required=True)
        return self.__client.get("/service/{}/vs".format(service_id),
                                 params=params,
                                 model=Result,
                                 retry_temporary_errors=self.retry_temporary_errors).objects

    @retry_exceptions
    def get_service(self, service_id) -> Service:
        return self.__client.get("/service/{}".format(service_id),
                                 model=Service,
                                 retry_temporary_errors=self.retry_temporary_errors)

    @retry_exceptions
    def get_service_configuration(self, service_id, configuration_id) -> Configuration:
        return self.__client.get("/service/{}/config/{}".format(service_id, configuration_id),
                                 model=Configuration,
                                 retry_temporary_errors=self.retry_temporary_errors)

    @retry_exceptions
    def get_real_server_states(self, virtual_server_id, params={"_limit": DEFAULT_LIMIT}) -> List[RealServerState]:
        class Result(Model):
            objects = ListType(ModelType(RealServerState), required=True)
        return self.__client.get("/vs/{}/rsstate".format(virtual_server_id),
                                 params=params,
                                 model=Result,
                                 retry_temporary_errors=self.retry_temporary_errors).objects

    def create_service_configuration(self, service_id, configuration_params) -> IntType:
        """
        :return: configuration_id
        """
        response = self.__client.post("/service/{}/config".format(service_id),
                                      configuration_params,
                                      retry_temporary_errors=self.retry_temporary_errors)
        return response["object"]["id"]

    def deploy_service_configuration(self, service_id, configuration_id, force=False) -> IntType:
        """
        :return: configuration_id
        """
        params = {}
        if force:
            params["force"] = "force"
        response = self.__client.post("/service/{}/config/{}/process".format(service_id, configuration_id),
                                      params,
                                      retry_temporary_errors=self.retry_temporary_errors)
        return response["object"]["id"]

    def replace_service_reals(self, service_id, new_reals) -> IntType:
        """
        Creates new configuration with specified reals and returns its ID.
        Warning: reals of ALL virtual servers will be replaced with new_reals.
        :return: configuration_id
        """
        params = {"group": new_reals}
        response = self.__client.post("/service/{}/editrs".format(service_id),
                                      params,
                                      retry_temporary_errors=self.retry_temporary_errors)
        return response["object"]["id"]


def get_l3manager_client(retry_temporary_errors=True) -> L3ManagerClient:
    conf = config.get_value("endpoints.l3manager", model=L3ManagerEndpointConfig)
    return L3ManagerClient(base_url=conf.base_url,
                           token=conf.token,
                           timeout=conf.timeout,
                           retry_temporary_errors=retry_temporary_errors)
