from enum import Enum
import logging
import typing as t

from pydantic import BaseModel, Field

from antiadblock.libs.adb_selenium_lib.browsers.base_browser import BaseBrowser
from antiadblock.libs.adb_selenium_lib.browsers.opera import Opera
from antiadblock.libs.adb_selenium_lib.browsers.chrome import Chrome
from antiadblock.libs.adb_selenium_lib.browsers.firefox import Firefox
from antiadblock.libs.adb_selenium_lib.schemas import BrowserInfo, Cookie
from antiadblock.libs.adb_selenium_lib.browsers.extensions import ExtensionAdb

SiteUrl = str


class BrowserType(Enum):
    Chrome = Chrome
    Opera = Opera
    Firefox = Firefox


class Browser(BaseModel):
    browser_type: BrowserType = Field(..., title='Тип селениум браузера')
    browser_info: BrowserInfo = Field(..., title='Информация о браузере')
    adb_extension: t.Optional[type[ExtensionAdb]] = Field(None, title='Тип расширения')
    util_extensions: dict = Field(..., title='Вспомогательные расширения (например, для кук/заголовков)')
    additional_filters: list[str] = Field([], title='Дополнительные адблок фильтры браузера')
    selenoid_url: str = Field(..., title='URL Selenoid')

    class Config:
        use_enum_values = True
        arbitrary_types_allowed=True

    def create_browser(self) -> BaseBrowser:
        logger = logging.getLogger(f'{self.browser_info.name} {self.adb_extension.type.adb_name}'.capitalize())
        browser = self.browser_type(
            browser_info=self.browser_info,
            adblock_class=self.adb_extension,
            selenium_executor=self.selenoid_url,
            internal_extensions=self.util_extensions,
            filters_list=self.additional_filters,
            logger=logger
        )
        return browser

    def __hash__(self) -> int:
        return hash(self._get_main_members())

    def __eq__(self, other: t.Any) -> bool:
        if isinstance(other, Browser):
            return self._get_main_members() == other._get_main_members()
        elif isinstance(other, tuple):
            return self._get_main_members() == other
        raise ValueError(f'Equality operator is not defined for Browser and {type(other)}')

    def _get_main_members(self) -> tuple:
        return self.browser_info.name, self.browser_info.version, self.adb_extension.type


class UrlSettings(BaseModel):
    url: SiteUrl = Field(title='URL страницы')
    selectors: list[str] = Field(title='CSS селекторы для скрытия элементов на странице')
    wait_sec: int = Field(3, title='Время ожидания прогрузки страницы')
    scroll_count: int = Field(0, title='Максимальное количество прокручиваний страницы')

    cookies: list[Cookie] = Field([], title='Куки на страницу')
    headers: dict[str, str] = Field({}, title='Дополнительные заголовки на страницу')
    invert_detect: bool = Field(False, title='Страница использует инверсивную схему (если нет куки -> крайпрокс)')

    cookie_per_page: t.Optional[Cookie] = Field(title='Дополнительная кука на страницу')
    is_reference: t.Optional[bool] = Field(title='Является ли заход на страницу референсным')
    count_of_retries: int = Field(2, title='Количество ретраев без перезапуска')


class Profile(BaseModel):
    run_id: int = Field(title='ID запуска аргуса')
    cookies: list[Cookie] = Field(
        [], title='Дополнительные куки на каждую страницу (в формате "кука=значение;кука2=значение"'
    )
    proxy_cookies: list[Cookie] = Field([], title='Куки для cryprox')
    headers: dict[str, str] = Field({}, title='Дополнительные заголовки на каждую страницу')
    filters_list: list[str] = Field([], title='Дополнительные фильтры для блокировщика')
    url_settings: list[UrlSettings] = Field([], title='Список страниц для проверки')

    count_of_retries: int = Field(3, title='Количество ретраев на проверку страницы')
    invert_detect: bool = Field(False, title='Сервис использует инверсивную схему (если нет куки -> крайпрокс)')


class Result(BaseModel):
    # browser=browser.name, browser_version=browser.version, adblocker=browser.adblock.name)
    browser: str = Field(title='Название браузера')
    browser_version: str = Field(title='Версия браузера')
    adblocker: str = Field(title='Название блокировщика')
    adblocker_url: t.Optional[str] = Field(title='URL использованного блокировщика')
    url: SiteUrl = Field(title='URL проверяемой страницы')

    ludca: t.Optional[str] = Field(title='Ludca')
    adb_bits: t.Optional[str] = Field(title='ADB bits')
    img_url: str = Field(title='Ссылка на скриншот страницы')

    rules: t.Optional[dict] = Field(title='Сработавшие правила блокировщика')
    headers: dict[str, str] = Field(title='Использованные дополнительные заголовки')
    logs: dict = Field(title='Логи запуска')
    cookies: dict[str, str] = Field(title='Использованные куки')
    console_logs: t.Optional[list] = Field(title='Консольные логи браузера')

    start: str = Field(title='Время запуска проверки')
    end: str = Field(title='Время окончания проверки')

    has_problem: str = Field('', title='Вид возникшей во время проверки проблемы')
    reference_case_id: t.Optional[str] = Field(None, title='ID референса (X-AAB-RequestId')
