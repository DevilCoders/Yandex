from cloud.mdb.internal.python import grpcutil
from google.rpc import status_pb2
from grpc_status import rpc_status
from yandex.cloud.priv.compute.v1 import error_pb2

from cloud.mdb.internal.python.grpcutil import exceptions


class ComputeGRPCService(grpcutil.WrappedGRPCService):
    exposable_types = ['QuotaExceeded', 'AddressSpaceExhausted']

    def specific_errors(self, rich_details: status_pb2.Status) -> exceptions.GRPCError:
        for detail in rich_details:
            self.logger.debug('Parsing error details: "%s"', detail)
            if detail.Is(error_pb2.ErrorDetails.DESCRIPTOR):
                error = error_pb2.ErrorDetails()
                detail.Unpack(error)
                self.logger.debug('Unpacked details into error "%s"', error)
                if any(op_type in error.type for op_type in self.exposable_types):
                    return exceptions.ResourceExhausted(
                        logger=self.logger, message=error.message, err_type=error.type, code=error.code
                    )
                result = self.code_based_errors(
                    rpc_status.code_to_grpc_status_code(error.code), error.message, error.type
                )
                if result:
                    return result
                return exceptions.GRPCError(
                    logger=self.logger,
                    message=error.message,
                    err_type=error.type,
                    code=error.code,
                )
