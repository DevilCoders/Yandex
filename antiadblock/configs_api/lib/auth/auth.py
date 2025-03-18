import sys
import traceback

from flask import request, g, current_app, abort, make_response, jsonify
from tvmauth.exceptions import TvmException


class Auth(object):
    def __init__(self, context):
        self.context = context

    def get_webmaster_service_ticket(self):
        return self.context.tvm.get_service_ticket_for("webmaster")

    def get_user_ticket(self):
        if g.get("user_ticket") is not None:
            return g.user_ticket
        session_id = request.cookies.get("Session_id")
        if not session_id:
            return None

        g.user_ticket = self.context.blackbox.get_user_ticket(session_id)
        return g.user_ticket

    def validate_service_ticket(self, ticket, expected_sources):
        """
        return source service tvm client id if ticket is ok. Raises 403 Forbidden otherwise
        :param expected_sources: List of idm tvm ids we expect
        :param ticket:
        :return:
        """
        def abort403():
            current_app.logger.error("Bad service ticket", extra=dict(ticket=ticket))
            abort(make_response(jsonify(dict(message="Bad service ticket")), 403))

        if not ticket:
            abort403()

        parsed_ticket = self.context.tvm.check_service_ticket(ticket)
        if not parsed_ticket:
            abort403()

        if parsed_ticket.src not in expected_sources:
            abort403()

        return parsed_ticket.src

    def get_user_id(self):
        if g.get("user_id") is not None:
            return g.user_id
        g.user_id = self.get_user_id_from_ticket(self.get_user_ticket())
        return g.user_id

    def get_user_id_from_ticket(self, user_ticket):
        if user_ticket is None:
            return None
        try:
            return self.context.tvm.check_user_ticket(user_ticket).default_uid
        except TvmException as e:
            current_app.logger.error("Got bad user ticket", extra=dict(exception=traceback.format_exception(*sys.exc_info(), limit=3), debug_info=e.debug_info, status=e.status))
            abort(make_response(jsonify(dict(message="Got bad user ticket")), 403))


ALLOWED_APPS_IDS = ["AAB_CRYPROX_TVM_CLIENT_ID", "AAB_SANDBOX_MONITORING_TVM_ID", "AAB_CHECKLIST_JOB_TVM_CLIENT_ID", "ERROR_BUSTER_TVM_CLIENT_ID"]


def ensure_tvm_ticket_is_ok(context, allowed_apps=None):
    allowed_apps = allowed_apps or ALLOWED_APPS_IDS
    context.auth.validate_service_ticket(request.headers.get("X-Ya-Service-Ticket"),
                                         [current_app.config[_id] for _id in allowed_apps])
