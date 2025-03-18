from typing import Union, List

from .exceptions import ValidationError
from .resources.certificate import Certificate
from .stamping import Stamp, TypeStampCompat

TypeCertCompat = Union[Certificate, int, str]

DEFAULT_CERTIFICATE_ID = 0


class SigningParams:

    _id: int = None

    def __init__(self, certificate: TypeCertCompat = None, pin: str = None):
        """
        :param certificate: Объект сертификата, либо его идентификатор.
            Если не передан, то будет использоваться сертификат, назначенный по умолчанию.

        :param pin: Пин для разблокировки личного ключа.

        """
        self._params = {}

        certificate = certificate or DEFAULT_CERTIFICATE_ID

        if not isinstance(certificate, Certificate):  # Ожидаем int / str
            certificate = Certificate(certificate)

        self._cert = certificate
        self._pin = pin or ''

    def _get_params(self) -> dict:
        return self._params

    def _asdict(self) -> dict:
        block = {
            'Type': self._id,
            'CertificateId': self._cert.id,
            'PinCode': self._pin,
        }

        params = self._get_params()
        if params:
            block['Parameters'] = params

        return {'Signature': block}


class _WithHash:

    def sign_hash(self):
        """Подписать результат хеширования данных по ГОСТ Р 34.11 - 94
        вместо самих данных.

        """
        self._params.update({'Hash': 'True'})

        return self


class _WithAttached:
    """Примесь для подписей, поддерживающий прикрепленных и открепленный варианты.

    Внимание: по умолчанию используется прикрепленная подпись.

    """
    def make_attached(self):
        """Прикрепить подпись к документу.
        Иначе вернётся открепленная подпись.

        """
        self._params.update({'IsDetached': 'False'})

        return self

    def make_detached(self):
        """Затребовать открепленную подпись.
        Иначе вернётся подпись, прикрепленная к документу.

        """
        self._params.update({'IsDetached': 'True'})

        return self


class _WithTsp:

    def set_tsp_address(self, value: str):
        """Адрес TSP службы (Time-stamping Protocol).

        Используется только для CAdES XLT1 (CAdES X Long Type 1).

        """
        self._params.update({'TSPAddress': value})

        return self


class XmlSigningParams(SigningParams):
    """Подпись для XML документов."""

    _id: int = 0

    TYPE_ENVELOPED = 'XMLEnveloped'
    """Обёрнутая подпись (находится внутри корневого тега подписываемого документа)."""

    TYPE_ENVELOPING = 'XMLEnveloping'
    """Обрамляющая подпись (подписываемый документ внутри тега подписи)."""

    TYPE_TEMPLATED = 'XMLTemplate'
    """По шаблону (подпись вставляется в документ в то место, где расположен шаблон для неё).

    На заметку: шаблон подписи выглядит как подпись с заполненным SignedInfo
    и незаполненными SignatureValue и KeyInfo.

    """

    TYPES: List[str] = [
        TYPE_ENVELOPED,
        TYPE_ENVELOPING,
        TYPE_TEMPLATED,
    ]
    """Поддерживаемые типы XML подписей."""

    def __init__(self, certificate: TypeCertCompat = None, pin: str = None, type: str = TYPE_ENVELOPED):
        """

        :param certificate: Объект сертификата, либо его идентификатор.
            Если не передан, то будет использоваться сертификат, назначенный по умолчанию.

        :param pin: Пин для разблокировки личного ключа.

        :param type: Тип подписи. См. атрибуты TYPE_*
            По умолчанию: TYPE_ENVELOPED.

        """
        super(XmlSigningParams, self).__init__(certificate, pin)

        self.set_type(type)

    def set_type(self, type: str) -> 'XmlSigningParams':
        """Задаёт тип XML подписи.

        По умолчанию: TYPE_ENVELOPED.

        """
        if type not in self.TYPES:
            raise ValidationError(f'Unknown XML signature type provided: {type}.')

        self._params.update({'XMLDsigType': type})

        return self

    def set_type_enveloped(self) -> 'XmlSigningParams':
        """Устанавливает тип обёрнутая подпись (находится внутри подписываемого документа)."""
        return self.set_type(self.TYPE_ENVELOPED)

    def set_type_enveloping(self) -> 'XmlSigningParams':
        """Устанавливает тип обрамляющая подпись (подписываемый документ внутри её тега)."""
        return self.set_type(self.TYPE_ENVELOPING)

    def set_type_templated(self) -> 'XmlSigningParams':
        """Устанавливает тип подпись по шаблону (подпись вставляется на место её шаблона)."""
        return self.set_type(self.TYPE_TEMPLATED)


class Ghost3410SigningParams(_WithHash, SigningParams):
    """Подпись по ГОСТ Р 34.10 ‑ 2001"""

    _id: int = 1


class CadesSigningParams(_WithHash, _WithAttached, _WithTsp, SigningParams):
    """Подпись формата CAdES (CMS Advanced Electronic Signatures)."""

    _id: int = 2

    TYPE_BES = 'BES'
    """Подпись в формате CAdES BES (Basic Electronic Signature)."""

    TYPE_XLT1 = 'XLT1'
    """Подпись в формате CAdES X Long Type 1.

    X Long Type 1 =
        CAdES-BES +
        CAdES-T (Time stamp) +
        CAdES-C (Certificates and revocation IDs) +
        CAdES-X Type 1 (More time stamps) +
        (Certificates and revocation values)

    """

    TYPES: List[str] = [
        TYPE_BES,
        TYPE_XLT1,
    ]
    """Поддерживаемые типы подписей."""

    def __init__(self, certificate: TypeCertCompat = None, pin: int = None, type: str = TYPE_BES):
        """

        :param certificate: Объект сертификата, либо его идентификатор.
            Если не передан, то будет использоваться сертификат, назначенный по умолчанию.

        :param pin: Пин для разблокировки личного ключа.

        :param type: Тип подписи. См. атрибуты TYPE_*
            По умолчанию: TYPE_BES.

        """
        super(CadesSigningParams, self).__init__(certificate, pin)

        self.set_type(type)

    def set_type(self, type: str) -> 'CadesSigningParams':
        """Задаёт тип подписи.

        :param type:

        """
        if type not in self.TYPES:
            raise ValidationError(f'Unknown CAdES signature type provided: {type}.')

        self._params.update({'CAdESType': type})

        return self

    def set_type_bes(self) -> 'CadesSigningParams':
        """Устанавливает формат подписи CAdES BES."""
        return self.set_type(self.TYPE_BES)

    def set_type_xlt1(self) -> 'CadesSigningParams':
        """Устанавливает формат подписи CAdES X Long Type 1."""
        return self.set_type(self.TYPE_XLT1)


class PdfSigningParams(_WithTsp, SigningParams):
    """Подпись PDF документов."""

    _id: int = 3

    TYPE_CMS = 'CMS'  # Cryptographic Message Syntax
    """Подпись в формате CMS (PKCS7)."""  # Public Key Cryptography Standards

    TYPE_CADES = 'CAdES'
    """Подпись в формате CAdES."""

    TYPES: List[str] = [
        TYPE_CMS,
        TYPE_CADES,
    ]
    """Поддерживаемые типы подписей."""

    cls_stamp = Stamp

    def __init__(
        self,
        certificate: TypeCertCompat = None,
        pin: int = None,
        type: str = TYPE_CMS,
        stamp: TypeStampCompat = None
    ):
        """

        :param certificate: Объект сертификата, либо его идентификатор.
            Если не передан, то будет использоваться сертификат, назначенный по умолчанию.

        :param pin: Пин для разблокировки личного ключа.

        :param type: Тип подписи. См. атрибуты TYPE_*
            По умолчанию: TYPE_CMS.

        :param stamp: Объект штампа, который требуется добавить в документ.

        """
        super(PdfSigningParams, self).__init__(certificate, pin)

        self.reason = None
        """Цель подписания документа."""

        self.location = None
        """Место подписания документа."""

        self.stamp = None

        self.set_type(type)
        self.set_stamp(stamp)

    def _get_params(self) -> dict:

        reason = self.reason
        if reason:
            self._params['PDFReason'] = reason

        location = self.location
        if location:
            self._params['PDFLocation'] = location

        stamp = self.stamp
        if stamp is not None:
            self._params['PdfSignatureAppearance'] = stamp.serialize()
            self._params['PdfSignatureTemplateId'] = stamp._template_id

        return super(PdfSigningParams, self)._get_params()

    def set_stamp(self, stamp: TypeStampCompat, page: int = None):
        """Задаёт штамп (печать), который следует расположить на одной из страниц документа.

        :param stamp:
        :param page: Номер страницы, на которую следует поместить печать.

        """
        if stamp and not isinstance(stamp, self.cls_stamp):
            stamp = self.cls_stamp(stamp)

        if page is not None:
            stamp.page = page

        self.stamp = stamp

    def set_type(self, type: str) -> 'PdfSigningParams':
        """Задаёт тип подписи.

        :param str|unicode type:
        """
        if type not in self.TYPES:
            raise ValidationError(f'Unknown PDF signature type provided: {type}.')

        self._params.update({'PDFFormat': type})

        return self

    def set_type_cms(self) -> 'PdfSigningParams':
        """Устанавливает формат CMS (PKCS7) для хранения подписи."""
        return self.set_type(self.TYPE_CMS)

    def set_type_cades(self) -> 'PdfSigningParams':
        """Устанавливает формат CAdES для хранения подписи."""
        return self.set_type(self.TYPE_CADES)


class MsOfficeSignarture(SigningParams):
    """Подпись документов MS Word и Excel."""

    _id: int = 4


class CmsSigningParams(_WithHash, _WithAttached, SigningParams):
    """Подпись формата CAdES BES (Basic Electronic Signature)."""

    _id: int = 5
