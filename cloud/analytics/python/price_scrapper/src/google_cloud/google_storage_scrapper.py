from core.page_scrapper_base import PageScrapperBase


class GoogleStorageScrapper(PageScrapperBase):
    STD_PER_GB = '#tabpanel-europe > div:nth-child(1) > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(5) > td:nth-child(2)'
    COLD_PER_GB = '#tabpanel-europe > div:nth-child(1) > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(5) > td:nth-child(4)'
    STD_READ = 'div.devsite-table-wrapper:nth-child(39) > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(1) > td:nth-child(3)'
    COLD_READ = 'div.devsite-table-wrapper:nth-child(39) > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(3) > td:nth-child(3)'
    STD_WRITE = 'div.devsite-table-wrapper:nth-child(39) > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(1) > td:nth-child(2)'
    COLD_WRITE = 'div.devsite-table-wrapper:nth-child(39) > table:nth-child(1) > tbody:nth-child(2) > tr:nth-child(3) > td:nth-child(2)'

    def __init__(self, url: str):
        super().__init__(url)

    def scrap_all(self):
        # std storage
        self.put_text_from_page('STD_PER_GB', self.STD_PER_GB)
        self.put_text_from_page('STD_READ_PER_10000', self.STD_READ)
        self.put_text_from_page('STD_WRITE_PER_10000', self.STD_WRITE)

        # cold storage
        self.put_text_from_page('COLD_PER_GB', self.COLD_PER_GB)
        self.put_text_from_page('COLD_READ_PER_10000', self.COLD_READ)
        self.put_text_from_page('COLD_WRITE_PER_10000', self.COLD_WRITE)
