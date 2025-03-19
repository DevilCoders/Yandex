from schematics.types import ModelType
from schematics.types import StringType
from yc_common.clients.models.operations import ErrorV1Beta1
from yc_common.clients.models.operations import OperationV1Beta1


# TODO do not use
class BillingOperation(OperationV1Beta1):
    class ErrorModel(ErrorV1Beta1):
        """"""
        code = StringType()
        message = StringType()

    error = ModelType(ErrorModel)
