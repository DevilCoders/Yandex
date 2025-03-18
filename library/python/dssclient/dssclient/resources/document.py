from base64 import b64encode, b64decode
from os.path import basename
from typing import Union
from uuid import uuid4

from ..utils import file_read, file_write


class Document:
    """Подписываемый документ."""

    __slots__ = ['_contents', 'name', 'signed_base64']

    def __init__(self, contents: Union[str, bytes], *, name: str = None, encode: bool = True):
        """
        :param contents: Содержимое, которое требуется подписать.
        :param name: Имя документа. Если не задано, то будет сгенерировано.
        :param encode: Следует ли кодировать содержимое в base64 автоматически.

        """
        if encode:
            if not isinstance(contents, bytes):
                contents = contents.encode('utf-8')
            contents = b64encode(contents).decode('ascii')

        self._contents: str = contents

        self.name = name or f'{uuid4().hex}.dat'
        """Имя документа, либо файла."""

        self.signed_base64 = ''
        """Подпись (отдельно, либо с данными), закодированная в base64.
        Атрибут населяется в ходе вызова .documents.sign()

        """

    @property
    def signed_bytes(self) -> bytes:
        """Подпись (отдельно, либо с данными) в виде байт."""
        data = self.signed_base64

        if not data:
            return b''

        return b64decode(data)

    def _as_dict(self) -> dict:
        block = {
            'Content': self._contents,
            'Name': self.name,
        }
        return block

    def save_signed(self, filepath: str):
        """Сохраняет подпись в указанный файл.

        :param filepath:

        """
        file_write(filepath, self.signed_bytes)


class File(Document):
    """Файл, содержимое которого требуется подписать."""

    def __init__(self, name: str):
        contents = file_read(name)
        super(File, self).__init__(contents, basename(name))
