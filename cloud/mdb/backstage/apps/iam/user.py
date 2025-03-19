import logging
from django.conf import settings

import cloud.mdb.backstage.apps.iam.session_service
import cloud.mdb.backstage.apps.iam.access_service
import cloud.mdb.backstage.apps.iam.missing_ava as missing_ava


logger = logging.getLogger('backstage.iam.user')


session_service = cloud.mdb.backstage.apps.iam.session_service.SessionService()
access_service = cloud.mdb.backstage.apps.iam.access_service.AccessService()


class UserDescriptor:
    def __get__(self, request, owner):
        if not hasattr(request, '_iam_user'):
            request._iam_user = self._get_iam_user(request)
        return request._iam_user

    def _get_iam_user(self, request):
        try:
            yc_session = request.COOKIES.get('yc_session', '')
            result = session_service.check(f'yc_session={yc_session}')
        except cloud.mdb.backstage.apps.iam.session_service.AuthenticateRedirectError as err:
            return User(
                is_authenticated=False,
                auth_url=err.redirect_url,
            )
        else:
            return User(
                is_authenticated=True,
                data=result,
            )


class TestUserDescriptor(UserDescriptor):
    def __get__(self, request, owner):
        if not hasattr(request, '_iam_user'):
            request._iam_user = self._get_iam_user(request)
        return request._iam_user

    def _get_iam_user(self, request):
        return TestUser()


class User:
    def __init__(
        self,
        is_authenticated,
        auth_url=None,
        data=None,
    ):
        self._auth_url = auth_url
        self._is_authenticated = is_authenticated
        self._data = data

    @property
    def login(self):
        if self._data:
            return self._data.subject_claims.preferred_username.split('@')[0]
        else:
            return None

    @property
    def sub(self):
        if self._data:
            return self._data.subject_claims.sub
        else:
            return None

    @property
    def federation(self):
        if self._data:
            return self._data.subject_claims.federation
        else:
            return None

    @property
    def picture_data(self):
        if self._data:
            return self._data.subject_claims.picture_data
        else:
            return None

    def is_authenticated(self):
        return self._is_authenticated

    def has_perm(self, permission):
        if not self.is_authenticated():
            return False
        try:
            return access_service.authorize(
                subject={
                    "user_account": {
                        "id": self.sub,
                        "federation_id": self.federation.id,
                    }
                },
                permission=permission,
            )
        except Exception as err:
            logger.exception(f'failed to check user permission: {err}')
            return False


class TestUser:
    login = settings.CONFIG.iam.test_user
    sub = settings.CONFIG.iam.test_sub
    federation = settings.CONFIG.iam.test_federation
    picture_data = missing_ava.DATA

    def is_authenticated(self):
        return True

    def has_perm(self, *args, **kwargs):
        return True
