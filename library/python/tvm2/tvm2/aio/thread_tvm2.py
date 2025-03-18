import logging

from ..sync.thread_tvm2 import TVM2 as TVM2Base
from .base import TVM2AsyncBase

log = logging.getLogger(__name__)


class TVM2(TVM2AsyncBase, TVM2Base):
    """
    Ассинхронный класс для работы с тредовым TVM2

    Важно - инициализация синхроннная, поэтому следует ее проводить
    при старте приложения
    """

    async def get_service_ticket(self, destination, **kwargs):
        tickets = self.get_service_tickets(destination)
        return tickets.get(destination)

    async def aio_get_service_tickets(self, *destinations, **kwargs):
        return self.get_service_tickets(*destinations, **kwargs)

    async def parse_user_ticket(self, ticket):
        return super().parse_user_ticket(ticket)

    async def parse_service_ticket(self, ticket):
        return super().parse_service_ticket(ticket)
