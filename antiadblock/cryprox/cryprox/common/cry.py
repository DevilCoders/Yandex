# -*- coding: utf8 -*-

import base64
import hashlib
from time import strftime, localtime, time

import numpy

from antiadblock.libs.decrypt_url.lib import SEED_LENGTH
from antiadblock.cryprox.cryprox.config.service import END2END_TESTING


def shifted_localtime(seconds=0):
    if seconds == 0:
        return localtime()
    return localtime(time() + seconds)


def generate_seed(change_period=1, time_shift_minutes=0, salt=None, shifted_time_func=shifted_localtime):
    """
    Функция генерации seed-а, который испольузется в качестве соли для шифрования ссылок
    :param change_period: период смены сида в течение текущих суток, определяемых форматом "%Y%m%d"
    :param time_shift_minutes: сдвиг по времени смены сида.
                               Например, при значении 0 смена будет происходить в 00:00, при 10 - в 00:10
    :param salt: дополнительная соль для генерации seed-а
    :param shifted_time_func: функция получения текущей даты, принимающая (опционально) не количество секунд с начала
                              эпохи, а сдвиг в секундах относительно текущего времени.
                              По умолчанию shifted_localtime, меняется только в тестах
    :return:
    """
    now = shifted_time_func(-60 * time_shift_minutes) if time_shift_minutes != 0 else shifted_time_func()
    # для функционального тестирования фиксируем сид
    if END2END_TESTING:
        return b'64a476'
    return bytes(hashlib.sha512(strftime("%Y%m%d", now) + str(now.tm_hour / change_period) + (str(salt) if salt is not None else '')).hexdigest())[:SEED_LENGTH]


def crypto_xor(data, key):
    # key multiplication in order to match the data length
    key = (key * ((len(data) / len(key)) + 1))[:len(data)]

    # Select the type size in bytes
    if not len(data) % 8:
        dt = numpy.uint64
    elif not len(data) % 4:
        dt = numpy.uint32
    elif not len(data) % 2:
        dt = numpy.uint16
    else:
        dt = numpy.uint8
    return numpy.bitwise_xor(numpy.fromstring(key, dtype=dt), numpy.fromstring(data, dtype=dt)).tostring()


def encrypt_base64(bdata):
    return base64.urlsafe_b64encode(bdata).rstrip(b'=')


def encrypt_xor(bdata, key):
    return encrypt_base64(crypto_xor(bdata, key))
