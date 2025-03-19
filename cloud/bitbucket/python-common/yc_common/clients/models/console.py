"""Console API models"""

from yc_common.models import Model
from yc_common.clients.models import BasePublicModel
from yc_common.fields import ModelType, ListType, StringType, IntType, DictType, SchemalessDictType, BooleanType


class AllowedConfiguration(BasePublicModel):
    cores = DictType(ListType(IntType()), key=IntType(), required=True)
    software_accelerated_network_cores = ListType(IntType())
    memory_per_core = ListType(IntType(), required=True)
    max_memory = IntType()


class AllowedGpuConfiguration(BasePublicModel):
    cores = IntType(required=True)


class Platform(BasePublicModel):
    id = StringType(required=True)
    default = BooleanType()
    allowed_configurations = DictType(ModelType(AllowedConfiguration), key=IntType(), required=True)
    allowed_gpu_configurations = DictType(ModelType(AllowedGpuConfiguration), key=IntType())


class PlatformsList(BasePublicModel):
    platforms = ListType(ModelType(Platform), required=True)


class BillingMetric(Model):
    version = StringType(required=True)
    schema = StringType(required=True)

    folder_id = StringType(required=True)
    tags = SchemalessDictType(required=True)


class BillingMetricsListModel(Model):
    metrics = ListType(ModelType(BillingMetric), required=True)


class FolderStats(Model):
    instance_count = IntType()
