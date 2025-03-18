# coding: utf-8
from __future__ import unicode_literals

import logging

from ids import exceptions

logger = logging.getLogger(__name__)


class PluginBase(object):
    def __init__(self, connector):
        self.connector = connector
        super(PluginBase, self).__init__()


class SetRequestHeadersPlugin(PluginBase):

    def get_headers(self):
        raise NotImplementedError()

    def prepare_params(self, params):
        headers = params.get('headers', {})
        headers_to_add = self.get_headers()
        headers.update(headers_to_add)
        params['headers'] = headers


class OAuth(SetRequestHeadersPlugin):
    """
    Плагин для коннекторов для авторизации по OAuth.

    Добавляет обязательный параметр `oauth_token` к параметрам коннектора.
    Проставляет заголовок Authorization для запроса.

    @warning: Если заголовок Authorization уже стоит, он будет перезаписан.
    """
    required_params = ['oauth_token']

    def get_headers(self):
        return {'Authorization': 'OAuth ' + self.connector.oauth_token}


class Tvm(SetRequestHeadersPlugin):
    """
    Плагин для коннекторов для авторизации по TVM-тикету.

    Добавляет обязательный параметр `tvm_ticket` к параметрам коннектора.
    Проставляет заголовок Ticket для запроса.

    @warning: Если заголовок Ticket уже стоит, он будет перезаписан.
    """
    required_params = ['tvm_ticket']

    header_name = 'Ticket'

    def get_headers(self):
        return {self.header_name: self.connector.tvm_ticket}


class TVM2UserTicket(SetRequestHeadersPlugin):
    """
    Плагин для коннекторов для авторизации по пользовательским tvm2 тикетам.

    Добавляет обязательные параметры `user_ticket` и `service_ticket` к параметрам коннектора.
    Проставляет заголовки X-Ya-User-Ticket и X-Ya-Service-Ticket для запроса.

    @warning: Если заголовки X-Ya-User-Ticket и X-Ya-Service-Ticket уже стоят, они будут перезаписан.
    """
    required_params = ['user_ticket', 'service_ticket']

    def get_headers(self):
        return {
            'X-Ya-User-Ticket': self.connector.user_ticket,
            'X-Ya-Service-Ticket': self.connector.service_ticket,
        }


class TVM2ServiceTicket(SetRequestHeadersPlugin):
    """
    Плагин для коннекторов для авторизации по сервисным (обезличенным) tvm2 тикетам.

    Добавляет обязательный параметр `service_ticket` к параметрам коннектора.
    Проставляет заголовок X-Ya-Service-Ticket для запроса.

    @warning: Если заголовок X-Ya-Service-Ticket уже стоит, он будет перезаписан.
    """
    required_params = ['service_ticket']

    def get_headers(self):
        return {'X-Ya-Service-Ticket': self.connector.service_ticket}


class JsonResponse(PluginBase):
    """
    Может использоваться для api, где всегда возвращается json, чтобы
    автоматически получать из методов дикты вместо json.
    """
    def handle_response(self, response):
        try:
            return response.json()
        except ValueError:
            logger.exception('Failed to parse json for url: "%s"',
                             repr(response.url),
                             )
            return self.connector.handle_bad_response(response)


def get_disjunctive_plugin_chain(plugins):
    """
    Возвращает плагин (класс) для коннекторов для объединения нескольких плагинов с
    prepare_params в цепочку – отрабатывает один первый плагин, чьи required_params
    целиком есть (в том числе, когда не заданы вообще) в актуальных параметрах.

    В текущей реализации у плагинов вызывается только prepare_params.
    В текущей реализации обязательно хотя бы один плагин из цепочки должен отработать,
    иначе выбрасывается эксепшн.
    (при необходимости легко дописать, чтобы могли отрабатывать все подходящие плагины,
    и чтобы отрабатывание не было обязательным, но пока нет такой необходимости)

    @param plugins iterable с классами плагинов внутри
    """

    class DisjunctivePluginChain(PluginBase):

        def __init__(self, connector):
            super(DisjunctivePluginChain, self).__init__(connector)
            self.plugins = [plugin(connector) for plugin in plugins]
            assert len(self.plugins)
            self.suitable_plugin = None

        def check_required_params(self, params):
            # ищем хотя бы один плагин, который может отработать
            for plugin in self.plugins:
                if self._can_process(params, plugin):
                    self.suitable_plugin = plugin
                    return

            # ни одного не нашли
            param_sets = ' | '.join(str(plugin.required_params) for plugin in self.plugins)
            raise exceptions.ConfigurationError(
                'Neither of required param sets {%s} is presented' % param_sets
            )

        def prepare_params(self, params):
            self.suitable_plugin.prepare_params(params)

        def _can_process(self, params, plugin):
            return (
                not hasattr(plugin, 'required_params')
                or all(
                    required_param in params
                    for required_param in plugin.required_params
                )
            )

    return DisjunctivePluginChain
