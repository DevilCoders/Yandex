import re
from typing import List, Union

from .base import EndpointSigning
from ..resources.certificate import Certificate, CertificateRequest
from ..settings import AUTHORITY_TEST
from ..utils import ALIAS_TO_OID

if False:  # pragma: nocover
    from ..http import HttpConnector  # noqa


class CertificateRequests(EndpointSigning):
    """Предоставляет информацию о запросах на выпуск сертификата и позволяет управлять ими."""

    def get_all(self) -> List[CertificateRequest]:
        """Возвращает список всех запросов пользователя на сертификат."""
        requests = []

        for request in self._call('requests'):
            requests.append(CertificateRequest.spawn(request, endpoint=self))

        return requests

    def get(self, req_id: int, *, refresh=True) -> CertificateRequest:
        """Возвращает зарегистрированный ранее запрос на выпуск сертификата по его идентификатору.

        :param req_id:
        :param refresh: Следует ли сразу подтянуть данные запроса с сервера.

        """
        cert = CertificateRequest.spawn(req_id, endpoint=self)
        refresh and cert.refresh()

        return cert

    def register(
        self,
        *,
        subject: Union[str, dict],
        template: str,
        authority: int = None,
        pin: str = None,
        options: dict = None
    ) -> CertificateRequest:
        """Регистрирует запрос на сертификат с указанными параметрами.

        В зависимости от УЦ, в некоторых случаях регистрация запроса автоматически приводит к созданию сертификата.
        Например, такое поведение характерно для встроенного тестового УЦ - см. AUTHORITY_TEST.

        :param subject: Субъект сертификата.

            Внимание: различные УЦ могут требовать разные нароборы компонентов различительного имени субъекта.
                Например, для тестовго УЦ достаточно указанать CN (Common Name),
                в то время как для OutOfBand потребуются CN и E (E-mail).

            * В виде строки  - будет трактоваться, как Common Name (CN)
            * В виде словаря - будет трактоваться, как компоненты различительного имени субъекта
                               в виде пар {oid: значение}. См. ALIAS_TO_OID

        :param template: Идентификатор (объектный) шаблона сертификата, по которому создаётся запрос.

            Например:
                * 1.3.6.1.5.5.7.3.2
                * 1.2.643.2.2.50.1.9.3222149.8143850.12077221.13733360.27687.63267

        :param authority: Идентификатор удостоверяющего центра, к которому будет направлен запрос на сертификат.

            Если не указан, то будет использован идентификатор встроенного тестового УЦ.

        :param pin: Пин-код для доступа к закрытому ключу сертификата.

        :param options: Дополнительные параметры.

            Согласно документации:

                * RequestType       # Тип запроса. First, Renew
                * ActiveCertPin     # Пин-код действующего сертификата, котором следует подписать запроса.
                * ActiveCertId      # Идентификатор действующего сертификата, на котором следует подписать запрос.
                * EkuString         # Строка с EKU (Extended Key Usage).
                * TemplateOid       # Объектный ид сертификата.

        :rtype: CertificateRequest

        """
        authority = authority or AUTHORITY_TEST

        if isinstance(subject, str):
            subject = {
                ALIAS_TO_OID['common_name']: subject,
            }

        params = {
            'DistinguishedName': subject,
            'AuthorityId': authority,
            'Template': template,
        }

        if pin is not None:
            params['PinCode'] = pin

        if options is not None:
            params['Parameters'] = options

        response = self._call('requests', params, method='post')

        return CertificateRequest.spawn(response, endpoint=self)


class Certificates(EndpointSigning):
    """Предоставляет информацию о сертификатах и позволяет управлять ими."""

    def __init__(self, http_connector: 'HttpConnector'):
        super(Certificates, self).__init__(http_connector)

        self.requests = CertificateRequests(http_connector)
        """Запросы на выпуск сертификатов."""

    def get_all(self) -> List[Certificate]:
        """Возвращает список всех зарегистированных сертификатов."""
        certs = []

        for cert_info in self._call('certificates'):
            certs.append(Certificate.spawn(cert_info, endpoint=self))

        return certs

    def get(self, cert_id: int, *, refresh=True) -> Certificate:
        """Возвращает сертификат по его идентификатору.

        :param cert_id:
        :param refresh: Следует ли сразу подтянуть данные сертификата с сервера.

        """
        cert = Certificate.spawn(cert_id, endpoint=self)
        refresh and cert.refresh()

        return cert

    def register(self, base64: str) -> Certificate:
        """Регистрирует сертификат в DSS, возвращает его объект.

        Внимание: обратите внимание на то, что помимо загрузки сертификата возможен вариант,
            генерирования сертификата прямо на сервере. Это делает через запрос на создание
            сертификата. См. .requests.register()

        :param base64: Сертификат, кодированный в base64.

        """
        if '\n' in base64:
            # Вероятно отформатирован столбцом по 64 символа для читаемости. Склеим строки.
            base64 = re.sub('\s+', '', base64)

        return Certificate.spawn(self._call('certificates', {'certificate': base64}), endpoint=self)
