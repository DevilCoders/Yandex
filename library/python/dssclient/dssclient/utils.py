import logging
from os.path import abspath


OID_TO_ALIAS = {
    '2.5.4.3': 'common_name',
    '2.5.4.4': 'surname',
    '2.5.4.6': 'country',
    '2.5.4.5': 'serial',
    '2.5.4.7': 'locality',
    '2.5.4.8': 'state_province',
    '2.5.4.9': 'street',
    '2.5.4.10': 'organization',
    '2.5.4.11': 'organization_unit',
    '2.5.4.12': 'title',
    '2.5.4.17': 'zip',
    '2.5.4.41': 'name',
    '2.5.4.42': 'given_name',
    '2.5.4.43': 'initials',
    '2.5.4.65': 'pseudonym',
    '1.2.840.113549.1.9.1': 'email',
}
"""Соответствие идентификаторов объектов, используемых в различительных именах субъектов
сертификатов, псевдонимам, используемым в данном клиенте.

"""

ALIAS_TO_OID = dict((val, key) for key, val in OID_TO_ALIAS.items())
"""Соответствие псевдонимов объектов, используемым в данном клиенте, их идентификаторам,
используемым в различительных именах субъектов сертификатов.

"""


def configure_logging(level: int = logging.INFO):
    """Конфигурирует протоколирование.

    :param level: Минимальный уровень важности записей.

    """
    logging.basicConfig(level=level, format='%(levelname)s: %(message)s')


def int_to_hex(number: int) -> str:
    """Преобразует целое число в шестнадцатеричное представление с четным количеством цифр.

    :param number: Число.

    """
    result = '{:x}'.format(number)

    if len(result) % 2 != 0:
        return f'0{result}'

    return result


def file_write(filepath: str, contents: bytes):
    """Записывает в указанный файл указанные байты.

    :param filepath: Путь к файлу.
    :param contents:

    """
    with open(filepath, 'wb') as f:
        f.write(contents)


def file_read(filepath: str) -> bytes:
    """Считывает и возвращает байты из указанного файла.

    :param filepath: Путь к файлу.

    """
    filepath = abspath(filepath)

    with open(filepath, 'rb') as f:
        contents = f.read()

    return contents
