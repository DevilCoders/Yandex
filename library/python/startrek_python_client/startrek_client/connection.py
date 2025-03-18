# coding: utf-8

from pkg_resources import get_distribution, DistributionNotFound

from yandex_tracker_client.connection import Connection as BaseConnection
from yandex_tracker_client.connection import *
from yandex_tracker_client.exceptions import *

from .settings import VERSION_V2
from .exceptions import EXCEPTIONS_MAP


class Connection(BaseConnection):

    def __init__(self,
                 useragent,
                 token=None,
                 session_id=None,
                 ticket=None,
                 user_ticket=None,
                 service_ticket=None,
                 base_url="https://st-api.yandex-team.ru",
                 timeout=10,
                 retries=10,
                 headers=None,
                 api_version=VERSION_V2,
                 verify=True,
                 ):
        super(Connection, self).__init__(
            token=token, org_id=None, base_url=base_url,
            timeout=timeout, retries=retries, headers=headers,
            api_version=api_version, verify=verify,
        )
        self.session.headers.pop('Authorization', None)
        self.session.headers.pop('X-Org-Id', None)

        try:
            version = get_distribution('yandex_tracker_client').version
        except DistributionNotFound:
            version = 'developer'
        useragent += ' via tracker-python-client/' + version
        useragent = useragent.lstrip(' ')
        self.session.headers['User-Agent'] = useragent

        if session_id is not None:
            self.session.cookies['Session_id'] = session_id
        elif token is not None:
            self.session.headers['Authorization'] = 'OAuth ' + token
        elif ticket is not None:
            self.session.headers['X-Tvm-Ticket'] = ticket
        elif (user_ticket is not None) or (service_ticket is not None):
            if user_ticket:
                self.session.headers['X-Ya-User-Ticket'] = user_ticket
            if service_ticket:
                self.session.headers['X-Ya-Service-Ticket'] = service_ticket
            

    def _try_request(self, **kwargs):
        try:
            return super(Connection, self)._try_request(**kwargs)
        except TrackerError as exc:
            internal_exception, context = self._get_internal_exception(exc)
            if internal_exception:
                raise internal_exception(*context)
            raise

    def _get_internal_exception(self, exc):

        exc_class = exc.__class__
        internal_exception = EXCEPTIONS_MAP.get(exc_class)
        if internal_exception:
            return internal_exception, exc.args
        return None, None
