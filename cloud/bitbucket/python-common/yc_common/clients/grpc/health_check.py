from typing import List

from yandex.cloud.priv.healthcheck.v1alpha.inner import (
    health_check_group_service_pb2 as hcg_service,
    health_check_group_service_pb2_grpc as hcg_service_grpc,
)

from yc_common import config, logging
from yc_common.clients.grpc.base import BaseGrpcClient, GrpcClientCredentials, GrpcEndpointConfig, TLSConfig
from yc_common.clients.models.grpc.health_check_group import HealthCheckGroup, HealthCheckResult

log = logging.get_logger(__name__)

DEFAULT_TIMEOUT = 1
DEFAULT_RETRIES = 4
DEFAULT_BACKOFF_FACTOR = 0.1


class HealthCheckEndpointConfig(GrpcEndpointConfig):
    pass


class HealthCheckCtrlClient(BaseGrpcClient):
    service_stub_cls = hcg_service_grpc.HealthCheckGroupServiceStub

    def get_health_check_group(self, health_check_group_id) -> HealthCheckGroup:
        request = hcg_service.GetHealthCheckGroupRequest(health_check_group_id=health_check_group_id)
        health_check_group_grpc = self.reading_stub.Get(request)
        health_check_group = HealthCheckGroup.from_grpc(health_check_group_grpc)
        return health_check_group

    def upsert_health_check_group(self, health_check_group: HealthCheckGroup):
        request = hcg_service.UpsertHealthCheckGroupRequest(
            health_check_group_id=health_check_group.id,
            targets=[target.to_grpc() for target in health_check_group.targets],
            health_checks=[health_check.to_grpc() for health_check in health_check_group.health_checks],
        )
        self.writing_stub.Upsert(request)

    def delete_health_check_group(self, healthcheck_group_id: str):
        request = hcg_service.DeleteHealthCheckGroupRequest(health_check_group_id=healthcheck_group_id)
        self.writing_stub.Delete(request)

    def get_health(self, health_check_group_id: str) -> List[HealthCheckResult]:
        request = hcg_service.HealthRequest(health_check_group_id=health_check_group_id)
        response = self.reading_stub.GetHealth(request)
        return [HealthCheckResult.from_grpc(result) for result in response.health_check_result]


def get_health_check_ctrl_client(metadata=None):
    endpoint = config.get_value("endpoints.health_check_ctrl.url")
    timeout = config.get_value("endpoints.health_check_ctrl.timeout", default=DEFAULT_TIMEOUT)
    retries = config.get_value("endpoints.health_check_ctrl.retries", default=DEFAULT_RETRIES)
    backoff_factor = config.get_value("endpoints.health_check_ctrl.backoff_factor", default=DEFAULT_BACKOFF_FACTOR)
    tls = config.get_value("endpoints.health_check_ctrl.tls", model=TLSConfig, default=TLSConfig.new(enabled=False))

    return HealthCheckCtrlClient(
        endpoint=endpoint,
        timeout=timeout,
        retries=retries,
        backoff_factor=backoff_factor,
        metadata=metadata,
        tls=GrpcClientCredentials(
            enabled=tls.enabled,
            cert_file=tls.cert_file,
            cert_private_key_file=tls.cert_private_key_file,
            root_certs_file=tls.root_certs_file,
        ),
    )
