from google.rpc import status_pb2
from yandex.cloud.priv.compute.v1 import error_pb2
from cloud.mdb.internal.python.grpcutil import exceptions
from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from .errors import DuplicatePlacementGroupNameError


class PlacementGroupsGRPCService(ComputeGRPCService):
    def specific_errors(self, rich_details: status_pb2.Status) -> exceptions.GRPCError:
        for detail in rich_details:
            self.logger.debug('Parsing error details: "%s"', detail)
            if detail.Is(error_pb2.ErrorDetails.DESCRIPTOR):
                error = error_pb2.ErrorDetails()
                detail.Unpack(error)
                self.logger.debug('Unpacked details into error "%s"', error)
                if error.type == DuplicatePlacementGroupNameError.compute_type:
                    return DuplicatePlacementGroupNameError(
                        logger=self.logger, message=error.message, err_type=error.type, code=error.code
                    )
        return super().specific_errors(rich_details)
