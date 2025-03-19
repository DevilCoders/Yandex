from schematics.exceptions import ValidationError
from schematics.types import ModelType, StringType, ListType, IntType

from yc_common.clients.models import base as base_models, labels as compute_labels
from yc_common.models import IPAddressType
from yc_common.clients.models.grpc.common import GrpcModelMixin
from yc_common.validation import (
    RegionId, RegionIdType, ResourceDescriptionType, ResourceIdType,
    ResourceNameType, LBPortType, UrlQueryPathType, ZoneIdType,
)

from . import operations

MIN_HC_INTERVAL = 2  # in seconds
MAX_HC_INTERVAL = 300  # in seconds
DEFAULT_HC_INTERVAL = 2
MIN_HC_TIMEOUT = 1  # in seconds
MAX_HC_TIMEOUT = 60  # in seconds
DEFAULT_HC_TIMEOUT = 1
MIN_HC_HEALTHY_THRESHOLD = 2
MAX_HC_HEALTHY_THRESHOLD = 10
DEFAULT_HC_HEALTHY_THRESHOLD = 2
MIN_HC_UNHEALTHY_THRESHOLD = 2
MAX_HC_UNHEALTHY_THRESHOLD = 10
DEFAULT_HC_UNHEALTHY_THRESHOLD = 2


class Target(base_models.BasePublicModel, GrpcModelMixin):
    subnet_id = ResourceIdType(required=True)
    address = IPAddressType(required=True)

    def __hash__(self):
        return hash(tuple(self.values()))

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.loadbalancer.v1.target_group_pb2 import Target
        return Target


class TargetGroup(base_models.RegionalPublicObjectModelV1Beta1, GrpcModelMixin):
    targets = ListType(ModelType(Target))
    network_load_balancer_ids = ListType(StringType())


class Duration(base_models.BasePublicModel, GrpcModelMixin):
    seconds = IntType()
    nanos = IntType(min_value=-999999999, max_value=999999999, default=0)  # TODO(lavrukov): test not div 1000000

    def to_nanoseconds(self):
        return self.seconds * 10**9 + self.nanos

    def to_milliseconds(self):
        return self.to_nanoseconds() // 10**6

    @classmethod
    def get_grpc_class(cls):
        from google.protobuf.duration_pb2 import Duration
        return Duration


class IntervalDuration(Duration):
    seconds = IntType(min_value=MIN_HC_INTERVAL, max_value=MAX_HC_INTERVAL, default=DEFAULT_HC_INTERVAL)


class TimeoutDuration(Duration):
    seconds = IntType(min_value=MIN_HC_TIMEOUT, max_value=MAX_HC_TIMEOUT, default=DEFAULT_HC_TIMEOUT)


class TcpOptionsPublic(base_models.BasePublicModel, GrpcModelMixin):
    port = LBPortType(required=True)

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.loadbalancer.v1.health_check_pb2 import HealthCheck
        return HealthCheck.TcpOptions


class HttpOptionsPublic(base_models.BasePublicModel, GrpcModelMixin):
    port = LBPortType(required=True)
    path = UrlQueryPathType(required=True, default="/")

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.loadbalancer.v1.health_check_pb2 import HealthCheck
        return HealthCheck.HttpOptions


class HealthCheckPublic(base_models.BasePublicModel, GrpcModelMixin):
    name = ResourceNameType(required=True, min_length=3)
    interval = ModelType(IntervalDuration, default=IntervalDuration())
    timeout = ModelType(TimeoutDuration, default=TimeoutDuration())
    healthy_threshold = IntType(
        min_value=MIN_HC_HEALTHY_THRESHOLD,
        max_value=MAX_HC_HEALTHY_THRESHOLD,
        default=DEFAULT_HC_HEALTHY_THRESHOLD,
    )
    unhealthy_threshold = IntType(
        min_value=MIN_HC_UNHEALTHY_THRESHOLD,
        max_value=MAX_HC_UNHEALTHY_THRESHOLD,
        default=DEFAULT_HC_UNHEALTHY_THRESHOLD,
    )
    tcp_options = ModelType(TcpOptionsPublic)
    http_options = ModelType(HttpOptionsPublic)

    def validate_http_options(self, data, http_options):
        tcp_options = data["tcp_options"]
        options = [tcp_options, http_options]
        not_none_options = [option for option in options if option is not None]
        if len(not_none_options) != 1:
            raise ValidationError("Only one options field is allowed.")

    def validate_timeout(self, data, timeout):
        interval = data["interval"]
        if not all(isinstance(x, Duration) for x in [interval, timeout]):
            # Validation failed
            return
        if interval.to_nanoseconds() - timeout.to_nanoseconds() < 10**9:
            raise ValidationError("Health check interval must be greater than timeout at least 1 second.")

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.loadbalancer.v1.health_check_pb2 import HealthCheck
        return HealthCheck


class AttachedTargetGroupPublic(base_models.BasePublicModel, GrpcModelMixin):
    target_group_id = ResourceIdType(required=True)
    health_checks = ListType(ModelType(HealthCheckPublic), min_size=1, max_size=1, default=list)

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.loadbalancer.v1.network_load_balancer_pb2 import AttachedTargetGroup
        return AttachedTargetGroup


class TargetGroupList(base_models.BaseListModel, GrpcModelMixin):
    target_groups = ListType(ModelType(TargetGroup), required=True, default=list)


class TargetGroupMetadata(operations.OperationMetadataV1Beta1, GrpcModelMixin):
    target_group_id = StringType(required=True)


class TargetGroupOperation(operations.OperationV1Beta1, GrpcModelMixin):
    metadata = ModelType(TargetGroupMetadata)


class TargetGroupSpec(base_models.BasePublicModel):
    name = ResourceNameType()
    region_id = RegionIdType(default=RegionId.RU_CENTRAL1)
    zone_id = ZoneIdType()
    description = ResourceDescriptionType()
    labels = compute_labels.LabelsType()

    targets = ListType(ModelType(Target))


class AttachedTargetGroupSpec(base_models.BasePublicModel):
    target_group_id = ResourceIdType()
    target_group_spec = ModelType(TargetGroupSpec)
    health_checks = ListType(ModelType(HealthCheckPublic), min_size=1, max_size=1, default=list)

    @classmethod
    def from_attached_target_group(cls, atg):
        return cls.new_from_model(atg, aliases={'target_group_spec': None})

    def validate_target_group_id(self, data, target_group_id):
        if (target_group_id is None) + (data['target_group_spec'] is None) != 1:
            raise ValidationError("Either targetGroupId or targetGroupSpec must be specified.")
        return target_group_id
