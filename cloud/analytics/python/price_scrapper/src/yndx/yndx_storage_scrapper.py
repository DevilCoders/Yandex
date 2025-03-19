from core.page_scrapper_base import PageScrapperBase


class YndxStorageScrapper(PageScrapperBase):
    PRICE_TEMPLATE = r'\$([0-9])+(\.[0-9]+$)*$'

    STD_PER_GB = '.dc-doc-page__body > table:nth-child(17) > tbody:nth-child(2) > tr:nth-child(2) > td:nth-child(2)'
    COLD_PER_GB = '.dc-doc-page__body > table:nth-child(17) > tbody:nth-child(2) > tr:nth-child(3) > td:nth-child(2)'
    STD_WRITE_PER_1000 = '.dc-doc-page__body > table:nth-child(22) > tbody:nth-child(2) > tr:nth-child(6) > td:nth-child(2)'
    STD_READ_PER_10000 = '.dc-doc-page__body > table:nth-child(22) > tbody:nth-child(2) > tr:nth-child(8) > td:nth-child(2)'
    COLD_WRITE_PER_1000 = '.dc-doc-page__body > table:nth-child(22) > tbody:nth-child(2) > tr:nth-child(12) > td:nth-child(2)'
    COLD_READ_PER_10000 = '.dc-doc-page__body > table:nth-child(22) > tbody:nth-child(2) > tr:nth-child(13) > td:nth-child(2)'

    def __init__(self, url: str):
        super().__init__(url)

    def scrap_all(self):
        # for std storage
        self.put_text_from_page('STD_PER_GB', self.STD_PER_GB, self.PRICE_TEMPLATE)
        self.put_text_from_page('STD_READ_PER_10000', self.STD_READ_PER_10000, self.PRICE_TEMPLATE)
        self.put_text_from_page('STD_WRITE_PER_1000', self.STD_WRITE_PER_1000, self.PRICE_TEMPLATE)

        # for cold storage
        self.put_text_from_page('COLD_PER_GB', self.COLD_PER_GB, self.PRICE_TEMPLATE)
        self.put_text_from_page('COLD_READ_PER_10000', self.COLD_READ_PER_10000, self.PRICE_TEMPLATE)
        self.put_text_from_page('COLD_WRITE_PER_1000', self.COLD_WRITE_PER_1000, self.PRICE_TEMPLATE)
