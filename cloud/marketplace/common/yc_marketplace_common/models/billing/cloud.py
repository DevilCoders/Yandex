from schematics import Model
from schematics.types import ListType
from schematics.types import ModelType
from schematics.types import StringType
from yc_common.clients.models import BasePublicModel


class CloudBatchRequest(Model):
    class CloudBatchRequestEntry(Model):
        cloud_id = StringType(required=True)
        dt = StringType(required=True)

    items = ListType(ModelType(CloudBatchRequestEntry))


class CloudBatchResponseEntry(Model):
    cloud_id = StringType(required=True)
    billing_account_id = StringType(required=False)
    dt = StringType(required=False)


class BindCloudAccountRequest(BasePublicModel):
    cloud_id = StringType(required=True)
    billing_account_id = StringType(required=True)
