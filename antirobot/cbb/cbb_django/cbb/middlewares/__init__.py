import logging

import sqlalchemy

from antirobot.cbb.cbb_django.cbb.models import GroupResponsible, UserRole
from django import http
from django.conf import settings
from django.shortcuts import render

from antirobot.cbb.cbb_django.cbb.library import common, db

NO_COOKIE_SUPPORT = "nocookiesupport"

logger = logging.getLogger("cbb.app.views")


class UserRights(object):
    def __init__(self, login, role, subject_groups):
        self.login = login
        self.role = role
        self._subject_groups = subject_groups
        self._is_god = self.role == settings.ROLE_SUPERVISOR

    def can(self, action, group_id=None):
        if self._is_god:
            return True

        if action == settings.ACTION_GROUP_MODIFY:
            if not group_id or int(group_id) not in self._subject_groups:
                return False

        return action in settings.ROLES[self.role]["actions"]

    def can_read(self):
        return self.can(settings.ACTION_CAN_READ)

    def can_modify_group(self, group_id):
        return self.can(settings.ACTION_GROUP_MODIFY, group_id)

    def can_create_group(self):
        return self.can(settings.ACTION_GROUP_CREATE)

    def __repr__(self):
        return f"{self.login}: {self.role}"

    @property
    def is_god(self):
        return self._is_god


class AuthCheckMiddleware(object):
    def __init__(self, get_response):
        self.get_response = get_response

    @db.main.use_slave()
    def get_role(self, login):
        return UserRole.query.get(login)

    @db.main.use_slave()
    def get_subject_groups(self, login):
        return set((row.group_id for row in GroupResponsible.query.filter_by(user_login=login)))

    def __call__(self, request):
        request.cbb_user = None

        if request.path in ["/ping", "/ping/"]:
            return http.HttpResponse("I'm alive.", content_type="text/plain")

        if request.path == "/status" or request.path.startswith("/ajax/") or request.path.startswith("/cgi-bin/") or request.path.startswith("/api/"):
            return self.get_response(request)

        if settings.FORCE_USER:
            request.cbb_user = UserRights(
                settings.FORCE_USER,
                settings.ROLE_SUPERVISOR,
                self.get_subject_groups(settings.FORCE_USER)
            )
            return self.get_response(request)

        if request.path.startswith("/" + settings.IDM_URL_PREFIX):
            return self.get_response(request)

        if not request.yauser or not request.yauser.is_authenticated():
            return common.response_forbidden()

        if request.yauser.login in settings.ADMINS:
            user_role = UserRole(login=request.yauser.login, role="admin")
        else:
            try:
                user_role = self.get_role(request.yauser.login)
            except db.DatabaseNotAvailable:
                return render(request, "cbb/nodb.html", status=500)

        if not user_role:
            return common.response_forbidden()

        request.cbb_user = UserRights(user_role.login, user_role.role, self.get_subject_groups(user_role.login))

        return self.get_response(request)


class DbManagerMiddleware(object):
    def __init__(self, get_response):
        self.get_response = get_response

    """
    Does database-related things
    """
    def __call__(self, request):
        """
        This commit never commits anything under any circumstance.
        Its ONLY purpose is to raise error if session is dirty
        (and no, checking session.dirty does not do it).
        It is simpler to try to commit than to try to
        find out whether session has changes pending.
        """
        try:
            for man in (db.main, db.history):
                man.session.commit()
                man.session.remove()
        except (db.DatabaseNotConfigured, sqlalchemy.exc.InvalidRequestError):
            for man in (db.main, db.history):
                man.session.rollback()
            logging.error("Session has some uncommited changes")

        return self.get_response(request)

    def process_exception(self, request, exception):
        """
        Renders nodb page for non-API request
        """
        if request.path.startswith("/cgi-bin/") or request.path.startswith("/api/"):
            # Don"t want to mess with API errors
            return

        if isinstance(exception, db.DatabaseNotAvailable):
            return render(request, "cbb/nodb.html", status=500)
