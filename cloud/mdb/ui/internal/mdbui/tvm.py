from blackboxer import Blackbox, UnknownError


class TVMBlackbox(Blackbox):
    """Клиент для походов в ЧЯ с авторизацией через TVM-тикеты.
    Чтобы авторизоваться по тикету, этот тикет нужно где-то получить.
    Например это можно сделать при помощи библиотеки `tvm2 <https://github.yandex-team.ru/common-python/tvm2>`_.
    Далее создадим callable объект, который будет эти тикеты возвращать.
    Кешировать тикеты не надо -- TVMBlackbox кеширует их самостоятельно.
    Создание тикета будет происходить при первом запросе.
    Обновление тикета происходит при получение ошибки от ЧЯ о том, что тикет просроченый::
        import tmv2
        from ticket_parser2.api.v1 import BlackboxClientId
        def ticket_generator():
            # type: () -> str
            tvm_client = tvm2.TVM2(
                client_id='1234567',
                secret='secret',
                blackbox_client=BlackboxClientId.Test
            )
            tickets = get_service_tickets(BlackboxClientId.Test.value)
            ticket = service_tickets.get(BlackboxClientId.Test.value)
            return ticket
        bb_tvm = TVMBlackbox(ticket_generator)
        bb_tvm.checkip('127.0.0.1','yandexusers')
    Ошибки, связанные с тикетами, приходят с кодом `1: UNKNOWN_ERROR` -- исключение UnknownError.
    Ошбки, которые могут прилететь от ЧЯ::
            <BlackBox error: Failed to check service ticket: Expired ticket>
            <BlackBox error: Failed to check service ticket: Malformed ticket>
            <BlackBox error: Failed to check service ticket: Invalid ticket type>
    Тикет передается в заголовке `X-Ya-Service-Ticket`.
    Полезные ссылки:
        * `TVM 2.0 <https://wiki.yandex-team.ru/passport/tvm2/>`_
        * `Коротко про ServiceTicket <https://wiki.yandex-team.ru/passport/tvm2/stbrief/>`_
        * `TVM FAQ <https://wiki.yandex-team.ru/passport/tvm2/faq/>`_
    """

    def __init__(self, ticket_generator, **kwargs):
        # type: (Callable[..., str], **str) -> None
        super(TVMBlackbox, self).__init__(**kwargs)
        self._ticket_generator = ticket_generator
        self._ticket_value = ''

    def _ticket(self, cached=True):
        # type: (bool) -> str
        if self._ticket_value and cached:
            return self._ticket_value
        self._ticket_value = self._ticket_generator()
        return self._ticket_value

    def _make_request(self, http_method, payload, **kwargs):
        # type: (str, dict, **str) -> dict
        headers = {'X-Ya-Service-Ticket': self._ticket()}
        origin_headers = kwargs.pop('headers', {})  # type: dict
        headers.update(origin_headers)
        try:
            return super(TVMBlackbox, self)._make_request(http_method, payload, headers=headers, **kwargs)
        except UnknownError as e:
            if 'Expired ticket' in str(e):
                headers['X-Ya-Service-Ticket'] = self._ticket(cached=False)
                return super(TVMBlackbox, self)._make_request(http_method, payload, headers=headers, **kwargs)
            raise
