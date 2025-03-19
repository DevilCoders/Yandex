#####################################################################################################################################
# DO NOT MODIFY THIS FILE, IT'S SYNCHRONIZED AUTOMATICALLY!                                                                         #
# Copied to Arcadia by https://bb.yandex-team.ru/projects/CLOUD/repos/iam-access-service-client-python/browse/tools/arcadia_sync.sh #
#####################################################################################################################################
import collections

import grpc


class YCAccessServiceException(Exception):
    def __init__(self, original_exception):
        super(YCAccessServiceException, self).__init__(original_exception.details())
        self._original_exception = original_exception


class BadRequestException(YCAccessServiceException):
    pass


class PermissionDeniedException(YCAccessServiceException):
    pass


class UnauthenticatedException(YCAccessServiceException):
    pass


class CancelledException(YCAccessServiceException):
    pass


class TimeoutException(YCAccessServiceException):
    pass


class UnavailableException(YCAccessServiceException):
    pass


class UnimplementedException(YCAccessServiceException):
    pass


class InternalException(YCAccessServiceException):
    pass


_grpc_code_to_exception_mapping = collections.defaultdict(
    lambda: YCAccessServiceException,  # default
    {
        grpc.StatusCode.INVALID_ARGUMENT: BadRequestException,
        grpc.StatusCode.CANCELLED: CancelledException,
        grpc.StatusCode.DEADLINE_EXCEEDED: TimeoutException,
        grpc.StatusCode.PERMISSION_DENIED: PermissionDeniedException,
        grpc.StatusCode.UNIMPLEMENTED: UnimplementedException,
        grpc.StatusCode.INTERNAL: InternalException,
        grpc.StatusCode.UNAVAILABLE: UnavailableException,
        grpc.StatusCode.UNAUTHENTICATED: UnauthenticatedException,
    }
)
