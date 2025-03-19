from core.page_scrapper_base import PageScrapperBase


class SelectedComputeScrapper(PageScrapperBase):
    Url = 'https://selectel.ru/services/cloud/servers/prices/?region=ru-2&zone=ru-2a'

    def __init__(self):
        super().__init__(self.Url)

    def scrap_all(self):
        pass
