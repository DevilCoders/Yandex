# -*- coding: utf8 -*-
from enum import Enum


# Общие имена для адблоковых расширений
class AdblockTypes(Enum):
    ADBLOCK = 'AdBlock'  # https://getadblock.com
    ADBLOCK_PLUS = 'Adblock Plus'  # https://adblockplus.org/
    ADGUARD = 'AdGuard'  # https://adguard.com/en/welcome.html
    OPERA = 'OperaAdblock'
    UBLOCK_ORIGIN = 'uBlock Origin'  # https://github.com/gorhill/uBlock/
    WITHOUT_ADBLOCK = 'Without adblock'  # чистый браузер без блокировщиков
    WITHOUT_ADBLOCK_CRYPTED = 'Without adblock crypted'  # шифрованная версия без блокировщика
    INCOGNITO = 'Incognito mode'  # или приватный режим, у каждого браузера по своему называется

    def __init__(self, name: str):
        self.adb_name: str = name


# значение ширины монитора, необходимо для создания скриншотов, чтоб все были одинаковой ширины
VIRTUAL_MONITOR_WIDTH = 1920
VIRTUAL_MONITOR_HEIGHT = 1080
