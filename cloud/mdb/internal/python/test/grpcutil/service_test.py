import logging
from typing import Callable

import grpc
import pytest
from google.rpc import code_pb2, status_pb2
from grpc._channel import _InactiveRpcError, _RPCState
from grpc_status import rpc_status

from cloud.mdb.internal.python.grpcutil import NotFoundError, WrappedGRPCService
from cloud.mdb.internal.python.grpcutil.exceptions import AuthenticationError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

logger = logging.getLogger(__name__)


def stub(mocked_call: Callable):
    class GRPCStub:
        def __init__(self, *args, **kwargs):
            pass

        def CallGRPCMethod(self, *args, **kwargs):
            return mocked_call(*args, **kwargs)

    return GRPCStub


def test_metadata_and_timeout(mocker):
    mocked_call = mocker.Mock()

    service = WrappedGRPCService(
        logger=MdbLoggerAdapter(logger, {}),
        channel=object(),
        grpc_service=stub(mocked_call),
        timeout=0,
        get_token=lambda *args: 'test-token',
        error_handlers={},
    )
    mocker.patch('cloud.mdb.internal.python.grpcutil.service.uuid.uuid4', return_value='mdb-req-id')
    service.CallGRPCMethod('test-request')
    mocked_call.assert_called_with(
        'test-request', metadata=(('authorization', 'Bearer test-token'), ('x-request-id', 'mdb-req-id')), timeout=0
    )


def test_idempotency_key_transformed(mocker):
    mocked_call = mocker.Mock()

    service = WrappedGRPCService(
        logger=MdbLoggerAdapter(logger, {}),
        channel=object(),
        grpc_service=stub(mocked_call),
        timeout=0,
        get_token=lambda *args: 'test-token',
        error_handlers={},
    )
    mocker.patch('cloud.mdb.internal.python.grpcutil.service.uuid.uuid4', return_value='mdb-req-id')
    service.CallGRPCMethod('test-request', idempotency_key='test-key')
    mocked_call.assert_called_with(
        'test-request',
        metadata=(
            ('authorization', 'Bearer test-token'),
            ('idempotency-key', 'test-key'),
            ('x-request-id', 'mdb-req-id'),
        ),
        timeout=0,
    )


def test_explicit_request_id(mocker):
    mocked_call = mocker.Mock()

    service = WrappedGRPCService(
        logger=MdbLoggerAdapter(logger, {}),
        channel=object(),
        grpc_service=stub(mocked_call),
        timeout=0,
        get_token=lambda *args: 'test-token',
        error_handlers={},
    )
    service.CallGRPCMethod('test-request', request_id='explicit-request-id')
    mocked_call.assert_called_with(
        'test-request',
        metadata=(
            ('authorization', 'Bearer test-token'),
            ('x-request-id', 'explicit-request-id'),
        ),
        timeout=0,
    )


def test_ignore_metadata_with_none_referrer(mocker):
    mocked_call = mocker.Mock()

    service = WrappedGRPCService(
        logger=MdbLoggerAdapter(logger, {}),
        channel=object(),
        grpc_service=stub(mocked_call),
        timeout=0,
        get_token=lambda *args: 'test-token',
        error_handlers={},
    )
    service.CallGRPCMethod('test-request', referrer_id=None, request_id='explicit-request-id')
    mocked_call.assert_called_with(
        'test-request',
        metadata=(
            ('authorization', 'Bearer test-token'),
            ('x-request-id', 'explicit-request-id'),
        ),
        timeout=0,
    )


def test_called_once(mocker):
    mocked_call = mocker.Mock()

    service = WrappedGRPCService(
        logger=MdbLoggerAdapter(logger, {}),
        channel=object(),
        grpc_service=stub(mocked_call),
        timeout=0,
        get_token=lambda *args: 'test-token',
        error_handlers={},
    )
    service.CallGRPCMethod('test-request')
    mocked_call.assert_called_once()


def not_found_error():
    status = status_pb2.Status(
        code=code_pb2.NOT_FOUND,
        message='not found',
        details=[],
    )
    err = grpc.RpcError()
    err.code = lambda *args, **kwargs: rpc_status.to_status(status).code
    err.details = lambda *args, **kwargs: status.message
    err.trailing_metadata = lambda *args, **kwargs: [
        ('grpc-status-details-bin', status.SerializeToString()),
    ]
    return err


def test_error_handlers_work(mocker):
    class ExpectedError(Exception):
        pass

    def raise_some_error(*args, **kwargs):
        raise ExpectedError

    error_handler = mocker.Mock()
    error_handler.side_effect = raise_some_error

    handlers = {
        NotFoundError: error_handler,
    }

    def mocked_call(*args, **kwargs):
        raise not_found_error()

    service = WrappedGRPCService(
        logger=MdbLoggerAdapter(logger, {}),
        channel=object(),
        grpc_service=stub(mocked_call),
        timeout=0,
        get_token=lambda *args: 'test-token',
        error_handlers=handlers,
    )
    with pytest.raises(ExpectedError):
        service.CallGRPCMethod('test-request')
    error_handler.assert_called_once()
    called_with = error_handler.call_args_list[0][0]
    assert isinstance(called_with[0], NotFoundError)


def test_auth_error_handled(mocker):
    patched = mocker.patch.object(_InactiveRpcError, 'debug_error_string')

    def mocked_call(*args, **kwargs):
        state = _RPCState(
            due=[],
            initial_metadata=None,
            trailing_metadata=None,
            code=grpc.StatusCode.UNAUTHENTICATED,
            details=[],
        )
        mocked_error = _InactiveRpcError(state)
        mocked_error.debug_error_string.side_effect = lambda *args, **kwargs: 'expected text'
        raise mocked_error

    service = WrappedGRPCService(
        logger=MdbLoggerAdapter(logger, {}),
        channel=object(),
        grpc_service=stub(mocked_call),
        timeout=0,
        get_token=lambda *args: 'test-token',
        error_handlers={},
    )
    with pytest.raises(AuthenticationError) as exc_info:
        service.CallGRPCMethod('test-request')
    assert exc_info.value.message == 'expected text'
    patched.assert_called_once()
