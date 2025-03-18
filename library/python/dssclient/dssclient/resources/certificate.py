from base64 import b64decode, b64encode
from datetime import datetime
from importlib import import_module
from io import StringIO

from .base import Resource
from ..exceptions import ValidationError
from ..utils import int_to_hex, OID_TO_ALIAS


class CertificateParser:
    """Разборщик сертификата. Предоставляет инструменты для изъятия нужных данных непосредственно
    из тела сертификата."""

    def __init__(self, base64: str):
        self.parsed = self.parse_pem(base64)

    @classmethod
    def _parse_date_utc(cls, src: dict) -> datetime:
        not_before = str(src['utcTime'])
        return datetime.strptime(not_before, '%y%m%d%H%M%SZ')

    @classmethod
    def decode_asn(cls, value: bytes, type_name: str, *, spec: str = 'rfc2459'):
        """Декодирует указанное ASN значение.

        :param value: Значение для декодирования.

        :param type_name: Имя типа, которое следует использовать для приведения (декодирования).
            Оно же имя типа в модуле ANS.

        :param spec: Имя спецификации, описывающей формат сертификата,
            разборщик для которого требуется задействовать. Оно же имя модуля ASN.

            Примеры:
                rfc2459 - X.509 (по умолчанию)
                rfc2315 - PKCS

        """
        from pyasn1.codec.der import decoder

        spec_module = import_module(f'pyasn1_modules.{spec}')
        decoded, _ = decoder.decode(value, asn1Spec=getattr(spec_module, type_name)())

        return decoded

    @classmethod
    def parse_pem(cls, base64: str) -> dict:
        """Возвращет ASN-объект из PEM, кодированного в base64.

        :param base64:

        """
        from pyasn1_modules import pem

        raw = pem.readBase64FromFile(StringIO(base64))
        cert_data = cls.decode_asn(raw, 'Certificate')

        return cert_data

    @property
    def date_issued(self) -> datetime:
        """Возвращает дату выдачи сертификата."""
        # todo Возможно ещё и Generalized time поддержать (%Y%m%d%H%M%SZ).
        return self._parse_date_utc(self.parsed['tbsCertificate']['validity']['notBefore'])

    @property
    def date_expires(self) -> datetime:
        """Возвращает дату стечения действия сертификата."""
        # todo Возможно ещё и Generalized time поддержать (%Y%m%d%H%M%SZ).
        return self._parse_date_utc(self.parsed['tbsCertificate']['validity']['notAfter'])

    def check_valid_on_date(self, dt: datetime = None) -> bool:
        """Проверяет валидность сертификата в указанное время.

        :param dt: Датавремя в UTC. Ели не указано, то будет использловаться текущее.

        """
        dt = dt or datetime.utcnow()
        return self.date_issued <= dt < self.date_expires

    @property
    def public_key_bytes(self) -> bytes:
        """Публичный ключ в виде байтов."""
        return self.parsed['tbsCertificate']['subjectPublicKeyInfo']['subjectPublicKey'].asOctets()

    @property
    def public_key_base64(self) -> str:
        """Публичный ключ в шестнадцатиричном представлении."""
        return b64encode(self.public_key_bytes).decode('utf-8')

    @property
    def serial(self) -> str:
        """Серийный номер сертификата в шестнадцатиричном представлении.

        Например: 090ee45d1500d380e711eda1332bf5f7

        """
        serial_number = int(self.parsed['tbsCertificate']['serialNumber'])
        return int_to_hex(serial_number)

    @property
    def subject(self) -> dict:
        """Вынимает из сертификата информацию о субъекте и возвращает её в виде словаря.

        Словарь индексирован псевдонимами сущностей. Если для объекта не удалось подобрать имя сущности,
        то запись будет индексирована идентификатором объекта.

        """
        oid_map = OID_TO_ALIAS
        subject = self.parsed['tbsCertificate']['subject'][0]

        result = {}

        for item in subject:
            item = item[0]

            item_oid = str(item['type'])
            item_alias = oid_map.get(item_oid, item_oid)
            item_value = self.decode_asn(item['value'], 'DirectoryString')
            item_value = str(item_value.getComponent())

            result[item_alias] = item_value

        return result


class CertificateRequest(Resource):
    """Запрос на выпуск сертификата."""

    _url_base = 'requests'

    STATUS_PENDING = 'PENDING'
    """К обработке."""

    STATUS_PROCESSED = 'ACCEPTED'
    """Обработан."""

    STATUS_REJECTED = 'REJECTED'
    """Отклонён"""

    STATUS_REGISTERING = 'REGISTRATION'
    """Регистрация."""

    TYPE_ISSUE = 'Certificate'
    """Запрос на выпуск, обновление сертификата."""

    TYPE_REVOKE = 'RevokeRequest'
    """Запрос на отзыв, восстановление, приостановление сертификата."""

    @property
    def type_code(self) -> str:
        """Код (текстовое представление) типа запроса."""
        return self._get_data_item('RequestType')

    @property
    def type_is_issue(self) -> bool:
        """Является ли запрос запросом на выпуск, обновление сертификата."""
        return self.type_code == self.TYPE_ISSUE

    @property
    def type_is_revoke(self) -> bool:
        """Является ли запрос запросом на отзыв, восстановление, приостановление сертификата."""
        return self.type_code == self.TYPE_REVOKE

    @property
    def status_code(self) -> str:
        """Код (текстовое представление) статуса."""
        return self._get_data_item('Status')

    @property
    def status_is_pending(self) -> bool:
        """Назначен ли запрос к обработке."""
        return self.status_code == self.STATUS_PENDING

    @property
    def status_is_processed(self) -> bool:
        """Успешно обработан ли запрос."""
        return self.status_code == self.STATUS_PROCESSED

    @property
    def status_is_rejected(self) -> bool:
        """Отклонён ли запрос."""
        return self.status_code == self.STATUS_REJECTED

    @property
    def status_is_registering(self) -> bool:
        """Находится ли запрос в процессе регистрации."""
        return self.status_code == self.STATUS_REGISTERING

    @property
    def body_bytes(self) -> bytes:
        """Запрос в байтах."""
        return b64decode(self.body_base64)

    @property
    def body_base64(self) -> str:
        """Запрос в формате base64."""
        return self._get_data_item('Base64Request')

    @property
    def subject(self) -> str:
        """Строка субъекта.

        Например: CN=idlesign, OU=Yandex, O=Yandex, L=Novosibirsk, S=Novosibirsk, C=RU

        """
        return self._get_data_item('DistName')

    @property
    def common_name(self) -> str:
        """Общее имя субъекта."""
        return self._get_data_item('Subject')

    @property
    def authority_id(self) -> int:
        """Идентификатор УЦ."""
        return self._get_data_item('CertificateAuthorityID')

    @property
    def authority_title(self) -> str:
        """Наименование УЦ."""
        return self._get_data_item('CADisplayName')

    @property
    def authority_request_id(self) -> str:
        """Идентификатор этого запроса в УЦ."""
        return self._get_data_item('CARequestID')

    @property
    def certificate_id(self) -> int:
        """Ид сертификата, с которым связан запрос."""
        return self._get_data_item('CertificateID')

    def delete(self):
        """Удаляет запрос на сертификат."""
        return self._endpoint._call(self._get_entity_url(), method='delete')

    def set_status(self, status: str):
        """Обновляет данные статуса запроса на сертификат.

        :param status:

        """
        # todo Request Status obj
        return self._endpoint._call(self._get_entity_url('status'), {'status': status}, method='patch')


class Certificate(Resource):
    """Сертификат."""

    _url_base = 'certificates'

    STATUS_VALID = 'ACTIVE'
    STATUS_REVOKED = 'REVOKED'
    STATUS_SUSPENDED = 'HOLD'
    STATUS_INVALID = 'NOT_VALID'
    STATUS_READ_ONLY = 'OUT_OF_ORDER'

    def __init__(self, id: int = None):
        self._parsed = None
        super(Certificate, self).__init__(id)

    @property
    def is_active(self) -> bool:
        """Действует ли сертификат."""
        return self.status_code in {self.STATUS_VALID, self.STATUS_READ_ONLY}

    @property
    def is_inactive(self) -> bool:
        """Не действует ли сертификат."""
        return self.status_code in {self.STATUS_REVOKED, self.STATUS_SUSPENDED, self.STATUS_INVALID}

    @property
    def status_is_valid(self) -> bool:
        """Действителен ли сертификат."""
        return self.status_code == self.STATUS_VALID

    @property
    def status_is_invalid(self) -> bool:
        """Является ли сертификат невалидным."""
        return self.status_code == self.STATUS_INVALID

    @property
    def status_is_revoked(self) -> bool:
        """Отозван ли сертификат."""
        return self.status_code == self.STATUS_REVOKED

    @property
    def status_is_suspended(self) -> bool:
        """Приостановлено ли действие сертификата."""
        return self.status_code == self.STATUS_SUSPENDED

    @property
    def status_is_read_only(self) -> bool:
        """Является ли сертификат доступным только для чтения
        (отзыв и приостановка таких сертификатов невозможны).

        """
        return self.status_code == self.STATUS_READ_ONLY

    @property
    def status_code(self) -> str:
        """Код (текстовое представление) статуса."""
        return self._get_data_item('Status')['Value']

    @property
    def body_bytes(self) -> bytes:
        """Сертификат в байтах."""
        return b64decode(self.body_base64)

    @property
    def body_base64(self) -> str:
        """Сертификат в формате base64."""
        return self._get_data_item('CertificateBase64')

    @property
    def csp_id(self) -> str:
        """Идентификатор в CSP.

        Например: 7a76f462-4204-4282-a6d5-687774bfb49f

        """
        return self._get_data_item('CspID')

    @property
    def parsed(self) -> CertificateParser:
        """Возвращает инициализированный объект разборщика сертификата, позволяющий
        получить из тела сертификата дополнительные полезные данные.

        """
        if self._parsed is None:
            self._parsed = CertificateParser(self.body_base64)

        return self._parsed

    @property
    def serial(self) -> str:
        """Серийный номер сертификата в шестнадцатиричном представлении."""
        return self.parsed.serial

    @property
    def subject(self) -> str:
        """Строка субъекта.

        Например: CN=perseus, OU=Yandex, O=Yandex, L=Novosibirsk, S=Novosibirsk, C=RU

        """
        return self._get_data_item('DName')

    @property
    def authority_id(self) -> int:
        """Идентификатор УЦ."""
        return self._get_data_item('CertificateAuthorityID')

    @property
    def is_default(self) -> bool:
        """Является ли сертификатом по умолчанию."""
        return self._get_data_item('IsDefault')

    def delete(self):
        """Удаляет сертификат."""
        return self._endpoint._call(self._get_entity_url(), method='delete')

    def set_default(self, default: bool = True):
        """Делает сертификат сертификатом используемемы по умолчанию, либо снимает этот флаг."""
        return self._endpoint._call(self._get_entity_url('default'), {'Default': int(default)}, method='patch')

    def set_friendly_name(self, name: str):
        """Устанавливает сертификату дружественное имя.

        .. note:: Для удаления дружественного имени требуется передать пустую строку.

        """
        if len(name) > 255:
            raise ValidationError('Friendly name must be no more than 255 characters long.')

        return self._endpoint._call(self._get_entity_url('friendlyName'), {'FriendlyName': name}, method='patch')

    def set_status(self, status: str):
        """Обновляет данные статуса сертификата.

        :param status:

        """
        # todo Certificate Status obj
        return self._endpoint._call(self._get_entity_url('status'), {'status': status}, method='patch')

    def set_pin(self, pin: str):
        """Отправляет запрос на смену PIN для доступа к личному ключу сертификата.

        :param pin:

        """
        self._endpoint._call(self._get_entity_url('pin'), {'pinRequest': pin}, method='patch')
        return True
