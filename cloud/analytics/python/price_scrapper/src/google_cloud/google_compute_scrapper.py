from selenium.webdriver.remote.webelement import WebElement

from core.exceptions import ScrappingException
from core.page_scrapper_base import PageScrapperBase


class GoogleComputeScrapper(PageScrapperBase):
    PageUrl = 'https://cloud.google.com/compute/vm-instance-pricing'
    PerCorePricePattern = r'^\$\d+\.\d+\s*\/\s*vCPU\s*hour$'
    PerRamPricePattern = r'^\$\d+\.\d+\s*\/\s*GB\s*hour$'

    WinLicenceCost: float = 67.16

    PerCorePrice = '.devsite-table-wrapper > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(1) > td:nth-child(2)'
    PerCorePrice_1yr = '.devsite-table-wrapper > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(1) > td:nth-child(4)'
    PerCorePrice_3yr = '.devsite-table-wrapper > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(1) > td:nth-child(5)'

    PerRamPrice = '.devsite-table-wrapper > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(2) > td:nth-child(2)'
    PerRamPrice_1yr = '.devsite-table-wrapper > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(2) > td:nth-child(4)'
    PerRamPrice_3yr = '.devsite-table-wrapper > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(2) > td:nth-child(5)'

    def __init__(self):
        super().__init__(self.PageUrl)

    def scrap_all(self):
        self._driver.switch_to.frame(4)
        # core prices
        self.put_from_element('PER_CORE_PER_HOUR', self.PerCorePrice, self._parse_region, self.PerCorePricePattern)
        self.put_from_element('PER_CORE_PER_HOUR_1YR', self.PerCorePrice_1yr, self._parse_region, self.PerCorePricePattern)
        self.put_from_element('PER_CORE_PER_HOUR_3YR', self.PerCorePrice_3yr, self._parse_region, self.PerCorePricePattern)

        # ram prices
        self.put_from_element('PER_RAM_PER_GB_PER_HOUR', self.PerRamPrice, self._parse_region, self.PerRamPricePattern)
        self.put_from_element('PER_RAM_PER_GB_PER_HOUR_1YR', self.PerRamPrice_1yr, self._parse_region, self.PerRamPricePattern)
        self.put_from_element('PER_RAM_PER_GB_PER_HOUR_3YR', self.PerRamPrice_3yr, self._parse_region, self.PerRamPricePattern)
        self.put_constant('SSD_MONTHLY_PRICE_PER_GB', '0.204')

    def _parse_region(self, key: str, elem: WebElement) -> str:
        region_price_value = elem.get_attribute('ffurt-hourly')
        if region_price_value is None:
            raise ScrappingException(message=f'No region attribute in element for key = <{key}>')
        return str(region_price_value)
