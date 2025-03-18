from datetime import datetime
from decimal import Decimal
from typing import Union, Type, List, Tuple, Optional, Dict, Callable

from .base import ApiResource, ApiEntity
from ..exceptions import ApiCallError

TypePaymentArg = Union[List['Payment'], List[str]]
TypeBatchTuple = Tuple[List['Payment'], List['Payment']]


class Payment(ApiEntity):
    """Сущность платежа."""

    def __init__(
        self,
        *,
        id: str,
        amount: Union[str, Decimal],
        acc_from: str,
        acc_to: str,
        purpose: str = '',
        **other
    ):
        super().__init__()

        self.id = id

        self.number: str = ''
        """Номер, под которым платёж известен внешней системе.
        Свойство населяется в Payments.register().

        """
        self.error: str = ''
        """Ошибка, связанная с данным плетежом.
        Свойство населяется в Payments.register().

        """

        self.props = {
            'id': id,
            'amount': str(amount),
            'f_acc': acc_from,
            't_acc': acc_to,
            'purpose': purpose,
            **other,
        }

    def __str__(self):
        return self.id


class Payments(ApiResource):
    """Инструменты для работы с платежами."""

    cls_payment: Type[Payment] = Payment

    def _separate(self, *, response, by_id, func_contribute: Callable = None) -> TypeBatchTuple:

        failed = []
        succeed = []

        for item in response.data.get('items', []):
            payment_id = item['id']
            payment = by_id.get(payment_id)

            if payment is None:
                payment = Payment(id=payment_id, amount='', acc_from='', acc_to='')

            payment.number = payment.number or item.get('number', '')

            if func_contribute:
                func_contribute(payment, item)

            succeed.append(payment)

        for item in response.errors:
            pay_id = item.get('id')

            if pay_id:
                bogus = by_id[pay_id]
                err_chunks = [item['msg']]

                event_id = item.get('event_id')
                if event_id:
                    err_chunks.append(f"Event ID {event_id}: {item['description']}")

                bogus.error = ' '.join(err_chunks)
                failed.append(bogus)

        if response.errors and not failed:
            raise ApiCallError(response.errors[0], status_code=response.status)

        return succeed, failed

    def register(self, *payments: Payment) -> TypeBatchTuple:
        """Осуществляет регистрацию платежей в BCL.

        Возвращает кортеж из двух элементов:
            (список_успешно_зарегистрированных, список_провалившихся)

        """
        by_id = {payment.id: payment for payment in payments}

        response = self._conn.request(
            url='payments/',
            data={'payments': [payment.props for payment in payments]},
            method='post',

            raise_on_error=False,
        )

        return self._separate(response=response, by_id=by_id)

    def probe(self, *payments: Payment) -> TypeBatchTuple:
        """Осуществляет проверку возможности платежа.

        Возвращает кортеж из двух элементов:
            (список_успешно_проверенных, список_провалившихся)

        """
        by_id = {payment.id: payment for payment in payments}

        response = self._conn.request(
            url='payments/probation/',
            data={'payments': [payment.props for payment in payments]},
            method='post',

            raise_on_error=False,
        )

        return self._separate(response=response, by_id=by_id)

    @staticmethod
    def _extract_attr(attr_name: str, items: Optional[TypePaymentArg], enrich: dict) -> list:
        out_items = []

        for item in items or []:
            if isinstance(item, Payment):
                enrich[item.id] = item
                item = getattr(item, attr_name, None)

            if item:
                out_items.append(item)

        return out_items

    def cancel(self, *, ids: TypePaymentArg) -> TypeBatchTuple:
        """Позволяет аннулировать зарегистированные ранее, но не отправленные платёжи.

        :param ids: Клиентские идентификаторы платежей, которые требует аннулировать.

        """
        by_id: Dict[str, Payment] = {}

        response = self._conn.request(
            url='payments/cancellation/',
            data={'ids': self._extract_attr('id', ids, by_id)},
            method='post',

            raise_on_error=False,
        )

        return self._separate(response=response, by_id=by_id)

    def revoke(self, *, ids: TypePaymentArg) -> TypeBatchTuple:
        """Позволяет отозвать зарегистированные ранее и отправленные платёжи.

        :param ids: Клиентские идентификаторы платежей, которые требуется отозвать.

        """
        by_id: Dict[str, Payment] = {}

        response = self._conn.request(
            url='payments/revocation/',
            data={'ids': self._extract_attr('id', ids, by_id)},
            method='post',

            raise_on_error=False,
        )

        return self._separate(response=response, by_id=by_id)

    def get(
        self,
        *,
        ids: TypePaymentArg = None,
        nums: TypePaymentArg = None,
        statuses: List[int] = None,
        orgs: List[int] = None,
        upd_since: Union[datetime, str] = None,
        upd_till: Union[datetime, str] = None,
        service: int = None
    ) -> List[Payment]:
        """Получает информацию о платежах.

        :param ids: Идентификаторы платежей (клиентские).

        :param nums: Номера платежей (BCL).

        :param statuses: Идентификаторы статусов платежей.

        :param orgs: Идентификаторы организаций.

        :param upd_since: Дата обновления данных платежа "с" (включительно).

        :param upd_till: Дата обновления данных платежа "до" (не включительно).

        :param service: Идентификатор сервиса от имени которого был зарегистрирован
            платёж. Установка данного параметра доступна только при запросах от имени Биллинга.

        """
        filter_dict = {}
        by_id: Dict[str, Payment] = {}

        extract_attr = self._extract_attr

        def contribute_filter(name, val):
            if val:
                filter_dict[name] = val

        contribute_filter('ids', extract_attr('id', ids, by_id))
        contribute_filter('nums', extract_attr('number', nums, by_id))
        contribute_filter('statuses', statuses)
        contribute_filter('orgs', orgs)
        contribute_filter('upd_since', self._cast_date(upd_since))
        contribute_filter('upd_till', self._cast_date(upd_till))
        contribute_filter('service', service)

        assert filter_dict, 'Please narrow down payments filter.'

        response = self._conn.request(url='payments/', data=filter_dict)

        def contribute(payment: Payment, item: dict):
            payment.error = ''
            payment.props = item

        result, _ = self._separate(response=response, by_id=by_id, func_contribute=contribute)

        return result
