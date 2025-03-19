import json

from selenium.webdriver.remote.webelement import WebElement

from core.exceptions import ScrappingException
from core.utils import json_attr
from core.page_scrapper_base import PageScrapperBase


class AzureStorageScrapperNew(PageScrapperBase):
    PageUrl = 'https://azure.microsoft.com/en-us/pricing/details/storage/blobs/'

    # per gb selectors
    Std_Per_Gb = '#recommended > div:nth-child(2) > div:nth-child(3) > div:nth-child(7) > ' \
                 'div:nth-child(1) > div:nth-child(2) > table:nth-child(1) > ' \
                 'tbody:nth-child(2) > tr:nth-child(1) > ' \
                 'td:nth-child(3) > span:nth-child(1)'

    Cold_Per_Gb = '#recommended > div:nth-child(2) > div:nth-child(3) > div:nth-child(7) > ' \
                  'div:nth-child(1) > div:nth-child(2) > table:nth-child(1) > ' \
                  'tbody:nth-child(2) > tr:nth-child(1) > ' \
                  'td:nth-child(4) > span:nth-child(1)'

    # std storage
    Std_Read_Per_10000 = '#recommended > div:nth-child(4) > div:nth-child(3) > div:nth-child(2) > ' \
                         'div:nth-child(7) > div:nth-child(1) > div:nth-child(2) > ' \
                         'table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(2) > ' \
                         'td:nth-child(3) > span:nth-child(1)'

    Std_Write_Per_10000 = '#recommended > div:nth-child(4) > div:nth-child(3) > div:nth-child(2) > ' \
                          'div:nth-child(7) > div:nth-child(1) > div:nth-child(2) > ' \
                          'table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(1) > ' \
                          'td:nth-child(3) > span:nth-child(1)'

    # cold storage
    Cold_Read_Per_10000 = '#recommended > div:nth-child(4) > div:nth-child(3) > div:nth-child(2) > div:nth-child(7) > div:nth-child(1) > div:nth-child(2) > ' \
                          'table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(2) > td:nth-child(4) > span:nth-child(1)'

    Cold_Write_Per_10000 = '#recommended > div:nth-child(4) > div:nth-child(3) > div:nth-child(2) > div:nth-child(7) > div:nth-child(1) > ' \
                           'div:nth-child(2) > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(1) > td:nth-child(4) > span:nth-child(1)'

    def __init__(self):
        super().__init__(self.PageUrl)

    def scrap_all(self):
        self.put_from_element('STD_PER_GB', self.Std_Per_Gb, self._parse_regions)
        self.put_from_element('STD_READ_PER_10000', self.Std_Read_Per_10000, self._parse_regions)
        self.put_from_element('STD_WRITE_PER_10000', self.Std_Write_Per_10000, self._parse_regions)
        self.put_from_element('COLD_PER_GB', self.Cold_Per_Gb, self._parse_regions)
        self.put_from_element('COLD_READ_PER_10000', self.Cold_Read_Per_10000, self._parse_regions)
        self.put_from_element('COLD_WRITE_PER_10000', self.Cold_Write_Per_10000, self._parse_regions)

    def _parse_regions(self, key: str, elem: WebElement) -> str:
        data = elem.get_attribute('data-amount')
        regional_prices_json = json.loads(data)
        price = json_attr(regional_prices_json, 'regional -> europe-west')
        if price is None:
            raise ScrappingException(message=f'Fail to get data from regional attributes for <{key}>')
        return str(price)
