import logging

from ylog.context import log_context

from ..sync.daemon_tvm2 import TVM2Daemon as TVM2DaemonBase
from .base import TVM2AsyncBase

log = logging.getLogger(__name__)


class TVM2Daemon(TVM2AsyncBase, TVM2DaemonBase):

    def get_headers(self):
        return {
            'Authorization': self.tvmtool_auth_token,
        }

    def _init_data(self, data):
        pass

    async def parse_ticket(self, ticket, ticket_type):
        if ticket_type == 'user_ticket':
            return await self.parse_user_ticket(ticket)
        elif ticket_type == 'service_ticket':
            return await self.parse_service_ticket(ticket)

    async def parse_user_ticket(self, ticket):
        parsed_ticket = await self.make_request(
            path='checkusr',
            headers={'X-Ya-User-Ticket': ticket},
        )
        if parsed_ticket:
            return self.get_ticket_obj(
                ticket_data=parsed_ticket,
                ticket_type='user_ticket',
            )

    async def parse_service_ticket(self, ticket):
        parsed_ticket = await self.make_request(
            path='checksrv',
            params={'dst': self.client_id},
            headers={'X-Ya-Service-Ticket': ticket},
        )
        if parsed_ticket:
            return self._check_allowed_client(parsed_ticket)

    async def get_service_tickets(self, *destinations):
        with log_context(method='get_service_tickets'):
            str_destinations = ','.join(destinations)
            with log_context(destinations=repr(str_destinations)):

                tickets_response = await self.make_request(
                    path='tickets',
                    params={
                        'src': self.client_id,
                        'dsts': str_destinations,
                    }
                )
                return self._parser_tickets_response(tickets_response, destinations)

