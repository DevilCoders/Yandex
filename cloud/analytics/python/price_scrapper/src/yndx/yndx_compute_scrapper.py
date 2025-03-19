from core.page_scrapper_base import PageScrapperBase


class YndxComputeScrapper(PageScrapperBase):
    PRICE_TEMPLATE = r'\$([0-9])+(\.[0-9]+$)*$'

    CPU_PRICE_SELECTOR = ".dc-doc-page__body > table:nth-child(47) > tbody:nth-child(2) > " \
                         "tr:nth-child(9) > td:nth-child(2)"

    RAM_PRICE_SELECTOR = ".dc-doc-page__body > table:nth-child(48) > " \
                         "tbody:nth-child(2) > tr:nth-child(7) > td:nth-child(2)"

    SSD_PRICE_SELECTOR = ".dc-doc-page__body > table:nth-child(65) > " \
                         "tbody:nth-child(2) > tr:nth-child(1) > td:nth-child(2)"

    WINDOWS_PRICE_SELECTOR = ".dc-doc-page__body > table:nth-child(53) > tbody:nth-child(2) > tr:nth-child(4) > td:nth-child(2)"

    def __init__(self, url: str):
        super().__init__(url)

    def scrap_all(self):
        self.put_text_from_page(key="CPU_HOURLY_PRICE",
                                css_selector=YndxComputeScrapper.CPU_PRICE_SELECTOR,
                                expected_regex=YndxComputeScrapper.PRICE_TEMPLATE)
        self.put_text_from_page(key="RAM_HOURLY_PRICE",
                                css_selector=YndxComputeScrapper.RAM_PRICE_SELECTOR,
                                expected_regex=YndxComputeScrapper.PRICE_TEMPLATE)
        self.put_text_from_page(key="SSD_PRICE_PER_GB",
                                css_selector=YndxComputeScrapper.SSD_PRICE_SELECTOR,
                                expected_regex=YndxComputeScrapper.PRICE_TEMPLATE)
        self.put_text_from_page(key="WINDOWS_PER_CORE_PRICE",
                                css_selector=YndxComputeScrapper.WINDOWS_PRICE_SELECTOR,
                                expected_regex=YndxComputeScrapper.PRICE_TEMPLATE)
