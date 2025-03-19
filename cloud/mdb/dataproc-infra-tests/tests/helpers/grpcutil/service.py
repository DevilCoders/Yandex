# TODO: may be we should replace this with cloud.mdb.internal.python.grpcutil.service?
import json
import logging
import uuid
from functools import wraps
from typing import Callable, Optional

import grpc
from retrying import retry

from .exceptions import (
    AuthenticationError,
    AuthorizationError,
    CancelledError,
    DeadlineExceededError,
    GRPCError,
    NotFoundError,
    ResourceExhausted,
    TemporaryUnavailableError,
    UnparsedGRPCError,
)

CODE_TO_EXCEPTION = {
    grpc.StatusCode.NOT_FOUND: NotFoundError,
    grpc.StatusCode.DEADLINE_EXCEEDED: DeadlineExceededError,
    grpc.StatusCode.UNAUTHENTICATED: AuthenticationError,
    grpc.StatusCode.PERMISSION_DENIED: AuthorizationError,
    grpc.StatusCode.RESOURCE_EXHAUSTED: ResourceExhausted,
    grpc.StatusCode.UNAVAILABLE: TemporaryUnavailableError,
    grpc.StatusCode.CANCELLED: CancelledError,
}


class WrappedGRPCService:
    def __init__(
        self,
        logger: logging.Logger,
        channel,
        grpc_service: Callable,
        timeout: float,
        get_token: callable,
    ):
        self.logger = logger
        self.channel = channel
        self.service = grpc_service(channel)
        self.timeout = timeout
        self._get_token = get_token

    def _get_metadata(self) -> dict:
        """
        Get gRPC authorization metadata
        """
        token = self._get_token()
        return {'authorization': f'Bearer {token}'}

    def code_based_errors(self, code: grpc.StatusCode, message: str) -> Optional[GRPCError]:
        """
        Transform errors which specific instance is not yet known to us.

        https://grpc.io/docs/guides/error/#error-status-codes
        """
        if code in CODE_TO_EXCEPTION:
            return CODE_TO_EXCEPTION[code](
                logger=self.logger,
                message=message,
                err_type=code.name,
                code=code.value[0],
            )

    def wrap_call(self, logger: logging.Logger, rpc_call: Callable, rpc_call_name: str):
        service_name = 'unknown'
        try:
            service_name = self.service.__class__.__name__
        except Exception as exc:
            logger.warning('could not get name for %s: %s', self.service, exc)

        @retry(
            retry_on_exception=lambda exc: isinstance(
                exc,
                (
                    DeadlineExceededError,
                    AuthenticationError,
                    TemporaryUnavailableError,
                ),
            ),
            stop_max_attempt_number=6,
            wait_exponential_multiplier=1000,
            wait_exponential_max=60000,
        )
        @wraps(rpc_call)
        def mdb_call_to_grpc_service(*args, timeout: float = None, **metadata):
            default_metadata = self._get_metadata()
            if metadata:
                default_metadata.update(metadata)
            metadata = default_metadata

            request_id = metadata.get('x-request-id', str(uuid.uuid4()))
            metadata['x-request-id'] = request_id

            if metadata.get('referrer_id'):
                # rename key. 'referrer_id' header is not valid
                metadata['referrer-id'] = metadata.pop('referrer_id')

            metadata = tuple(metadata.items())

            timeout = timeout or self.timeout
            status_code = grpc.StatusCode.OK

            logger.debug(f'Starting request to {service_name}.{rpc_call_name} (request_id={request_id})')
            try:
                return rpc_call(*args, metadata=metadata, timeout=timeout)
            except grpc.RpcError as exc:
                status_code = exc.code()
                message = None
                details = exc.details()
                if details.startswith("{") and details.endswith("}"):
                    try:
                        d = json.loads(details)
                        message = d['Message']
                    except RuntimeError:
                        pass
                else:
                    message = details

                error = self.code_based_errors(status_code, message)
                if error:
                    raise error

                logger.info('Could not parse GRPC error: "%s"', exc)
                error = UnparsedGRPCError(
                    logger=logger,
                    message='error cannot be parsed',
                    err_type=status_code.name,
                    code=status_code,
                )
                raise error
            finally:
                logger.debug(
                    'Request to %s.%s finished with %s',
                    service_name,
                    rpc_call_name,
                    status_code,
                )

        return mdb_call_to_grpc_service

    def __getattr__(self, item):
        rpc_call = getattr(self.service, item)

        return self.wrap_call(self.logger, rpc_call, item)
