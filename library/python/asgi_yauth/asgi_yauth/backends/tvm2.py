import logging
import os

from typing import Optional
from enum import Enum

from tvm2.aio.thread_tvm2 import TVM2 as TVM2Thread
from tvm2.aio.daemon_tvm2 import TVM2Daemon

from .base import (
    BaseBackend,
)
from ..utils import get_real_ip

log = logging.getLogger(__name__)


class AuthTypes(Enum):
    user = 'user_ticket'
    service = 'service_ticket'


class AnonymousReasons(Enum):
    user = 'invalid_user_ticket'
    service = 'invalid_service_ticket'


class Backend(BaseBackend):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        tvm_class = self._get_tvm_class()
        secret = self.config.tvm2_secret

        if secret is not None:
            secret = secret.get_secret_value()
        self.tvm = tvm_class(
            client_id=self.config.tvm2_client,
            secret=secret,
            blackbox_client=self.config.tvm2_blackbox_client,
            allowed_clients=self.config.tvm2_allowed_clients,
        )

    @staticmethod
    def _get_tvm_class():
        for key in ('TVM2_USE_QLOUD', 'TVM2_USE_DAEMON'):
            if key in os.environ:
                return TVM2Daemon
        return TVM2Thread

    async def authenticate(
        self,
        service_ticket: str,
        user_ip: Optional[str] = None,
        user_ticket: Optional[str] = None,
    ):
        parsed_user_ticket = None
        parsed_service_ticket = await self.tvm.parse_service_ticket(service_ticket)
        if not parsed_service_ticket:
            log.warning('Request with not valid service ticket was made')
            return self.anonymous(reason=AnonymousReasons.service.value)

        if user_ticket is not None:
            parsed_user_ticket = await self.tvm.parse_user_ticket(user_ticket)
            if not parsed_user_ticket:
                log.warning('Request with not valid user ticket was made')
                return self.anonymous(reason=AnonymousReasons.user.value)

        yauser_params = dict(
            uid=None,
            backend=self,
            auth_type=AuthTypes.service.value,
            service_ticket=parsed_service_ticket,
            raw_service_ticket=service_ticket,
            user_ip=user_ip,
        )
        if parsed_user_ticket:
            user_ticket_params = dict(
                uid=parsed_user_ticket.default_uid,
                auth_type=AuthTypes.user.value,
                raw_user_ticket=user_ticket,
                user_ticket=parsed_user_ticket,
            )
            yauser_params.update(user_ticket_params)

        return self.yauser(**yauser_params)

    async def extract_params(self, scope, headers):
        service_ticket = headers.get(self.config.tvm2_service_header)
        user_ticket = headers.get(self.config.tvm2_user_header)
        if user_ticket and not service_ticket:
            log.error('User ticket present, service ticket expected but not passed')

        if service_ticket:
            return {
                'user_ticket': user_ticket,
                'service_ticket': service_ticket,
                'user_ip': get_real_ip(headers=headers, config=self.config),
            }
