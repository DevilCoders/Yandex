# -*- coding: utf8 -*-
from itertools import chain
import logging
import os
import re
import sys
import time
import traceback
import typing as t
from abc import ABC, abstractmethod
from base64 import b64encode
from functools import wraps
from urllib.parse import urlsplit

import requests
from selenium.common import exceptions as selenium_exceptions
from selenium.webdriver.common.by import By
from selenium.webdriver.remote.webdriver import WebDriver
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.events import AbstractEventListener, EventFiringWebDriver
from selenium.webdriver.support.ui import WebDriverWait

from antiadblock.libs.adb_selenium_lib.browsers.extensions import ExtensionAdb
from antiadblock.libs.adb_selenium_lib.config import VIRTUAL_MONITOR_HEIGHT, VIRTUAL_MONITOR_WIDTH
from antiadblock.libs.adb_selenium_lib.schemas import BrowserInfo, Cookie
from antiadblock.libs.adb_selenium_lib.utils import screenshot  # для снятия скриншотов
from antiadblock.libs.adb_selenium_lib.utils.exceptions import (
    DownloadExtensionException,
    ExtensionModifyHeadersException,
    ExtensionModifyCookiesException,
)


class CookieListener(AbstractEventListener):
    def __init__(self, logger, cookies_to_add: list[Cookie] = None) -> None:
        if cookies_to_add is None:
            cookies_to_add = []
        self.cookies_to_add: list[Cookie] = cookies_to_add
        self.logger = logger
        super().__init__()

    def get_host(self, url: str) -> str:
        """
        >>> get_host('http://lena-miro.ru')
        'lena-miro.ru'
        >>> get_host('http://sub1.sub2.yandex.ru')
        'sub1.sub2.yandex.ru'
        >>> get_host('http://www.yandex.ru/asdfasd?arg=1&arg2=2')
        'www.yandex.ru'
        """
        host = re.match(r'^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?', url).group(4)
        return host

    def before_close(self, driver: WebDriver) -> None:
        for cookie in self.cookies_to_add:
            cookie.domain = self.get_host(driver.current_url)
            self.logger.info(f"Add cookie {cookie.dict()}")
            driver.add_cookie(cookie.dict(exclude_none=True))

    def on_exception(self, exception: Exception, driver: WebDriver):
        self.logger.info(f"EVENT LISTENER EXCEPTION {exception}")


class BaseBrowser(ABC):
    class Decorators:
        @staticmethod
        def modifier_extension(exception_type: t.Type[Exception], url_to_leave: str):
            def inner(func):
                @wraps(func)
                def wrapper(self, *args, **kwargs):
                    func(self, *args, **kwargs)
                    current_url = self.driver.current_url.split('?')[0]
                    if current_url == url_to_leave:
                        raise exception_type(f"Not redirect to ya.ru. Current url: {current_url}")
                    self.driver.close()
                    self.driver.switch_to.window(self.driver.window_handles[0])
                return wrapper
            return inner

        @staticmethod
        def retry_selenium_timeout(func):
            """
            Попытка борьбы с таймаутами селениума. Пока хватает перезапроса
            """

            @wraps(func)
            def wrapper(self, *args, **kwargs):
                exc = None
                on_fail = kwargs.pop('_on_fail', None)
                for _ in range(5):
                    try:
                        func(self, *args, **kwargs)
                        return
                    except selenium_exceptions.TimeoutException:
                        self.logger.info('retry selenium timeout')
                        if on_fail is not None:
                            on_fail()
                        exc = sys.exc_info()
                raise exc

            return wrapper

    def __init__(
        self,
        browser_info,
        adblock_class: type[ExtensionAdb],
        selenium_executor,
        internal_extensions=None,
        filters_list: list[str] = None,
        logger: t.Optional[logging.Logger] = None,
    ):
        if logger is None:
            logging.basicConfig(format='%(levelname)-8s [%(asctime)s] %(threadName)-30s: %(message)s', level=logging.INFO)
            logger = logging

        self.logger = logger
        self.adblock: ExtensionAdb = adblock_class(internal_extensions["adblocks"])
        self.browser_info: BrowserInfo = browser_info
        self.driver: t.Optional[WebDriver] = None
        self.on_close_cookies: list[Cookie] = []

        browser_options, capabilities = self.init_options(self.browser_info, internal_extensions)
        self.start_selenium(selenium_executor, browser_options, capabilities)
        self.logger.info(f'Session ID: {self.driver.session_id}')
        self.configure_driver()
        self.adblock.setup(self, filters_list)

    def __enter__(self):
        return self

    @property
    def name(self):
        return self.browser_info.name

    @property
    def version(self):
        return self.browser_info.version

    @abstractmethod
    def init_options(self, browser_info, internal_extensions):
        pass

    @abstractmethod
    def start_selenium(self, selenium_executor, options, capabilities):
        pass

    @abstractmethod
    def get_extension_version(self, filename):
        pass

    def get_element_text_by_element_id(self, element_id):
        return self.driver.find_element_by_id(element_id).text

    def get_triggered_rules(self) -> t.Optional[list[str]]:
        return self.driver.execute_script("return window.adbFiltersData;")

    def _install_extension(self, bro_options, filename):
        """debug function"""
        extension_filename = filename
        extension_filename = extension_filename.replace(
            "extension", f"{self.adblock.name}_{self.browser_info.name}"
        )  # for chrome ext
        self.logger.info('Successfully downloaded file: {extension_filename}')
        self.adblock.adblocker_url = extension_filename

        self.logger.info('Add extension to options')
        bro_options.add_extension(extension_filename)

    def _download_extension(self, bro_options) -> None:
        self.logger.info(f'Download extension {self.adblock.name}')
        self.logger.info(f'Download url: {self.adblock.get_download_url(self.name)}')
        response = requests.get(self.adblock.get_download_url(self.name))

        if response.status_code != 200:
            raise DownloadExtensionException(
                f'Extension {self.adblock.name} can not download, get code - {response.status_code}.'
            )

        extension_filename = urlsplit(response.url).path.rsplit('/', 1)[-1]
        extension_filename = extension_filename.replace(
            "extension", f"{self.adblock.name}_{self.browser_info.name}"
        )  # for chrome ext
        self.logger.info(f'Successfully downloaded file: {extension_filename}')
        self.adblock.adblocker_url = extension_filename

        if not os.path.exists(extension_filename):
            with open(extension_filename, 'wb') as f:
                for chunk in response:
                    f.write(chunk)

        self.logger.info('Add extension to options')
        if self.name == "yandex-browser" or self.name == "chrome":
            bro_options.add_encoded_extension(b64encode(bytes(response.content)).decode())
        else:
            bro_options.add_extension(extension_filename)

    def configure_driver(self):
        self.set_screen_resolution()
        self.driver.set_script_timeout(12)
        self.driver.set_page_load_timeout(60)
        # в случае обращения к любому элементу, если его не на странице будет ожидание максимум 10 сек, иначе дефолт 0 сек
        self.driver.implicitly_wait(10)

    def set_screen_resolution(self):
        self.driver.set_window_size(VIRTUAL_MONITOR_WIDTH, VIRTUAL_MONITOR_HEIGHT)

    def get_console_log(self):
        return self.driver.get_log('browser')

    def get_cookies(self) -> dict:
        return {c["name"]: c["value"] for c in self.driver.get_cookies()}

    # используем в замену обычному click() селениума
    def click_element_via_js(self, css_selector=None, element=None):
        if css_selector is None and element is None:
            raise Exception('Pass at least one parameter to find element')

        if css_selector is not None:
            self.driver.execute_script(f"document.querySelector('{css_selector}').click();")
        elif element is not None:
            self.driver.execute_script("arguments[0].click();", element)

    def add_cookies_on_close(self, cookie_list: list[Cookie]) -> None:
        self.on_close_cookies.extend(cookie_list)

    def execute_script(self, script: str) -> None:
        for _ in range(3):
            try:
                self.driver.execute_script(script)
            except selenium_exceptions.JavascriptException as e:
                self.logger.info(str(e).replace("\n", ""))
                time.sleep(1)
            else:
                break

    @Decorators.retry_selenium_timeout
    def switch_to(self, frame=None) -> None:
        if frame is None:
            self.driver.switch_to.default_content()
        else:
            self.driver.switch_to.window(frame)

    @Decorators.retry_selenium_timeout
    def open_url(self, url: str, clear_pages: bool = True) -> None:
        if clear_pages:
            self.__clear_pages()
        self.logger.info(f"Open url {url}")
        self.driver.get(url)

    @Decorators.retry_selenium_timeout
    def refresh_page(self) -> None:
        self.driver.refresh()

    def open_new_tab(self) -> None:
        self.execute_script("window.open('');")

    def __clear_pages(self) -> None:
        self.open_new_tab()
        last_opened_window = self.driver.window_handles[-1]
        current_driver = self.driver

        if self.on_close_cookies:
            current_driver = EventFiringWebDriver(self.driver, CookieListener(self.logger, self.on_close_cookies))
            self.on_close_cookies = []

        for opened_window in self.driver.window_handles:
            current_driver.switch_to.window(opened_window)
            if opened_window == last_opened_window:
                break
            current_driver.close()

    def clear_pages(self) -> None:
        self.open_new_tab()
        last_opened_window = self.driver.window_handles[-1]
        for opened_window in self.driver.window_handles:
            self.driver.switch_to.window(opened_window)
            if opened_window == last_opened_window:
                break
            self.driver.close()

    @Decorators.modifier_extension(ExtensionModifyHeadersException, 'https://ya.ru/clear')
    def clear_headers(self) -> None:
        self.open_new_tab()
        self.driver.switch_to.window(self.driver.window_handles[-1])
        self.open_url("https://ya.ru/clear", clear_pages=False)

    @Decorators.modifier_extension(ExtensionModifyHeadersException, 'https://ya.ru/add')
    def add_headers(self, headers: dict[str, str], clear: bool = False) -> None:
        if clear:
            self.clear_headers()
        url = "https://ya.ru/add?{}".format("&".join(f"{name}={value}" for name, value in headers.items()))
        self.logger.info(f"Add headers {headers}")
        self.open_new_tab()
        self.driver.switch_to.window(self.driver.window_handles[-1])
        self.open_url(url, clear_pages=False)

    @Decorators.modifier_extension(ExtensionModifyCookiesException, 'https://ya.ru/cookies')
    def set_cookies(
        self,
        add: t.Optional[list[Cookie]] = None,
        remove: t.Optional[list[Cookie]] = None,
        block: t.Optional[list[Cookie]] = None,
        unblock: t.Optional[list[Cookie]] = None,
        domain: t.Optional[str] = None,
    ) -> None:
        """
        Removes then adds specified cookies.

        If domain is specified forces all cookies domain to it.
        """
        assert add or remove or block or unblock, 'Add or remove or block or unblock cookies should be specified'

        add = add.copy() if add is not None else []
        remove = remove.copy() if remove is not None else []
        block = block.copy() if block is not None else []
        unblock = unblock.copy() if unblock is not None else []

        if domain is not None:
            for cookie in chain(add, remove, block, unblock):
                cookie.domain = domain

        cookies_query = '&'.join(
            query
            for query in [
                '&'.join(f'-::{cookie.domain}::{cookie.name}' for cookie in remove),
                '&'.join(f'+::{cookie.domain}::{cookie.name}' for cookie in add if cookie.value is None),
                '&'.join(
                    f'+::{cookie.domain}::{cookie.name}={cookie.value}' for cookie in add if cookie.value is not None
                ),
                '&'.join(f'^::{cookie.domain}::{cookie.name}' for cookie in block),
                '&'.join(f'!^::{cookie.domain}::{cookie.name}' for cookie in unblock),
            ]
            if query
        )
        url = f'https://ya.ru/cookies?{cookies_query}'
        for operation, cookie_list in (('Add', add), ('Remove', remove), ('Block', block), ('Unblock', unblock)):
            if cookie_list:
                self.logger.info(
                    f'{operation} cookies '
                    + ', '.join(f'{cookie.name}={cookie.value} for {cookie.domain}' for cookie in cookie_list)
                )
        self.open_new_tab()
        self.driver.switch_to.window(self.driver.window_handles[-1])
        self.open_url(url, clear_pages=False)

    def get_ludca(self):
        try:
            return self.driver.execute_script('return window.localStorage.ludca;')
        except Exception:
            self.logger.info("Failed to get Ludca.")
            return ""

    def wait_element_on_page(self, css_selector=None, xpath=None, timeout=10):
        if css_selector:
            WebDriverWait(self.driver, timeout).until(EC.presence_of_element_located((By.CSS_SELECTOR, css_selector)))
            return self.driver.find_element_by_css_selector(css_selector)
        elif xpath:
            WebDriverWait(self.driver, timeout).until(EC.presence_of_element_located((By.XPATH, xpath)))
            pass
        else:
            raise Exception("no selector and no xpath")

    def wait_element_available_to_click(self, css_selector, timeout=10):
        """
        Ждет появления к-л элемента по заданному селектору в зоне видимости и кликабельного (не должен быть скрыт другим элементом)
        Не работает со списком элементов, вернет первый найденный
        :param timeout: таймаут ожидания элемента на странице
        :param css_selector: селектор по которому ищем элемент
        """
        WebDriverWait(self.driver, timeout).until(EC.element_to_be_clickable((By.CSS_SELECTOR, css_selector)))

    def click_element(self, css_selector, use_js_click=False):
        element = self.wait_element_on_page(css_selector)
        self.scroll_to_view(element)

        if use_js_click:
            self.click_element_via_js(element=element)
        else:
            element.click()

    def scroll_to_view(self, element):
        self.driver.execute_script("arguments[0].scrollIntoView({block:\"center\"});", element)

    @Decorators.retry_selenium_timeout
    def set_display_none_by_css_selector(self, css_selector: str) -> None:
        try:
            self.driver.find_element_by_css_selector(css_selector)
            self.driver.execute_script("document.querySelector(arguments[0]).style.display=\"none\";", css_selector)
        except selenium_exceptions.TimeoutException:
            raise selenium_exceptions.TimeoutException
        except selenium_exceptions.NoSuchElementException:
            self.logger.info("Can not find element by css selector.")
        except Exception as e:
            self.logger.warning("Can not hide element by css selector. Error message: " + str(e))

    def get_page_full_screenshot(self, callback_functions=None, filename=None, wait_sec=None, scroll_count=0):
        """
        :param filename: имя файла в который сохранить скриншот, если не передать, сгенерит имя типа `домен_адблок.png`
        :param callback_functions: функции которые будут выполняться перед каждым скриншотом
        :param wait_sec: ожидание по времени между скриншотами
        :param scroll_count: сколько скроллов для скриншота на странице
        """
        return screenshot.fullpage_screenshot(
            self.driver,
            self.logger,
            filename,
            self.adblock.type.adb_name,
            callback_functions,
            wait_sec or 3,
            scroll_count,
        )

    def close(self, retry_count: int = 2) -> None:
        self.logger.info(f'Trying to end session {self.driver.session_id}')
        exc = None
        for _ in range(retry_count):
            try:
                self.driver.quit()
            except (selenium_exceptions.WebDriverException, selenium_exceptions.TimeoutException) as exception:
                self.logger.warning(f'Failed to close session {self.driver.session_id}')
                self.logger.warning(str(exception))
                # если сессии уже нету, нет смысла пытаться ее закончить несколько раз
                if exception.msg == 'Session timed out or not found':
                    return
                exc = sys.exc_info()
            else:
                self.logger.info(f'Session {self.driver.session_id} complete')
                return
        raise exc

    def __exit__(self, exc_type, exc_val, exc_tb):
        if exc_type is not None:
            self.logger.warning(traceback.format_exc())
        self.close()
        return exc_type is None
