from datetime import datetime, date
from decimal import Decimal
from typing import List, Type, Union, Optional

from .base import ApiResource, ApiEntity


class StatementPayment(ApiEntity):
    """Платёж по выписке."""

    __slots__ = ['number', 'date', 'amount', 'currency', 'purpose', 'direction', 'recipient']

    def __init__(
        self,
        *,
        number: str,
        on_date: date,
        amount: Decimal,
        currency: str,
        purpose: str,
        direction: str,
        recipient: dict
    ):
        self.number = number
        """Номер."""

        self.date = on_date
        """Дата."""

        self.amount = amount
        """Сумма."""

        self.currency = currency
        """Валюта."""

        self.purpose = purpose
        """Назначение."""

        self.direction = direction
        """Строковое представление нправлени платежа."""

        self.recipient = recipient
        """Данные получателя."""

    def __str__(self):
        return f'{self.number} [{self.date}] {self.amount} {self.currency}'


class Statement(ApiEntity):
    """Сущность выписки."""

    __slots__ = ['account', 'date', 'payments', 'turnover_dt', 'turnover_ct', 'balance_opening', 'balance_closing']

    def __init__(
        self,
        *,
        account: str,
        on_date: date,
        payments: List[StatementPayment],
        turnover_dt: Decimal,
        turnover_ct: Decimal,
        balance_opening: Optional[Decimal],
        balance_closing: Optional[Decimal],
    ):
        self.account = account
        """Счёт."""

        self.date = on_date
        """Дата."""

        self.payments = payments
        """Платежи."""

        self.turnover_dt = turnover_dt
        """Обороты дебет."""

        self.turnover_ct = turnover_ct
        """Обороты кредит."""

        self.balance_opening = balance_opening
        """Входящее сальдо."""

        self.balance_closing = balance_closing
        """Исходящее сальдо."""

    def __str__(self):
        return f'{self.account} [{self.date}]'


class Statements(ApiResource):
    """Выписки"""

    cls_statement: Type[Statement] = Statement

    def get(self, *, accounts: List[str], on_date: Union[datetime, str], intraday: bool = False) -> List[Statement]:
        """

        :param accounts: Номера счетов, для которых требуется получить информацию.
        :param on_date: Дата выписки. Дата, за которую требуется получить данные.
        :param intraday: Следует ли получить данные промежуточной (внутридевной) выписки, а не итоговой.

        """
        result = []
        cast_statement = self._cast_statement

        statements = self._conn.request(url='statements/', data={
            'accounts': accounts,
            'on_date': self._cast_date(on_date),
            'intraday': self._cast_bool(intraday),
        })

        for statement in statements.data.get('items', []):
            result.append(cast_statement(statement))

        return result

    @classmethod
    def _cast_statement_payment(cls, item: dict) -> StatementPayment:

        result = StatementPayment(
            number=item['number'],
            on_date=datetime.strptime(item['date'], '%Y-%m-%d').date(),
            amount=Decimal(item['amount']),
            currency=item['currency'],
            purpose=item['purpose'],
            direction=item['direction'],
            recipient=item['recipient'],
        )

        return result

    @classmethod
    def _cast_statement(cls, item: dict) -> Statement:

        balance_opening = item.get('balance_opening')
        if balance_opening is not None:
            balance_opening = Decimal(balance_opening)

        balance_closing = item.get('balance_closing')
        if balance_closing is not None:
            balance_closing = Decimal(balance_closing)

        result = Statement(
            account=item['account'],
            on_date=datetime.strptime(item['date'], '%Y-%m-%d').date(),
            payments=list(map(cls._cast_statement_payment, item['payments'])),
            turnover_ct=Decimal(item['turnover_ct']),
            turnover_dt=Decimal(item['turnover_dt']),
            balance_opening=balance_opening,
            balance_closing=balance_closing,
        )

        return result
