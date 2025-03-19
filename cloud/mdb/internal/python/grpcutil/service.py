import uuid
from functools import wraps
from typing import Callable, Dict, Optional, Type

import grpc
from dbaas_common import retry
from google.rpc import status_pb2
from grpc._channel import _InactiveRpcError
from grpc_status import rpc_status

from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from .exceptions import (
    AlreadyExistsError,
    AuthenticationError,
    AuthorizationError,
    CancelledError,
    DeadlineExceededError,
    FailedPreconditionError,
    GRPCError,
    InternalError,
    NotFoundError,
    ResourceExhausted,
    TemporaryUnavailableError,
    UnparsedGRPCError,
)

CODE_TO_EXCEPTION = {
    grpc.StatusCode.NOT_FOUND: NotFoundError,
    grpc.StatusCode.ALREADY_EXISTS: AlreadyExistsError,
    grpc.StatusCode.DEADLINE_EXCEEDED: DeadlineExceededError,
    grpc.StatusCode.UNAUTHENTICATED: AuthenticationError,
    grpc.StatusCode.PERMISSION_DENIED: AuthorizationError,
    grpc.StatusCode.RESOURCE_EXHAUSTED: ResourceExhausted,
    grpc.StatusCode.UNAVAILABLE: TemporaryUnavailableError,
    grpc.StatusCode.CANCELLED: CancelledError,
    grpc.StatusCode.FAILED_PRECONDITION: FailedPreconditionError,
    grpc.StatusCode.INTERNAL: InternalError,
}


class WrappedGRPCService:
    def __init__(
        self,
        logger: MdbLoggerAdapter,
        channel,
        grpc_service: Callable,
        timeout: float,
        get_token: callable,
        error_handlers: Dict[Type[GRPCError], Callable[[GRPCError], None]],
    ):
        """
        :param error_handlers: raise your errors directly from handlers
        """
        self.logger = logger
        self.channel = channel
        self.service = grpc_service(channel)
        self.timeout = timeout
        self._get_token = get_token
        self.error_handlers = error_handlers

    def _get_metadata(self) -> dict:
        """
        Get gRPC authorization metadata
        """
        token = self._get_token()
        return {'authorization': f'Bearer {token}'}

    def specific_errors(self, rich_details) -> GRPCError:
        """
        Transform errors by details.
        """

    def code_based_errors(self, code: grpc.StatusCode, message: str, error_type: str) -> Optional[GRPCError]:
        """
        Transform errors which specific instance is not yet known to us.

        https://grpc.io/docs/guides/error/#error-status-codes
        """
        if code in CODE_TO_EXCEPTION:
            return CODE_TO_EXCEPTION[code](
                logger=self.logger,
                message=message,
                err_type=error_type,
                code=code.value[0],
            )

    _metadata_headers_renames = {
        'idempotency_key': 'idempotency-key',
        'request_id': 'x-request-id',
    }

    def rename_keys(self, metadata: dict) -> dict:
        """
        Rename for convenient calls like stub.Create(request, idempotency_key='my-key')
        """
        try:
            # Ignore referrer-id with None or '' value
            # Some gRPC clients pass it,
            # so call fails with:
            #   GRPCError: Request validation error: invalid referrer id
            referrer_id = metadata.pop('referrer_id')
            if referrer_id:
                metadata['referrer-id'] = referrer_id
        except KeyError:
            pass
        for var_name, header_name in self._metadata_headers_renames.items():
            if var_name not in metadata:
                continue
            metadata[header_name] = metadata.pop(var_name)

        return metadata

    def wrap_call(self, original_logger: MdbLoggerAdapter, rpc_call: Callable, rpc_call_name: str):
        service_name = 'unknown'
        try:
            service_name = self.service.__class__.__name__
        except Exception as exc:
            original_logger.warning('could not get name for %s: %s', self.service, exc)

        @wraps(rpc_call)
        @retry.on_exception(
            (grpc.RpcError, DeadlineExceededError, CancelledError, ResourceExhausted, TemporaryUnavailableError),
            factor=1,
            max_wait=5,
            max_tries=24,
        )
        def mdb_call_to_grpc_service(*args, timeout: float = None, **metadata):
            default_metadata = self._get_metadata()
            if metadata:
                default_metadata.update(metadata)
            metadata = default_metadata

            if 'request_id' not in metadata:
                metadata['request_id'] = str(uuid.uuid4())
            request_id = metadata['request_id']

            metadata = tuple(sorted(self.rename_keys(metadata).items()))
            timeout = timeout or self.timeout
            status_code = None

            logger = original_logger.copy_with_ctx(request_id=request_id)
            logger.info('Starting request to %s.%s', service_name, rpc_call_name)
            try:
                result = rpc_call(*args, metadata=metadata, timeout=timeout)
                status_code = grpc.StatusCode.OK
                return result
            except grpc.RpcError as exc:
                status_code = grpc.StatusCode.INTERNAL
                try:
                    rich_status = rpc_status.from_call(exc)
                except ValueError as parse_exc:
                    logger.error(
                        'Failed to parse error from %s.%s: "%s"',
                        service_name,
                        rpc_call_name,
                        parse_exc,
                    )
                else:
                    if rich_status:
                        status_code = rpc_status.to_status(rich_status).code
                        error = self.error_from_rich_status(logger, rich_status)
                        if error.__class__ in self.error_handlers:
                            self.error_handlers[error.__class__](error)
                        raise error
                if isinstance(exc, _InactiveRpcError):
                    status_code = exc.code()
                    error = self.code_based_errors(status_code, exc.debug_error_string(), status_code.name)
                    if error:
                        if error.__class__ in self.error_handlers:
                            self.error_handlers[error.__class__](error)
                        raise error
                logger.exception('Unhandled error "%s"', exc)
                raise  # grpc.RpcError
            finally:
                logger.info(
                    'Request to %s.%s finished with %s',
                    service_name,
                    rpc_call_name,
                    status_code,
                )

        return mdb_call_to_grpc_service

    def error_from_rich_status(self, logger: MdbLoggerAdapter, rich_status: status_pb2.Status) -> GRPCError:
        error = self.specific_errors(rich_status.details)
        if error:
            return error
        status_code = rpc_status.to_status(rich_status).code
        error = self.code_based_errors(status_code, rich_status.message, status_code.name)
        if error:
            return error
        logger.info('Could not parse GRPC error: "%s"', rich_status)
        return UnparsedGRPCError(
            logger=logger,
            message='error cannot be parsed',
            err_type=grpc.StatusCode.INTERNAL.name,
            code=grpc.StatusCode.INTERNAL.value[0],
        )

    def __getattr__(self, item):
        rpc_call = getattr(self.service, item)

        return self.wrap_call(self.logger, rpc_call, item)
