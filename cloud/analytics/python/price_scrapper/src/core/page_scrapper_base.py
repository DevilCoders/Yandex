from abc import ABCMeta, abstractmethod
from typing import Any, Optional, Callable
import re

from selenium.common.exceptions import NoSuchElementException
from selenium.webdriver.firefox.webdriver import WebDriver
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.remote.webelement import WebElement

from core.exceptions import ScrappingException
from core.scrapped_data import ScrappedData


class PageScrapperBase(metaclass=ABCMeta):

    def __init__(self, url: str):
        self._scrapped_data = {}
        self.url = url
        self._driver: Optional[WebDriver] = None

    @abstractmethod
    def scrap_all(self):
        pass

    def put_text_from_page(self, key: str, css_selector: str, expected_regex: str = None):
        if key in self._scrapped_data.keys():
            raise Exception(f"Data with key = {key} already exists")
        text = self._get_text(ScrappedData(key, css_selector))
        self._assert_format(key, expected_regex, text)
        if text is None or text == '':
            raise ScrappingException(message=f'Element for key <{key}> not exists on page <{self.url}>')
        self._scrapped_data[key] = text

    def put_from_element(self,
                         key: str,
                         css_selector: str,
                         parse_elem_func: Callable[[str, WebElement], str],
                         expected_regex: str = None):
        if parse_elem_func is None:
            raise ValueError('Process function that transforms web element to text is required')
        if not isinstance(parse_elem_func, Callable):
            raise TypeError('Callable is required')
        elem = self._get_element(css_selector)
        data = parse_elem_func(key, elem)
        self._assert_format(key, expected_regex, data)
        self._scrapped_data[key] = data

    def put_constant(self, key: str, value: str):
        self._scrapped_data[key] = value

    def _assert_format(self, key: str, exp_reg: str, text_from_page: str):
        if exp_reg is None:
            return
        if not re.match(exp_reg, text_from_page.strip()):
            raise ScrappingException(message=f'Data for key <{key}> is expected to match <{exp_reg}>, '
                                             f'but having: <{text_from_page}>')

    def __getitem__(self, key):
        if key not in self._scrapped_data.keys():
            raise Exception("Unknown data key = {key}")
        text_data = self._scrapped_data[key]
        return self._parse_text(text_data)

    def __enter__(self):
        options = Options()
        options.headless = True
        self._driver = WebDriver(options=options)
        self._driver.get(self.url)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._driver.close()

    def scrap_data(self):
        try:
            self._try_scrap(self._driver)
        except Exception as ex:
            print(ex)
        finally:
            self._driver.close()

    @property
    def has_scrapped_data(self) -> bool:
        return len(self._scrapped_data) > 0

    def _try_scrap(self, driver: WebDriver):
        pass

    def _get_text(self, d: ScrappedData) -> Optional[str]:
        try:
            elem = self._driver.find_element_by_css_selector(d.css_selector)
            return elem.text
        except NoSuchElementException:
            return None
        except Exception as ex:
            raise ScrappingException(d, ex)

    def _get_element(self, css_selector: str) -> WebElement:
        return self._driver.find_element_by_css_selector(css_selector)

    def _parse_text(self, text: Any) -> float:
        if type(text) is float or type(text) is int:
            return text
        return float(text.strip()
                     .replace('$', '')
                     .replace('GiB', '')
                     .replace('/ vCPU month', '')
                     .replace('/ vCPU hour', '')
                     .replace('/ GB month', '')
                     .replace('/ GB hour', '')
                     .replace(' USD per core/hour', ' ')
                     .replace(' per GB', '')
                     .replace(' ', ''))
