#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""This script get info about memory banks from BOT api and local
hw info and comparing them"""

import socket
import time
import dmidecode
from requests import Session


BOT_API_URL = "http://bot.yandex-team.ru/api"


class Status(object):
    OK = 0
    WARNING = 1
    CRITICAL = 2


class Check(object):
    NAME = "memory_bank"
    TYPE = "PASSIVE-CHECK"


class Messages(object):
    OK = "OK"
    DMIDECODE_READ_ERR = "Can't get memory bank via dmidecode"
    BOTAPI_READ_ERR = "Can't get memory bank via BOT api"
    MEMORY_BANK_UNAVAILABLE_ERR = "GB of RAM unavailable"


class MessagePrinter(object):

    def __init__(self):
        self._message_severity = []
        self._message_text = []

    def add(self, message_severity, message_text):
        self._message_severity.append(message_severity)
        self._message_text.append(message_text)

    @property
    def messages_count(self):
        return len(self._message_severity)

    @property
    def out(self):
        return "{}:{};{};{}".format(
            Check.TYPE,
            Check.NAME,
            max(self._message_severity),
            "; ".join(self._message_text)
        )


def get_bank_size_in_gb(size_as_str):
    """
    convert MB to GB units
    :rtype: int
    :return memory bank size (GB)
    """
    value, unit = size_as_str.split()
    size = int(value) / 1024 if unit == 'MB' else int(value)
    return size


def get_local_fqdn():
    """
    Gets fqdn hostname (like hostname -f)
    :return fqdn_hostname: Fully Qualified Domain Name
    """
    fqdn_hostname = socket.getfqdn()

    return fqdn_hostname


def get_total_mem_from_os():
    """
    Gets info about all memory banks size via dmidecode
    :rtype: int
    :return memory_total: sum of memory banks size
    """
    memory_info = dmidecode.memory()

    banks_size = [x for x in (value['data'].get('Size') for value in memory_info.values()) if x]

    memory_total = sum(map(get_bank_size_in_gb, banks_size))

    dmidecode.clear_warnings()

    return memory_total


def get_total_mem_from_bot():
    """
    Gets info about all installed memory banks size via bot.yandex-team.ru/api
    :rtype: int
    :return memory_total: sum of installed memory banks size (KB)
    """
    query = "{}{}{}{}".format(BOT_API_URL,
                              "/consistof.php?name=", get_local_fqdn(), "&format=json")

    try:
        max_retry = 5
        timeout = 3

        for _ in range(max_retry):
            s = Session()
            response = s.get(query)
            if response.status_code != 200:
                time.sleep(timeout)
            else:
                break

        memory_total = sum(
            [int(component['attribute13']) for component in response.json()['data']['Components']
                if component['item_segment3'] == 'RAM'])

    except Exception as e:
        return 0

    return memory_total


def main():

    message = MessagePrinter()

    loc_ram_size = get_total_mem_from_os()
    bot_ram_size = get_total_mem_from_bot()

    if loc_ram_size == 0:
        message.add(Status.WARNING, Messages.DMIDECODE_READ_ERR)

    if bot_ram_size == 0:
        message.add(Status.WARNING, Messages.BOTAPI_READ_ERR)

    if bot_ram_size > loc_ram_size:
        message.add(Status.CRITICAL, "{} {}".format(
            bot_ram_size - loc_ram_size,
            Messages.MEMORY_BANK_UNAVAILABLE_ERR,
        ))

    if message.messages_count == 0:
        message.add(Status.OK, Messages.OK)

    return message.out


if __name__ == '__main__':
    print(main())
    exit(0)
