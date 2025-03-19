from grpc._channel import _InactiveRpcError
import grpc
import logging
from tests.helpers.grpcutil.service import CODE_TO_EXCEPTION


class BaseCluster:
    def __init__(self, context):
        self.context = context

    def _make_request(self, method, request, rewrite_context_with_response=True):
        try:
            if hasattr(method, '__name__') and method.__name__ == 'mdb_call_to_grpc_service':
                # WrappedGRPCService is ugly and breaks grpc call signature (**metadata instead of metadata argument)
                # but it's widely used and it's easier to make this hack
                response = method(request, **dict(self._get_meta()))
            else:
                # real grpc service
                response = method(request, metadata=self._get_meta())
        except grpc.RpcError as exc:
            if isinstance(exc, _InactiveRpcError):
                status_code = exc.code()
                if status_code in CODE_TO_EXCEPTION:
                    noopLogger = logging.getLogger()
                    noopLogger.setLevel(logging.ERROR)
                    new_exc = CODE_TO_EXCEPTION[status_code](
                        logger=noopLogger,
                        message=exc.debug_error_string(),
                        err_type=status_code.name,
                        code=status_code.value[0],
                    )
                    raise new_exc
            raise

        if rewrite_context_with_response:
            self.context.response = response
            if hasattr(response, 'id'):
                self.context.operation_id = response.id
        return response

    def _get_meta(self):
        token = self.context.conf['compute_driver']['compute']['token']
        assert token is not None, "context doesn't contain authorization token"
        return [('authorization', "Bearer " + str(token))]
