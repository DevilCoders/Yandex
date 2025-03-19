from yc_common.models import IntType
from yc_common.models import Model
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.validation import ResourceIdType
from cloud.marketplace.queue.yc_marketplace_queue.types.export import ExportDestinationType


class ExportTablesParams(Model):
    timeout = IntType()
    export_destination = StringEnumType(choices=ExportDestinationType.ALL, required=True)


class ExportTableLbParams(Model):
    table_name = StringType(required=True)


class BuildBlueprintsParams(Model):
    timeout = IntType()
    publisher_account_id = ResourceIdType(required=True)
