from core.page_scrapper_base import PageScrapperBase


class SelectelStorageScrapper(PageScrapperBase):

    Url = 'https://selectel.ru/services/cloud/storage/'

    def __init__(self):
        super().__init__(self.Url)

    def scrap_all(self):
        pass
