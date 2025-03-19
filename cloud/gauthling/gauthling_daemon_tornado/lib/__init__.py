import json

import grpc
import six
from tornado import gen
from tornado.ioloop import IOLoop

from gauthling_daemon.errors import GauthlingError
from cloud.gauthling.gauthling_daemon.proto.gauthling_pb2 import AuthWithSignRequest, AuthzRequest, Status, AuthRequest, \
    AUTHENTICATED, AUTHORIZED
from cloud.gauthling.gauthling_daemon.proto.gauthling_pb2_grpc import GauthlingServiceStub


class GauthlingClient(object):

    def __init__(self, target, credentials=None, options=None, timeout=10):
        if credentials is None:
            self.channel = grpc.insecure_channel(target, options)
        else:
            self.channel = grpc.secure_channel(target, credentials, options)
        self.stub = GauthlingServiceStub(self.channel)
        self.timeout = timeout

    @gen.coroutine
    def ping(self):
        result = yield _wrap_future(self.stub.Ping.future(Status(), timeout=self.timeout))
        raise gen.Return(result)

    @gen.coroutine
    def auth(self, token):
        request = AuthRequest()
        request.token = token
        response = yield _wrap_future(self.stub.Auth.future(request, timeout=self.timeout))
        raise gen.Return(response.state == AUTHENTICATED)

    @gen.coroutine
    def auth_with_sign(self, token, signed_string, signature, datestamp, service):
        request = AuthWithSignRequest()
        request.token = token
        request.signedString = signed_string
        request.signature = signature
        request.datestamp = datestamp
        request.service = service
        response = yield _wrap_future(self.stub.AuthWithSign.future(request, timeout=self.timeout))
        raise gen.Return(response.state == AUTHENTICATED)

    @gen.coroutine
    def auth_with_amazon_sign(self, token, signed_string, signature, datestamp, service, region):
        request = AuthWithSignRequest()
        request.token = token
        request.signedString = signed_string
        request.signature = signature
        request.datestamp = datestamp
        request.service = service
        request.region = region
        response = yield _wrap_future(self.stub.AuthWithAmazonSign.future(request, timeout=self.timeout))
        raise gen.Return(response.state == AUTHENTICATED)

    @gen.coroutine
    def authz(self, token, context):
        request = AuthzRequest()
        request.token = token
        for k, v in six.iteritems(context):
            request.invocation_context[k] = v
        response = yield _wrap_future(self.stub.Authz.future(request, timeout=self.timeout))
        raise gen.Return(response.state == AUTHORIZED)


def _wrap_future(grpc_future):
    """Wraps a GRPC future in one that can be yielded by Tornado"""

    tornado_future = gen.Future()
    loop = IOLoop.current()
    grpc_future.add_done_callback(lambda _: loop.add_callback(_set_result, grpc_future, tornado_future))

    return tornado_future


def _set_result(grpc_future, tornado_future):
    """Assigns GRPC future result to Tornado one"""

    try:
        tornado_future.set_result(grpc_future.result())
    except grpc.RpcError as e:
        details = e.details()
        if details.startswith("{") and details.endswith("}"):
            try:
                gautling_error = json.loads(details)
                e = GauthlingError.from_dict(gautling_error)
            except:
                pass
        tornado_future.set_exception(e)
    except Exception as e:
        tornado_future.set_exception(e)


from gauthling_daemon import errors     # noqa: F401
