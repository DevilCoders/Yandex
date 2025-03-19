from enum import Enum


class StrEnum(Enum):
    def __str__(self) -> str:
        return str(self.value)


class Product(StrEnum):
    Yandex = 'yandex'
    Azure = 'azure'


class ServiceBranch(StrEnum):
    Stable = 'stable'
    RC = 'rc'
    Deprecated = 'deprecated'


class Language(StrEnum):
    Auto = 'auto'
    Ru = 'ru-RU'
    Kk = 'kk-KK'
    En = 'en-US'
    De = 'de-DE'
    Fr = 'fr-FR'
    Tr = 'tr-TR'
    Fi = 'fi-FI'
    Sv = 'sv-SV'
