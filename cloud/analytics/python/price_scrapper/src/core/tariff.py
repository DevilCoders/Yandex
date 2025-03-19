from copy import copy
from dataclasses import dataclass
from enum import Enum


class TariffCurrency(Enum):
    USD = 'USD',
    RUB = 'RUB'


class StorageServiceType(Enum):
    PER_SPACE = 'per_space'
    READ = 'read'
    WRITE = 'write'


class Commitment(Enum):
    ON_DEMAND = 'on_demand'
    ONE_YEAR = '1yr'
    THREE_YEAR = '3yr'


class Service(Enum):
    CONFIG_2_4_80 = '2_4_80'
    CONFIG_2_8_160 = '2_8_160'


class OperSys(Enum):
    Windows = 'windows'
    Linux = 'linux'


class StorageType(Enum):
    COLD = 'COLD'
    STD = 'STD'


@dataclass
class Tariff:
    company: str
    currency: TariffCurrency
    pricing_unit: str
    price: float
    link: str
    comment: str
    with_vat: bool


@dataclass
class ComputeTariff(Tariff):
    service: Service
    commitment: Commitment
    oper_sys: OperSys

    def __str__(self) -> str:
        return f"[{self.service} ({self.commitment}) = {self.price} ({self.oper_sys})]"
        # return f"[Compute tariff: for service [{self.service}] in {self.company}. " \
        #        f"Price = {round(self.price, 3)} " \
        #        f"{self.currency} for {self.pricing_unit}]"


    def __repr__(self):
        return self.__str__()

    def clone(self) -> "ComputeTariff":
        menu = copy(self)
        menu.service = self.service
        menu.commitment = self.commitment
        menu.oper_sys = self.oper_sys
        return menu

    def clone_and_inc_price(self, delta: float) -> "ComputeTariff":
        menu = self.clone()
        menu.price += delta
        return menu


# NO_TARIFF = ComputeTariff(company="",
#                           commitment=Commitment.ON_DEMAND,
#                           oper_sys=OperSys.Linux,
#                           service=Service.CONFIG_2_4_80,
#                           currency=TariffCurrency.USD,
#                           comment="Empty tariff",
#                           with_vat=False,
#                           price=-1,
#                           pricing_unit="",
#                           link="No link")


# TODO: add pricing unit
@dataclass
class StorageTariff(Tariff):
    storage_type: StorageType
    service: StorageServiceType

    def __str__(self) -> str:
        return f"[Storage tariff: storage_type = {self.storage_type}, service = {self.service}, price = {self.price}]"

    def __repr__(self):
        return self.__str__()


@dataclass
class SsdComputeTariff(Tariff):

    def __str__(self):
        return f"[SSD for compute: price = {self.price} ({self.comment})]"

    def __repr__(self):
        return self.__str__()


@dataclass
class ComputeTariffMenu:
    oper_system: OperSys.Linux
    config_2_4_80: ComputeTariff
    config_2_4_80_1yr: ComputeTariff
    config_2_4_80_3yr: ComputeTariff

    config_2_8_160: ComputeTariff
    config_2_8_160_1yr: ComputeTariff
    config_2_8_160_3yr: ComputeTariff

    def clone_and_inc_price(self, delta: float) -> "ComputeTariffMenu":
        return ComputeTariffMenu(
            oper_system=self.oper_system,
            config_2_4_80=self.config_2_4_80.clone_and_inc_price(delta),
            config_2_4_80_1yr=self.config_2_4_80_1yr.clone_and_inc_price(delta),
            config_2_4_80_3yr=self.config_2_4_80_3yr.clone_and_inc_price(delta),
            config_2_8_160=self.config_2_8_160.clone_and_inc_price(delta),
            config_2_8_160_1yr=self.config_2_8_160_1yr.clone_and_inc_price(delta),
            config_2_8_160_3yr=self.config_2_8_160_3yr.clone_and_inc_price(delta)
        )


@dataclass
class StorageTariffMenu:
    std_per_space: StorageTariff
    std_read: StorageTariff
    std_write: StorageTariff

    cold_per_space: StorageTariff
    cold_read: StorageTariff
    cold_write: StorageTariff
