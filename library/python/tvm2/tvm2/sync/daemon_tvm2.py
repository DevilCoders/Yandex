# coding: utf-8

from __future__ import unicode_literals

import requests
import logging
import six
import os

from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry
from ylog.context import log_context

from ..ticket import ServiceTicket, UserTicket
from ..protocol import BaseTVM2

log = logging.getLogger(__name__)


class TVM2Daemon(BaseTVM2):
    """
    Общение с tvm-api через демон, доступный в Qloud и Deploy.
    Параметры tvm2 необходимо конфигурировать из интерфейса платформы.
    В Deploy демон по-умолчанию выключен, а при включении начнет потреблять доп. квоту.
    """

    DEFAULT_TVM_API = 'http://localhost:1/tvm'

    def _init_data(self, data):
        session = requests.Session()
        session.headers.update({'Authorization': self.tvmtool_auth_token})
        retry = Retry(
            total=self.retries,
            backoff_factor=0.1,
            status_forcelist=(500, 502, 503, 504),
        )
        adapter = HTTPAdapter(max_retries=retry)
        session.mount(self.api_url, adapter)
        self.session = session

    @property
    def is_deploy(self):
        return 'DEPLOY_BOX_ID' in os.environ

    def _get_api_url(self, url):
        if self.is_deploy:
            url = url or '{}/tvm'.format(os.environ['DEPLOY_TVM_TOOL_URL'])
        return url or self.DEFAULT_TVM_API

    @property
    def tvmtool_auth_token(self):
        if self.is_deploy:
            token = os.environ['TVMTOOL_LOCAL_AUTHTOKEN']
        else:
            token = os.environ['QLOUD_TVM_TOKEN']
        return token

    def get_request(self, method, params, headers):
        return requests.Request(
            method='GET',
            url='{}/{}'.format(self.api_url, method),
            params=params,
            headers=headers,
        )

    def make_request(self, method, params=None, headers=None):
        request = self.get_request(method, params, headers)
        try:
            response = self.session.send(
                self.session.prepare_request(request),
                timeout=5,
            )
            response.raise_for_status()
            response = response.json()
        except requests.RequestException as exc:
            log.exception('Got exception while making request: "%s"', repr(exc)[-300:])
            response = {}
        return response

    def get_ticket_obj(self, ticket_data, ticket_type):
        if ticket_type == 'service_ticket':
            return ServiceTicket(ticket_data)
        elif ticket_type == 'user_ticket':
            return UserTicket(ticket_data)

    def _parser_tickets_response(self, tickets_response, destinations):
        tickets = {}
        received_destinations = set()

        if tickets_response:
            for _, service_data in six.iteritems(tickets_response):
                tvm_id = str(service_data['tvm_id'])
                tickets[tvm_id] = service_data['ticket']
                received_destinations.add(tvm_id)

        if len(received_destinations) != len(destinations):
            log.error(
                'Don\'t get tickets for all destinations "%s", get "%s"',
                repr(received_destinations), ','.join(destinations),
            )
            tickets.update({
                destination: None
                for destination in destinations
                if destination not in received_destinations
            })
        return tickets

    def parse_ticket(self, ticket, ticket_type):
        """
        Возвращет либо тикет, если проверка прошла
        успешно, либо же None, если возникла ошибка
        при проверке
        """
        with log_context(action='parse_ticket', ticket=repr(ticket)):
            if ticket_type == 'user_ticket':
                return self.parse_user_ticket(ticket)
            elif ticket_type == 'service_ticket':
                return self.parse_service_ticket(ticket)

    def parse_user_ticket(self, ticket):
        parsed_ticket = self.make_request(
            method='checkusr',
            headers={'X-Ya-User-Ticket': ticket},
        )
        if parsed_ticket:
            return self.get_ticket_obj(
                ticket_data=parsed_ticket,
                ticket_type='user_ticket',
            )

    def _check_allowed_client(self, parsed_ticket):
        ticket_obj = self.get_ticket_obj(
            ticket_data=parsed_ticket,
            ticket_type='service_ticket',
        )
        src = ticket_obj.src
        if '*' in self.allowed_clients or src in self.allowed_clients:
            return ticket_obj
        log.error('Request from not allowed client was made: "%s"', src)

    def parse_service_ticket(self, ticket):
        parsed_ticket = self.make_request(
            method='checksrv',
            params={'dst': self.client_id},
            headers={'X-Ya-Service-Ticket': ticket},
        )
        if parsed_ticket:
            return self._check_allowed_client(parsed_ticket)

    def get_service_tickets(self, *destinations):
        """
        Возвращает словарь тикетов для передачи в запросе
        к соответствующим клиентам с id переданными в "destinations"
        Ответ будет вида {'client_id': 'ticket', 'client_id2': 'ticket2, }
        в случае успешного получения данных по всем клиентам, если по некоторым
        клиентам api tvm не вернуло тикета - в ответе будет {'client_id': None,
        'client_id2': 'ticket'}, если ответ от апи твм пришел с ошибкой - в ответе
        будет пустой словарь

        :type destinations: six.string_types
        :rtype dict
        """
        with log_context(method='get_service_tickets'):
            str_destinations = ','.join(destinations)
            with log_context(destinations=repr(str_destinations)):

                tickets_response = self.make_request(
                    method='tickets',
                    params={
                        'src': self.client_id,
                        'dsts': str_destinations,
                    }
                )
                return self._parser_tickets_response(tickets_response, destinations)
