# coding=utf-8
from enum import IntEnum

from antiadblock.cryprox.cryprox.common.tools.url import UrlClass
from . import adfox as adfox_config
from . import bk as bk_config
from . import rambler as rambler_config


class AdSystem(IntEnum):
    """
    Какие рекламные системы используются на сайте. В ad_systems.py устанавливается соответствие "рекламная система - доп.
    поля в конфиг"
    """
    BK = 1
    ADFOX = 2
    AWAPS = 3  # deprecated
    RAMBLER_SSP = 4

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return str(self.value)


AD_SYSTEM_CONFIG = {
    AdSystem.BK: bk_config,
    AdSystem.ADFOX: adfox_config,
    AdSystem.RAMBLER_SSP: rambler_config,
}

AD_SYSTEM_TAG_MAP = {
    AdSystem.BK: UrlClass.BK,
    AdSystem.ADFOX: UrlClass.ADFOX,
    AdSystem.RAMBLER_SSP: UrlClass.RAMBLER,
}

YANDEX_AD_SYSTEMS = {AdSystem.BK, AdSystem.ADFOX}
