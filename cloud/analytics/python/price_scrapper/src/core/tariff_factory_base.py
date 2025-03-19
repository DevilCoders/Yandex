from abc import ABCMeta, abstractmethod
from typing import Optional
import re

from core.tariff import *
from core.page_scrapper_base import PageScrapperBase


class TariffFactoryBase(metaclass=ABCMeta):
    MULT_ONE_YEAR = 0.71
    MUL_THREE_YEARS = 0.54

    def __init__(self, company: str, scrapper: PageScrapperBase):
        if company is None:
            raise ValueError('Company name is expected')
        if scrapper is None:
            raise ValueError('Page scrapper is expected')
        if not (isinstance(scrapper, PageScrapperBase)):
            raise TypeError('Page scrapper but be a subclass of PageScrapperBase')
        self.scrapper = scrapper
        self._company = company
        self._url = scrapper.url

    @abstractmethod
    def get_compute(self) -> ComputeTariffMenu:
        pass

    @abstractmethod
    def get_storage(self) -> StorageTariffMenu:
        pass

    # 2-4-80 - means CONFIG_2_4_80 on demand
    # 2-4-80-1yr - means CONFIG_2_4_80 1 year reserve price
    def t(self, repr_str: str, price: float, comment='') -> ComputeTariff:
        return self._create_tariff(self._service_for(repr_str),
                                   self._commitment_for(repr_str),
                                   price,
                                   comment,
                                   oper_sys=OperSys.Linux)

    def win_t(self, repr_str: str, price: float, comment='') -> ComputeTariff:
        tariff = self.t(repr_str, price, comment)
        tariff.oper_sys = OperSys.Windows
        return tariff

    # (cold|std)_(read|write|per_gb) - pattern to create storage tarigg
    # For example, passing 'cold_per_gb' to method return StorageTariff.PER_SPACE
    def s(self, repr_str: str, price: float, comment='') -> StorageTariff:
        return self._storage_tariff(storage_type=self.get_stor_type_from(repr_str),
                                    service=self.get_stor_service_from(repr_str),
                                    price=price,
                                    comment=comment)

    @staticmethod
    def get_stor_type_from(repr_str: str) -> StorageType:
        TariffFactoryBase.assert_storage_repr_str(repr_str)
        if re.match(r'^cold_', repr_str):
            return StorageType.COLD
        elif re.match(f'^std_', repr_str):
            return StorageType.STD
        else:
            raise ValueError(f'Invalid representation string <{repr_str}> for storage type')

    @staticmethod
    def get_stor_service_from(repr_str: str) -> StorageServiceType:
        TariffFactoryBase.assert_storage_repr_str(repr_str)
        if repr_str.endswith('read'):
            return StorageServiceType.READ
        elif repr_str.endswith('write'):
            return StorageServiceType.WRITE
        elif repr_str.endswith('per_gb'):
            return StorageServiceType.PER_SPACE
        else:
            raise ValueError('Fatal error in processing storage service type')

    @staticmethod
    def assert_storage_repr_str(repr_str: str):
        if not re.match(r'^(cold|std)_(read|write|per_gb)', repr_str):
            raise ValueError(f'Invalid storage tariff pattern <{repr_str}>')

    def empty(self, repr_str: str) -> ComputeTariff:
        return self.empty_compute_tariff(self._service_for(repr_str), self._commitment_for(repr_str))

    def _service_for(self, repr_str: str) -> Service:
        self._assert_tariff_pattern(repr_str)

        if re.match(r'^2-4', repr_str):
            return Service.CONFIG_2_4_80
        elif re.match(r'^2-8', repr_str):
            return Service.CONFIG_2_8_160
        else:
            raise ValueError(f"Invalid tariff pattern: {repr_str}")

    def _commitment_for(self, repr_str: str) -> Commitment:
        self._assert_tariff_pattern(repr_str)
        if re.search(r'^(2-4-80|2-8-160)$', repr_str):
            return Commitment.ON_DEMAND
        if re.search(r'1yr$', repr_str):
            return Commitment.ONE_YEAR
        elif re.search(r'3yr$', repr_str):
            return Commitment.THREE_YEAR
        else:
            raise ValueError(f'Fail to get commitment from tariff pattern: <{repr_str}>')

    def _assert_tariff_pattern(self, repr_str: str):
        if not re.search(r'^(2-4-80|2-8-160)(-1yr|-3yr)?$', repr_str):
            raise ValueError(f'Tariff patter must be like <2-4-80-1yr>, but having <{repr_str}>')

    def empty_compute_tariff(self, service: Service, commitment: Commitment) -> ComputeTariff:
        return ComputeTariff(company=self._company,
                             commitment=commitment,
                             oper_sys=self._get_os_by_class_name(),
                             service=service,
                             currency=TariffCurrency.USD,
                             comment="Empty tariff",
                             with_vat=False,
                             price=0,
                             pricing_unit="",
                             link="No link")

    def _storage_tariff(self,
                        storage_type: StorageType,
                        service: StorageServiceType,
                        price: float, comment: str = '') -> StorageTariff:
        return StorageTariff(company=self._company,
                             currency=TariffCurrency.USD,
                             storage_type=storage_type,
                             service=service,
                             link=self._url,
                             comment=comment,
                             price=price,
                             with_vat=False,
                             pricing_unit=self._storage_pricing_unit(service))

    def _create_tariff(self,
                       service: Service,
                       commitment: Commitment,
                       price: float,
                       comment: str = '',
                       oper_sys: Optional[OperSys] = None) -> ComputeTariff:
        return ComputeTariff(company=self._company,
                             oper_sys=oper_sys or self._get_os_by_class_name(),
                             service=service,
                             commitment=commitment,
                             pricing_unit="$/month",
                             currency=TariffCurrency.USD,
                             with_vat=False,
                             link=self._url,
                             comment=comment,
                             price=price)

    def _get_os_by_class_name(self) -> OperSys:
        class_name = self.__class__.__name__.lower()
        return OperSys.Windows if "windows" in class_name else OperSys.Linux

    def _storage_pricing_unit(self, service: StorageServiceType):
        if service == StorageServiceType.PER_SPACE:
            return "Gb"
        elif service == StorageServiceType.READ:
            return "10000 requests"
        elif service == StorageServiceType.WRITE:
            return "1000 requests"
