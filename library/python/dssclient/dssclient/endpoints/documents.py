from typing import Union, List
from uuid import uuid4

from .base import EndpointSigning
from ..resources.document import Document
from ..signing_params import SigningParams

TypeDocumentCompat = Union[Document, List[Document], str, bytes]


class Documents(EndpointSigning):
    """Предоставляет информацию о документах и позволяет подписывать их."""

    @classmethod
    def _spawn(cls, documents) -> List[Document]:
        """Создаёт и возвращает список объектов документов.

        :param documents:  Один или несколько документов,
            которые требуется подписать. Здесь может быть как объект-наследник Document,
            так и строки/байты, которые следует подписать.

        """
        if not isinstance(documents, list):
            documents = [documents]

        result = []

        for document in documents:
            if not isinstance(document, Document):  # Предпологаем строку с содержимым.
                document = Document(document)
            result.append(document)

        return result

    def sign(self, documents: TypeDocumentCompat, params: SigningParams) -> List[Document]:
        """Отправляет документ(ы) на подпись.

        :param documents: Один или несколько документов,
            которые требуется подписать. Здесь может быть как объект-наследник Document,
            так и строки/байты, которые следует подписать.

            Если отправлено несоклько документов, по подписывание будет осуществляться пакетно.

        :param params: Объект параметров подписания. См. Dss.signing_params.

        """
        payload = {}
        payload.update(params._asdict())

        documents = self._spawn(documents)

        if not documents:
            return []

        endpoint = 'documents'

        mode_single = len(documents) == 1

        if mode_single:
            payload.update(documents[0]._as_dict())

        else:
            # Производим пакетную подпись.
            endpoint += '/packagesignature'

            payload.update({
                'Documents': [document._as_dict() for document in documents],
                'Name': f'batch_{uuid4()}.dat'
            })

        response = self._call(endpoint, data=payload)

        if mode_single:
            documents[0].signed_base64 = response

        else:

            for idx, signed_b64 in enumerate(response['Results']):
                # todo проверить разбрасывание по индексам в свете существования Errors
                documents[idx].signed_base64 = signed_b64

        return documents
