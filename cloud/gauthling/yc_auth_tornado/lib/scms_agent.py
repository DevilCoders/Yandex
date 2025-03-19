import socket

import grpc
from tornado import gen
from cloud.gauthling.yc_auth.proto.scms_agent_pb2 import AuthRequest, Status
from cloud.gauthling.yc_auth.proto.scms_agent_pb2_grpc import SCMSAgentStub
from yc_auth_tornado import futures


_UNDEFINED = object()


class SCMSAgentClient(object):
    def __init__(self, target, credentials=None, options=None, timeout=10):
        if credentials is None:
            self.channel = grpc.insecure_channel(target, options)
        else:
            self.channel = grpc.secure_channel(target, credentials, options)
        self.stub = SCMSAgentStub(self.channel)
        self.timeout = timeout

    @gen.coroutine
    def auth(self, key_id, signed_string, signature, datestamp, algorithm, ip_address, is_url=False, timeout=_UNDEFINED):
        if timeout is _UNDEFINED:
            timeout = self.timeout

        request = AuthRequest(
            key_id=key_id,
            signed_string=signed_string,
            signature=signature,
            datestamp=datestamp,
            algorithm=algorithm,
            ip_address=socket.inet_pton(
                socket.AF_INET if '.' in ip_address else socket.AF_INET6,
                ip_address
            ),
            is_url=is_url,
        )
        result = yield futures.wrap_future(self.stub.Auth.future(request, timeout=timeout))
        raise gen.Return(result)

    def auth_stream(self, request_iterator, timeout=_UNDEFINED):
        if timeout is _UNDEFINED:
            timeout = self.timeout

        return futures.wrap_future(self.stub.AuthStream.future(request_iterator, timeout=timeout))

    @gen.coroutine
    def health(self, timeout=_UNDEFINED):
        if timeout is _UNDEFINED:
            timeout = self.timeout

        result = yield futures.wrap_future(self.stub.Health.future(Status(), timeout=timeout))
        raise gen.Return(result)
