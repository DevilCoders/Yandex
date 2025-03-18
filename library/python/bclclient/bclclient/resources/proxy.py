from typing import Dict

from .base import ApiResource
from ..exceptions import ApiCallError
from ..http import Connector


def proxy_request(*, connection, url: str, data: dict):
    response = connection.request(url=url, data=data, method='post', raise_on_error=False)

    if response.errors:
        raise ApiCallError(response.errors[0], status_code=response.status)

    return response.data


class PayPal(ApiResource):
    """Проброс запросов в PayPal."""

    def get_userinfo(self, *, token: str) -> Dict:
        """Возвращает информацию о пользователе PayPal, связанном с указанным токеном.

        Пример:
            {
                'language': 'en_US',
                'account_type': 'BUSINESS',
                'email_verified': true,
                ...
            }

        """
        return proxy_request(connection=self._conn, url='proxy/paypal/getuserinfo/', data={'token': token})

    def get_token(self, *, auth_code: str) -> Dict:
        """Получает данные токена PayPal для указанного кода авторизации.

        Пример:
            {'token_type': 'Bearer', 'expires_in': '1000', 'refresh_token': 'yyy', 'access_token': 'zzz'}

        """
        return proxy_request(connection=self._conn, url='proxy/paypal/gettoken/', data={'auth_code': auth_code})


class Payoneer(ApiResource):
    """Проброс запросов в Payoneer."""

    def get_payee_status(self, *, program_id: str, payee_id: str):
        """Позволяет получить статус указанного получателя в указанной программе.

        :param program_id: ID программы в Payoneer.
            Программа должна быть связана с сервисом, который обращается к RPC в настройках BCL.

        :param payee_id: ID получателя, статус которого требуется узнать.

        """
        return proxy_request(connection=self._conn, url='proxy/payoneer/getpayeestatus/', data={
            'program_id': program_id,
            'payee_id': payee_id,
        })

    def get_login_link(self, *, program_id: str, payee_id: str, options: dict = None):
        """Позволяет получить ссылку для регистрации пользователя Payoneer в указанной программе.

        Пример:
            {'audit_id': 56025866, 'code': 0, 'description': 'Success',
            'login_link': 'https://payoneer.com/partners/lp.aspx?xxx'}

        :param program_id: ID программы в Payoneer.
            Программа должна быть связана с сервисом, который обращается к RPC в настройках BCL.

        :param payee_id: ID получателя.

        :param options: Дополнительные необязательные параметры.
            Имена параметров ожидаются те же, что ожидает Payoneer в 'payees/login-link'.

        """
        return proxy_request(connection=self._conn, url='proxy/payoneer/getloginlink/', data={
            'program_id': program_id,
            'payee_id': payee_id,
            'options': options or {},
        })


class PingPong(ApiResource):
    """Проброс запросов в PingPong."""

    def get_seller_status(self, *, seller_id: str) -> Dict:
        """Позволяет получить статус указанного продавца.

        Пример:
            {'seller_id': '10', 'status': 'Approved', 'user_id': '123456'}

        :param seller_id: Уникальный идентификатор магазина в PingPong.

        """
        return proxy_request(connection=self._conn, url='proxy/pingpong/getsellerstatus', data={'seller_id': seller_id})

    def get_onboarding_link(
        self,
        *,
        seller_id: str,
        currency: str,
        country: str,
        store_name: str,
        store_url: str,
        notify_url: str
    ) -> Dict:
        """Позволяет получить ссылку для регистрации продавца.

        Пример:
            {'seller_Id': '10', 'token': 'xxx', 'redirect_url': 'http://notify.url?token=xxx'}

        :param seller_id: Уникальный идентификатор продавца в PingPong (на этот ID отправляются платежи).

        :param currency: Валюта, в которой магазин будет получать платежи (код по ISO 4217). Пример : USD.

        :param country: Страна магазина (код по ISO 3166). Пример: US.

        :param store_name: Наименование магазина.

        :param store_url: URL магазина.

        :param notify_url: URL-адрес, используемый PingPong, чтобы перенаправить пользователя
            после завершения регистрации.

        """
        return proxy_request(connection=self._conn, url='proxy/pingpong/getonboardinglink', data={
            'seller_id': seller_id,
            'currency': currency,
            'country': country,
            'store_name': store_name,
            'store_url': store_url,
            'redirect_url': notify_url,
        })


class Proxy(ApiResource):
    """Инструменты для проброса запросов во внешние системы."""

    def __init__(self, connector: Connector):
        super().__init__(connector)

        self.paypal = PayPal(connector)
        self.payoneer = Payoneer(connector)
        self.pingpong = PingPong(connector)
