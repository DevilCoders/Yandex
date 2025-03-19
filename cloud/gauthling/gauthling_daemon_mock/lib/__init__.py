import json
import flask
import grpc

from cloud.gauthling.gauthling_daemon.proto import gauthling_pb2_grpc
from cloud.gauthling.gauthling_daemon.proto.gauthling_pb2 import AuthState, AuthResponse, AuthzState, AuthzResponse, Status
from yandex.cloud.priv.servicecontrol.v1.access_service_pb2 import AuthenticateResponse, AuthorizeResponse, Subject
from yandex.cloud.priv.servicecontrol.v1 import access_service_pb2_grpc


class GauthlingMockServicer(gauthling_pb2_grpc.GauthlingServiceServicer):
    def __init__(self):
        self.state = {
            "authenticate_requests": True,
            "authorize_requests": True,
        }

    def set_auth(self, value):
        self.state["authenticate_requests"] = value

    def set_authz(self, value):
        self.state["authorize_requests"] = value

    def Ping(self, request, context):
        return Status()

    def Auth(self, request, context):
        auth_state = AuthState.Value("AUTHENTICATED") if self.state["authenticate_requests"] else \
            AuthState.Value("UNAUTHENTICATED")
        response = AuthResponse()
        response.state = auth_state
        return response

    def AuthWithSign(self, request, context):
        auth_state = AuthState.Value("AUTHENTICATED") if self.state["authenticate_requests"] else \
            AuthState.Value("UNAUTHENTICATED")
        response = AuthResponse()
        response.state = auth_state
        return response

    def AuthWithAmazonSign(self, request, context):
        auth_state = AuthState.Value("AUTHENTICATED") if self.state["authenticate_requests"] else \
            AuthState.Value("UNAUTHENTICATED")
        response = AuthResponse()
        response.state = auth_state
        return response

    def Authz(self, request, context):
        authz_state = AuthzState.Value("AUTHORIZED") if self.state["authorize_requests"] else \
            AuthzState.Value("UNAUTHORIZED")
        response = AuthzResponse()
        response.state = authz_state
        return response

    def Sync(self, request, context):
        return Empty()


class AccessServiceMockServicer(access_service_pb2_grpc.AccessServiceServicer):
    def __init__(self):
        self.state = {
            "authenticate_requests": {"id": "0000-0000-0000-0000", "user_account": {"email": "my@email.com"}},
            "authorize_requests": {"id": "0000-0000-0000-0000", "user_account": {"email": "my@email.com"}}
        }

    def set_auth_subject(self, value):
        self.state["authenticate_requests"] = value

    def set_authz_subject(self, value):
        self.state["authorize_requests"] = value

    @staticmethod
    def _copy_subject(src, dst):
        dst.id = src["id"]
        if "user_account" in src:
            dst.user_account.email = src["user_account"]["email"]
        if "service_account" in src:
            dst.service_account.folder_id = src["service_account"]["folder_id"]
            dst.service_account.name = src["service_account"]["name"]

    def Authenticate(self, request, context):
        subject = self.state["authenticate_requests"]
        if subject is None:
            context.set_code(grpc.StatusCode.UNAUTHENTICATED);
            raise RuntimeError("Unauthenticated")

        response = AuthenticateResponse()
        self._copy_subject(subject, response.subject)
        return response

    def Authorize(self, request, context):
        subject = self.state["authorize_requests"]
        if subject is None:
            context.set_code(grpc.StatusCode.PERMISSION_DENIED);
            raise RuntimeError("Unauthorized")

        response = AuthorizeResponse()
        self._copy_subject(subject, response.subject)
        return response


class ControlServer(flask.Flask):
    """
    Flask application that controls Gauthling Server Mock.
    """

    def __init__(self, gauthling, access_service, *args, **kwargs):
        super(ControlServer, self).__init__(*args, **kwargs)

        self.gauthling = gauthling
        self.access_service = access_service

        self.add_url_rule("/mock/ping", view_func=self._ping_handler)
        self.add_url_rule("/gauthling/setAuthenticateRequests", methods=["POST"],
                          view_func=self.set_gauthling_authenticate_requests_handler)
        self.add_url_rule("/gauthling/setAuthorizeRequests", methods=["POST"],
                          view_func=self.set_gauthling_authorize_requests_handler)
        self.add_url_rule("/access_service/setAuthenticateRequestsSubject", methods=["POST"],
                          view_func=self.set_access_service_authenticate_requests_handler)
        self.add_url_rule("/access_service/setAuthorizeRequestsSubject", methods=["POST"],
                          view_func=self.set_access_service_authorize_requests_handler)
        self.add_url_rule("/gauthling/state", methods=["GET"],
                          view_func=self.get_gauthling_state_handler)
        self.add_url_rule("/access_service/state", methods=["GET"],
                          view_func=self.get_access_service_state_handler)
        self.add_url_rule("/shutdown", view_func=self.shutdown_handler, methods=["POST"])

    def _ping_handler(self):
        """
        Check service availability.
        """
        return flask.jsonify(status="OK")

    def set_gauthling_authenticate_requests_handler(self):
        """
        Set gauthling to authenticate or not to authenticate all incoming requests
        """
        value = self._get_bool_value()
        self.gauthling.set_auth(value)
        return flask.jsonify(status="OK")

    def set_gauthling_authorize_requests_handler(self):
        """
        Set gauthling to authorize or not to authorize all incoming requests
        """
        value = self._get_bool_value()
        self.gauthling.set_authz(value)
        return flask.jsonify(status="OK")

    def set_access_service_authenticate_requests_handler(self):
        """
        Set access service to authenticate or not to authenticate all incoming requests
        """
        value = self._get_subject_value()
        self.access_service.set_auth_subject(value)
        return flask.jsonify(status="OK")

    def set_access_service_authorize_requests_handler(self):
        """
        Set access service to authorize or not to authorize all incoming requests
        """
        value = self._get_subject_value()
        self.access_service.set_authz_subject(value)
        return flask.jsonify(status="OK")

    def get_gauthling_state_handler(self):
        """
        Return current gauthling state
        """
        return flask.jsonify(**self.gauthling.state)

    def get_access_service_state_handler(self):
        """
        Return current access service state
        """
        return flask.jsonify(**self.access_service.state)

    @staticmethod
    def shutdown():
        func = flask.request.environ.get('werkzeug.server.shutdown')
        if func is None:
            raise RuntimeError('Not running with the Werkzeug Server')
        func()

    def shutdown_handler(self):
        self.shutdown()
        return 'Gauthling control server shutting down...'

    @staticmethod
    def _get_subject_value():
        try:
            raw_body = flask.request.data.decode("utf-8")
            body = json.loads(raw_body) if raw_body else None
            return body
        except KeyError as e:
           flask.abort(400, "Missing body param: {}".format(e))

    @staticmethod
    def _get_bool_value():
        try:
            raw_body = flask.request.data.decode("utf-8")
            body = json.loads(raw_body) if raw_body else {}
            value = body["value"]
            if not isinstance(value, bool):
                flask.abort(400, "Invalid value: {}".format(value))
            return value
        except KeyError as e:
            flask.abort(400, "Missing body param: {}".format(e))
