from yandex.cloud.priv.healthcheck.v1alpha.inner import health_check_group_pb2 as hcg

from schematics.exceptions import ValidationError
from schematics.types import StringType, ListType, ModelType, IntType, BaseType
from yc_common import logging
from yc_common.clients.models.grpc.common import GrpcModel, GrpcTypeMixin, QuasiIpAddress, BaseLoadBalancerGrpcModel
from yc_common.clients.models.target_groups import MIN_HC_INTERVAL, MIN_HC_TIMEOUT, MAX_HC_INTERVAL, MAX_HC_TIMEOUT, \
    MIN_HC_HEALTHY_THRESHOLD, MAX_HC_HEALTHY_THRESHOLD, MAX_HC_UNHEALTHY_THRESHOLD, MIN_HC_UNHEALTHY_THRESHOLD
from yc_common.validation import LBPortType


log = logging.get_logger(__name__)


class TcpOptions(GrpcModel):
    port = LBPortType(required=True)

    @classmethod
    def get_grpc_class(cls):
        return hcg.HealthCheck.TcpOptions

    @property
    def frozen(self):
        return (self.port,)


class HttpOptions(GrpcModel):
    port = LBPortType(required=True)
    path = StringType()  # TODO(lavrukov): maybe regex validation?

    @classmethod
    def get_grpc_class(cls):
        return hcg.HealthCheck.HttpOptions

    @property
    def frozen(self):
        return (self.port, self.path)


class HealthCheck(GrpcModel):
    name = StringType(required=True)
    interval = IntType(min_value=MIN_HC_INTERVAL * 10**3, max_value=MAX_HC_INTERVAL * 10**3)
    timeout = IntType(min_value=MIN_HC_TIMEOUT * 10**3, max_value=MAX_HC_TIMEOUT * 10**3)
    healthy_threshold = IntType(min_value=MIN_HC_HEALTHY_THRESHOLD, max_value=MAX_HC_HEALTHY_THRESHOLD)
    unhealthy_threshold = IntType(min_value=MIN_HC_UNHEALTHY_THRESHOLD, max_value=MAX_HC_UNHEALTHY_THRESHOLD)
    tcp_options = ModelType(TcpOptions)
    http_options = ModelType(HttpOptions)

    @classmethod
    def get_grpc_class(cls):
        return hcg.HealthCheck

    def validate(self, **kwargs):
        super().validate(**kwargs)

        options = [self.tcp_options, self.http_options]
        not_none_options = [option for option in options if option is not None]
        if len(not_none_options) != 1:
            raise ValidationError("Only one options field is allowed.")

        if self.interval - self.timeout < 10**9:
            raise ValidationError("Health check interval must be greater than timeout at least 1 second.")

    @property
    def frozen(self):
        return (
            self.name,
            self.interval,
            self.timeout,
            self.healthy_threshold,
            self.unhealthy_threshold,
            self.tcp_options.frozen if self.tcp_options is not None else None,
            self.http_options.frozen if self.http_options is not None else None,
        )


class HealthCheckGroupTarget(GrpcModel):
    zone_id = StringType()
    address = ModelType(QuasiIpAddress)

    @classmethod
    def get_grpc_class(cls):
        return hcg.HealthCheckGroup.Target

    @property
    def frozen(self):
        return (self.zone_id, self.address.ipv6)


class HealthCheckGroup(BaseLoadBalancerGrpcModel):
    id = StringType(required=True)
    targets = ListType(ModelType(HealthCheckGroupTarget), default=list, required=True)
    health_checks = ListType(ModelType(HealthCheck), default=list, required=True)

    @classmethod
    def get_grpc_class(cls):
        return hcg.HealthCheckGroup

    @property
    def frozen(self):
        return (
            self.id,
            frozenset(t.frozen for t in self.targets),
            frozenset(h.frozen for h in self.health_checks),
        )


class HealthCheckStatus:
    NOT_SHIPPED = "not_shipped"
    NOT_CHECKED = "not_checked"
    NOT_CHECKING = "not_checking"
    OK = "ok"
    FAIL_TIMEOUT = "fail_timeout"
    FAIL_NETWORK_UNREACHABLE = "fail_network_unreachable"
    FAIL_CONNECTION_RESET = "fail_connection_reset"
    FAIL_HTTP_5XX = "fail_http_5xx"
    FAIL_UNSPECIFIED = "fail_unspecified"

    FAILED = [FAIL_TIMEOUT, FAIL_NETWORK_UNREACHABLE, FAIL_CONNECTION_RESET, FAIL_HTTP_5XX, FAIL_UNSPECIFIED]
    ALL = [OK, NOT_CHECKED] + FAILED

    _STR_TO_INT_MAP = {
        NOT_CHECKED: 1,
        OK: 2,
        FAIL_TIMEOUT: 3,
        FAIL_NETWORK_UNREACHABLE: 4,
        FAIL_CONNECTION_RESET: 5,
        FAIL_HTTP_5XX: 6,
        FAIL_UNSPECIFIED: 7,
        NOT_CHECKING: 8,
        NOT_SHIPPED: 9,
    }
    _INT_TO_STR_MAP = {value: key for key, value in _STR_TO_INT_MAP.items()}


class HealthCheckStatusType(BaseType, GrpcTypeMixin):
    primitive_type = int
    native_type = str

    MESSAGES = {
        'convert': "Couldn't convert '{0}' value to status type",
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, choices=HealthCheckStatus.ALL, **kwargs)

    def convert_grpc(self, value, context=None):
        return value.lower()

    def to_grpc(self, value, context=None):
        return HealthCheckStatus._STR_TO_INT_MAP[value]


class HealthCheckResult(GrpcModel):
    health_check_group_id = StringType()
    name = StringType()
    address = ModelType(QuasiIpAddress)
    status = HealthCheckStatusType()

    @classmethod
    def get_grpc_class(cls):
        return hcg.HealthCheckResult
