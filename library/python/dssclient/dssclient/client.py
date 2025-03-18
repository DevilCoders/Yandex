from typing import Type, Union

from .auth import OwnerAuth
from .endpoints.certificates import Certificates
from .endpoints.documents import Documents
from .endpoints.policies import Policies
from .http import HttpConnector
from .resources.document import Document, File
from .signing_params import CadesSigningParams, CmsSigningParams, Ghost3410SigningParams, \
    MsOfficeSignarture, PdfSigningParams, XmlSigningParams


TypeConnectorCompat = Union[HttpConnector, str, dict]


class Dss:
    """Предоставляет интерфейс для взаимодествия с DSS сервером."""

    cls_document: Type[Document] = Document
    cls_file = File

    def __init__(self, http_connector: TypeConnectorCompat, host: str = None, timeout: int = None, retries: int = None):
        """
        :param http_connector: Объект клиента для обращения к DSS по HTTP.

        :param host: Имя хоста, на котором находится DSS. Будет использован протокол HTTPS.
            Если не указан, будет использован хост по умолчанию (см. .settings.HOST_DEFAULT).

        :param timeout: Таймаут на подключение. По умолчанию: 5 сек.

        :param retries: Максимальное число дополнительных попыток проведения запроса.
            По умолчанию: 7 штук.

        """
        if isinstance(http_connector, str):
            # Поддержка StaticAuth
            http_connector = HttpConnector(http_connector)

        elif isinstance(http_connector, dict):
            # Поддержка OwnerAuth
            http_connector = HttpConnector(OwnerAuth(**http_connector))

        http_connector.host = host
        http_connector.timeout = timeout
        http_connector.retries = retries

        self.connector = http_connector
        self.certificates = Certificates(http_connector)
        self.documents = Documents(http_connector)
        self.policies = Policies(http_connector)

    class signing_params:
        """Параметры подписания документов.
        Могут олицетворять как различные виды подписей,
        так и подиси для различных типов документов.

        """
        cades = CadesSigningParams
        cms = CmsSigningParams
        ghost3410 = Ghost3410SigningParams
        msoffice = MsOfficeSignarture
        pdf = PdfSigningParams
        xml = XmlSigningParams
