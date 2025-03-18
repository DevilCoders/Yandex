# coding: utf-8

from __future__ import unicode_literals

import logging

from ylog.context import log_context
from tvmauth import TvmClient, TvmApiClientSettings, TvmClientStatus, TIROLE_TVMID, TIROLE_TVMID_TEST
from tvmauth import exceptions as tvm_exceptions

from ..exceptions import NotAllRequiredKwargsPassed
from ..protocol import (
    BaseTVM2, TIROLE_HOST,
    TIROLE_HOST_TEST, TIROLE_PORT,
    TIROLE_CHECK_SRC_DEFAULT,
    TIROLE_CHECK_UID_DEFAULT,
    TIROLE_ENV_DEFAULT,
)

log = logging.getLogger(__name__)


class TVM2(BaseTVM2):
    REQUIRED_KWARGS = {'secret', }

    def _init_data(self, data):
        self.secret = data['secret']
        if len(self.secret) == 0:
            raise NotAllRequiredKwargsPassed('Secret shouldn\'t be blank string')
        destinations = data.get('destinations') or tuple()
        if not isinstance(destinations, dict):
            destinations = {str(tvm_id): int(tvm_id) for tvm_id in destinations}
        self.destinations = destinations
        self.disk_cache_dir = data.get('disk_cache_dir')
        self.fetch_roles_for_idm_system = data.get('fetch_roles_for_idm_system')
        self._setup_tirole(data)
        self._init_context()

    def _init_context(self):
        self.destinations_values = set(self.destinations.values())
        secret = self.secret
        if not self.destinations:
            secret = None
        kwargs = {
            'self_tvm_id': self.client_id,
            'enable_service_ticket_checking': True,
            'enable_user_ticket_checking': self.blackbox_env,
            'self_secret': secret,
            'dsts': self.destinations or None,
        }

        if self.disk_cache_dir:
            kwargs['disk_cache_dir'] = self.disk_cache_dir

        if self.fetch_roles_for_idm_system:
            kwargs['fetch_roles_for_idm_system_slug'] = self.fetch_roles_for_idm_system
            kwargs['tirole_host'] = self.tirole_host
            kwargs['tirole_tvmid'] = self.tirole_tvm_id
            kwargs['tirole_port'] = TIROLE_PORT
            kwargs['check_src_by_default'] = self.tirole_check_src
            kwargs['check_default_uid_by_default'] = self.tirole_check_uid

        current_context = getattr(self, '_context', None)
        self._context = TvmClient(TvmApiClientSettings(**kwargs))
        if current_context is not None:
            # уже был запущен клиент с другими настройками нужно его остановить
            current_context.stop()

    def _setup_tirole(self, data):
        if self.fetch_roles_for_idm_system:
            is_prod = data.get('tirole_env', TIROLE_ENV_DEFAULT) == TIROLE_ENV_DEFAULT
            tirole_tvm_id = TIROLE_TVMID
            tirole_host = TIROLE_HOST
            if not is_prod:
                tirole_tvm_id = TIROLE_TVMID_TEST
                tirole_host = TIROLE_HOST_TEST

            self.destinations.setdefault(str(tirole_tvm_id), tirole_tvm_id)
            self.tirole_tvm_id = tirole_tvm_id
            self.tirole_host = tirole_host
            self.tirole_check_src = data.get('tirole_check_src', TIROLE_CHECK_SRC_DEFAULT)
            self.tirole_check_uid = data.get('tirole_check_uid', TIROLE_CHECK_UID_DEFAULT)

    def _check_role_available(self):
        if not (self.fetch_roles_for_idm_system and self.disk_cache_dir):
            raise NotAllRequiredKwargsPassed('Attempt to check role without fetch_roles_for_idm_system / disk_cache_dir set')

    def check_service_role(self, service_ticket, role, exact_entity=None):
        self._check_role_available()
        roles = self._context.get_roles()
        return roles.check_service_role(service_ticket, role, exact_entity)

    def check_user_role(self, user_ticket, role, selected_uid=None, exact_entity=None):
        self._check_role_available()
        roles = self._context.get_roles()
        return roles.check_user_role(user_ticket, role, selected_uid=selected_uid, exact_entity=exact_entity)


    def add_destinations(self, *destinations):
        for client_id in destinations:
            self.destinations[str(client_id)] = int(client_id)
        self._init_context()

    def set_destinations(self, *destinations):
        self.destinations = {
            str(client_id): int(client_id)
            for client_id in destinations
        }
        self._init_context()

    def parse_user_ticket(self, ticket):
        self.check_context()
        with log_context(blackbox_url=self.blackbox_url):
            return self.parse_ticket(ticket, context='user')

    def parse_service_ticket(self, ticket):
        self.check_context()
        parsed_ticket = self.parse_ticket(ticket, context='service')

        if not parsed_ticket:
            return

        if '*' in self.allowed_clients or parsed_ticket.src in self.allowed_clients:
            return parsed_ticket

        log.error('Request from not allowed client was made: "%s"', parsed_ticket.src)

    def check_context(self):
        if self._context.status == TvmClientStatus.Warn:
            log.warning(
                'There are problems with refreshing tvm tickets '
                'cache, so it is expiring, error - {}'.format(self._context.status.last_error)
            )
            return TvmClientStatus.Warn
        elif self._context.status == TvmClientStatus.Error:
            log.error(
                'TvmClient cache is already invalid (expired) or '
                'soon will be: you cant check valid '
                'ServiceTicket, error - {}'.format(self._context.status.last_error)
            )
            return TvmClientStatus.Error

    def parse_ticket(self, ticket, context):
        """
        Возвращет либо тикет, если проверка прошла
        успешно, либо же None, если возникла ошибка
        при проверке
        """
        with log_context(action='parse_ticket', ticket=repr(ticket[:10])):
            if context == 'user':
                check_func = 'check_user_ticket'
            else:
                check_func = 'check_service_ticket'

            try:
                return getattr(self._context, check_func)(ticket)
            except tvm_exceptions.TicketParsingException as exc:
                log.exception('Got ticket parsing error while parsing ticket: "%s", info: "%s"', repr(exc.status), repr(exc.debug_info))
            except tvm_exceptions.TvmException as exc:
                log.exception('Unexpected error acquired while parsing ticket: "%s"', repr(exc)[-300:])


    def _has_new_destinations(self, destinations):
        destinations = {int(destination) for destination in destinations}
        return destinations.difference(self.destinations_values)

    def get_service_tickets(self, *destinations, **kwargs):
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
            self.check_context()
            new_destinations = self._has_new_destinations(destinations)
            if new_destinations:
                log.info('New destinations appeared - {}, please consider adding it to initial values'.format(destinations))
                self.add_destinations(*new_destinations)

            with log_context(destinations=repr(destinations), client_id=self.client_id):
                return {
                    destination: self._context.get_service_ticket_for(tvm_id=int(destination))
                    for destination in destinations
                }
