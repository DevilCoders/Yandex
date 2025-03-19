import json

from selenium.webdriver.remote.webelement import WebElement

from core.exceptions import ScrappingException
from core.utils import json_attr
from core.page_scrapper_base import PageScrapperBase


class AzureComputeScrapper(PageScrapperBase):
    PageUrl = 'https://azure.microsoft.com/en-us/pricing/details/virtual-machines/linux/'
    NumericPricePattern = r'^[0-9]+\.[0-9]*$'
    StoragePattern = r'^\d+\s+GiB$'

    A2v2 = '#GeneralPurpose-table-content > div:nth-child(2) > div:nth-child(2) > div:nth-child(1) > div:nth-child(2) > ' \
           'table:nth-child(1) > tbody:nth-child(2) > ' \
           'tr:nth-child(2) > td:nth-child(5) > span:nth-child(1) > span:nth-child(1)'

    D2v3_on_demand = '#GeneralPurpose-table-content > div:nth-child(17) > div:nth-child(2) > div:nth-child(1) > ' \
                     'div:nth-child(2) > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(1) > ' \
                     'td:nth-child(5) > span:nth-child(1) > span:nth-child(1)'

    D2v3_one_year = '#GeneralPurpose-table-content > div:nth-child(17) > div:nth-child(2) > ' \
                    'div:nth-child(1) > div:nth-child(2) > table:nth-child(1) > tbody:nth-child(2) > ' \
                    'tr:nth-child(1) > td:nth-child(6) > span:nth-child(1) > span:nth-child(1)'

    D2v3_three_years = '#GeneralPurpose-table-content > div:nth-child(17) > div:nth-child(2) > ' \
                       'div:nth-child(1) > div:nth-child(2) > table:nth-child(1) > tbody:nth-child(2) > ' \
                       'tr:nth-child(1) > td:nth-child(7) > span:nth-child(1) > span:nth-child(1)'

    A2v2_storage = '#GeneralPurpose-table-content > div:nth-child(2) > div:nth-child(2) > ' \
                   'div:nth-child(1) > div:nth-child(2) > ' \
                   'table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(2) > td:nth-child(4)'

    D2v3_storage = '#GeneralPurpose-table-content > div:nth-child(17) > div:nth-child(2) > ' \
                   'div:nth-child(1) > div:nth-child(2) > table:nth-child(1) > ' \
                   'tbody:nth-child(2) > tr:nth-child(1) > td:nth-child(4)'

    def __init__(self):
        super().__init__(self.PageUrl)

    def scrap_all(self):
        self.put_from_element('A2v2', self.A2v2, self._parse_regions, self.NumericPricePattern)
        self.put_from_element('D2v3_demand', self.D2v3_on_demand, self._parse_regions, self.NumericPricePattern)
        self.put_from_element('D2v3_one_year', self.D2v3_one_year, self._parse_regions, self.NumericPricePattern)
        self.put_from_element('D2v3_three_years', self.D2v3_three_years, self._parse_regions, self.NumericPricePattern)
        self.put_text_from_page('A2v2_incl_storage', self.A2v2_storage, self.StoragePattern)
        self.put_text_from_page('D2v3_incl_storage', self.D2v3_storage, self.StoragePattern)

    def _parse_regions(self, key: str, elem: WebElement) -> str:
        data = elem.get_attribute('data-amount')
        regional_prices_json = json.loads(data)
        price = json_attr(regional_prices_json, 'regional -> europe-north')
        if price is None:
            raise ScrappingException(message=f'Fail to get data from regional attributes for <{key}>')
        return str(price)
