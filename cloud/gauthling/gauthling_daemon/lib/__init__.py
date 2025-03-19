import six

import grpc

from gauthling_daemon.errors import handle_rpc_error
from gauthling_daemon.rpc import AuthWithSignRequest, AuthzRequest, Status, AuthRequest, \
    AUTHENTICATED, AUTHORIZED, GauthlingServiceStub


class GauthlingClient(object):

    def __init__(self, target, credentials=None, options=None, timeout=10):
        if credentials is None:
            self.channel = grpc.insecure_channel(target, options)
        else:
            self.channel = grpc.secure_channel(target, credentials, options)
        self.stub = GauthlingServiceStub(self.channel)
        self.timeout = timeout

    @handle_rpc_error
    def ping(self):
        return self.stub.Ping(Status(), timeout=self.timeout)

    @handle_rpc_error
    def auth(self, token):
        request = AuthRequest()
        request.token = token
        response = self.stub.Auth(request, timeout=self.timeout)
        return response.state == AUTHENTICATED

    @handle_rpc_error
    def auth_with_sign(self, token, signed_string, signature, datestamp, service):
        request = AuthWithSignRequest()
        request.token = token
        request.signedString = signed_string
        request.signature = signature
        request.datestamp = datestamp
        request.service = service
        response = self.stub.AuthWithSign(request, timeout=self.timeout)
        return response.state == AUTHENTICATED

    @handle_rpc_error
    def auth_with_amazon_sign(self, token, signed_string, signature, datestamp, service, region):
        request = AuthWithSignRequest()
        request.token = token
        request.signedString = signed_string
        request.signature = signature
        request.datestamp = datestamp
        request.service = service
        request.region = region
        response = self.stub.AuthWithAmazonSign(request, timeout=self.timeout)
        return response.state == AUTHENTICATED

    @handle_rpc_error
    def authz(self, token, context):
        request = AuthzRequest()
        request.token = token
        for k, v in six.iteritems(context):
            request.invocation_context[k] = v
        response = self.stub.Authz(request, timeout=self.timeout)
        return response.state == AUTHORIZED
