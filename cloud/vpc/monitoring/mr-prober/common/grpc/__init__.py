import collections
import logging
import textwrap
import time
import uuid
from typing import Sequence, Tuple, Optional

import grpc

import settings

logger = logging.getLogger(__name__)


class YcGrpcSecureChannel:
    """
    Basic Yandex Cloud GRPC TLS Channel.
    See https://bb.yandex-team.ru/projects/CLOUD/repos/python-common/browse/yc_common/clients/grpc/base.py
    for future inspirations.
    """

    def __init__(self, endpoint: str, iam_token: str):
        self._endpoint = endpoint
        self._iam_token = iam_token
        self._channel = None

    def __enter__(self) -> grpc.Channel:
        root_certificates = settings.GRPC_ROOT_CERTIFICATES_PATH.read_bytes()
        credentials = grpc.ssl_channel_credentials(root_certificates)
        self._channel = grpc.secure_channel(self._endpoint, credentials)

        metadata = (
            ("authorization", f"Bearer {self._iam_token}"),
            ("x-client-request-id", str(uuid.uuid4())),
        )
        self._channel = grpc.intercept_channel(self._channel, TimeLoggingInterceptor())
        self._channel = grpc.intercept_channel(self._channel, MetadataInterceptor(metadata))

        return self._channel

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self._channel is not None:
            try:
                self._channel.close()
            except Exception as e:
                logger.warning(f"Can't close grpc channel: {e}", exc_info=e)


class _ClientCallDetails(
    collections.namedtuple("_ClientCallDetails", ("method", "timeout", "metadata", "credentials")),
    grpc.ClientCallDetails
):
    pass


def _get_field_from_metadata(metadata: Sequence[Tuple[str, str]], key: str, default: Optional[str] = None) -> str:
    for name, value in metadata:
        if name.lower() == key.lower():
            return value
    if default is None:
        raise KeyError(f"Can't find {key!r} in metadata for grpc request: {metadata!r}")
    return default


class MetadataInterceptor(grpc.UnaryUnaryClientInterceptor):
    """
    Interceptor for GRPC channel which adds provided metadata to existing one
    """

    def __init__(self, metadata: Sequence[Tuple[str, str]]):
        self.metadata = metadata

    def intercept_unary_unary(self, continuation, details, request):
        # details.metadata is actually a tuple, list or None at this point
        if details.metadata:
            metadata = list(details.metadata)
        else:
            metadata = []
        metadata.extend(self.metadata)

        new_details = _ClientCallDetails(
            details.method,
            details.timeout,
            metadata,
            details.credentials
        )

        return continuation(new_details, request)


class TimeLoggingInterceptor(grpc.UnaryUnaryClientInterceptor):
    def intercept_unary_unary(self, continuation, details, request):
        request_id = _get_field_from_metadata(details.metadata, "x-client-request-id", "<empty>")

        logger.debug(f"Sending grpc request \\[id={request_id}]: {details.method}")
        if str(request):
            logger.debug(f"Request parameters \\[id={request_id}]:\n{textwrap.indent(str(request).rstrip(), '  ')}")

        start_time = time.monotonic()
        try:
            return continuation(details, request)
        finally:
            duration = time.monotonic() - start_time
            logger.debug(
                f"Grpc request \\[id={request_id}] to {details.method} has been completed in {duration:.2f} seconds"
            )
