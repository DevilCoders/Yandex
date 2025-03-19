from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from yc_common.models import IntType
from yc_common.models import ListType
from yc_common.models import Model
from yc_common.models import ModelType
from yc_common.models import StringType
from yc_common.models import get_model_options


class ComputePlatform:
    STANDARD_V1 = "standard-v1"
    STANDARD_V2 = "standard-v2"
    GPU_STANDARD_V1 = "gpu-standard-v1"
    ALL = {STANDARD_V1, STANDARD_V2, GPU_STANDARD_V1}


class NetworkInterfaceRequirements(Model):
    min = IntType()
    max = IntType()
    default = IntType()

    Options = get_model_options(public_api_fields=(
        "min",
        "max",
        "default",
    ))


class BillingAccountRequirements(Model):
    usage_status = ListType(StringType)

    Options = get_model_options(public_api_fields=(
        "usage_status",
    ))


class ResourceSpec(Model):
    memory = IntType()
    cores = IntType()
    disk_size = IntType()
    compute_platforms = ListType(StringType(choices=ComputePlatform.ALL))
    user_data_form_id = StringType(required=True, default="linux")
    network_interfaces = ModelType(NetworkInterfaceRequirements)
    service_account_roles = ListType(StringType())
    billing_account_requirements = ModelType(BillingAccountRequirements)

    def __init__(self, *args, strict=False, **kwargs):
        super().__init__(*args, **kwargs, strict=False)

    Options = get_model_options(public_api_fields=(
        "memory",
        "cores",
        "disk_size",
        "compute_platforms",
        "user_data_form_id",
        "network_interfaces",
        "service_account_roles",
        "billing_account_requirements",
    ))


class BillingAccountRequirementsPublicModel(MktBasePublicModel):
    usage_status = ListType(StringType)
    Options = get_model_options(public_api_fields=(
        "usage_status",
    ))


class ResourceSpecPublicModel(MktBasePublicModel):
    memory = IntType()
    cores = IntType()
    disk_size = IntType()
    compute_platforms = ListType(StringType(choices=ComputePlatform.ALL))
    user_data_form_id = StringType(required=True, default="linux")
    network_interfaces = ModelType(NetworkInterfaceRequirements)
    service_account_roles = ListType(StringType())
    billing_account_requirements = ModelType(BillingAccountRequirementsPublicModel)

    Options = get_model_options(public_api_fields=(
        "memory",
        "cores",
        "disk_size",
        "compute_platforms",
        "user_data_form_id",
        "network_interfaces",
        "service_account_roles",
        "billing_account_requirements"
    ))
