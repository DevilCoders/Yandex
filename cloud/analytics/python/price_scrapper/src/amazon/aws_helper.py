from abc import abstractmethod
from typing import Callable, Any, Union, Optional

from selenium.common.exceptions import NoSuchElementException
from selenium.webdriver.firefox.webdriver import WebDriver
from selenium.webdriver.firefox.options import Options

from core.tariff import *


class AwsHelper:

    def __init__(self, company: str, url):
        if company is None or len(company) == '':
            raise ValueError("Company name must be not empty")
        if url is None:
            raise ValueError("Url must be not empty")
        self._url = url
        self._company = company

    def get_compute(self) -> ComputeTariffMenu:
        return self._get_by_selenuim(self._scrap_compute)

    def get_storage(self) -> StorageTariffMenu:
        return self._get_by_selenuim(self._scrap_storage)

    def get_compute_ssd(self) -> SsdComputeTariff:
        return self._get_by_selenuim(self._scrap_compute_ssd)

    @abstractmethod
    def _scrap_compute(self, driver: WebDriver) -> ComputeTariffMenu:
        pass

    @abstractmethod
    def _scrap_storage(self, driver: WebDriver) -> StorageTariffMenu:
        pass

    @abstractmethod
    def _scrap_compute_ssd(self, driver: WebDriver) -> SsdComputeTariff:
        pass

    def parse_price(self, p: Any) -> float:
        if type(p) is float or type(p) is int:
            return p
        return float(p.replace('$', '')
                     .replace('GiB', '')
                     .replace('/ vCPU month', '')
                     .replace('/ GB month', '')
                     .replace(' USD per core/hour', ' ')
                     .replace(' per GB','')
                     .replace(' ', ''))

    def empty_compute_tariff(self, service: Service, commitment: Commitment) -> ComputeTariff:
        return ComputeTariff(company=self._company,
                             commitment=commitment,
                             oper_sys=self._get_os_by_class_name(),
                             service=service,
                             currency=TariffCurrency.USD,
                             comment="Empty tariff",
                             with_vat=False,
                             price=-1,
                             pricing_unit="",
                             link="No link")

    def empty_storage_tariff(self, service: StorageServiceType) -> StorageTariff:
        return StorageTariff(company=self._company,
                             currency=TariffCurrency.USD,
                             pricing_unit=self._get_pricing_unit(service),
                             with_vat=False,
                             service=service,
                             comment="Empty tariff",
                             price=-1,
                             link="No link",
                             storage_type=StorageType.STD)

    def _get_by_selenuim(self, scrapping_func: Callable[[WebDriver], Union[ComputeTariffMenu, StorageTariffMenu, SsdComputeTariff]]) -> Any:
        if scrapping_func is None:
            raise ValueError("Scrapping function that take 1 WebDriver argument is expected")
        options = Options()
        options.headless = True
        driver = WebDriver(options=options)
        driver.get(self._url)
        menu = None
        try:
            menu = scrapping_func(driver)
        except Exception as ex:
            print(ex)
        finally:
            driver.close()
        return menu

    def _get_text(self, driver: WebDriver, css_selector: str) -> str:
        text = 'n/a'
        try:
            elem = driver.find_element_by_css_selector(css_selector)
            text = elem.text
        except NoSuchElementException as ex:
            print(f"No such element exception (message: {ex}): {css_selector}")
        except Exception as ex:
            print(f"Fatal error ({ex}) when finding css selector :  [{css_selector}]")
        return text

    def _create_tariff(self,
                       service: Service,
                       commitment: Commitment,
                       price: Any,
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
                             price=self.parse_price(price))

    def _storage_tariff(self,
                        storage_type: StorageType,
                        service: StorageServiceType,
                        price: Any, comment: str = '') -> StorageTariff:
        return StorageTariff(company=self._company,
                             currency=TariffCurrency.USD,
                             storage_type=storage_type,
                             service=service,
                             link=self._url,
                             comment=comment,
                             price=self.parse_price(price),
                             with_vat=False,
                             pricing_unit=self._get_pricing_unit(service))

    def _get_os_by_class_name(self) -> OperSys:
        class_name = self.__class__.__name__.lower()
        return OperSys.Windows if "windows" in class_name else OperSys.Linux

    def _get_pricing_unit(self, service: StorageServiceType):
        if service == StorageServiceType.PER_SPACE:
            return "Gb"
        elif service == StorageServiceType.READ:
            return "10000 requests"
        elif service == StorageServiceType.WRITE:
            return "1000 requests"
