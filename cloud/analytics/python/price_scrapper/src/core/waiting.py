import time

from selenium.webdriver.firefox.webdriver import WebDriver


class Waiting:

    def __init__(self, driver: WebDriver, selector: str):
        self._driver = driver
        self._selector = selector
        self._initial_value = None
        self.max_time_seconds = 15

    def save(self):
        self._initial_value = self._read_element()

    def wait_until_changes(self):
        if self._initial_value is None:
            raise Exception("Save value must be called first to remember html element value")
        for second in range(self.max_time_seconds):
            current_value = self._read_element()
            if current_value != self._initial_value:
                return
            time.sleep(1)

    def _read_element(self):
        elem = self._driver.find_element_by_css_selector(self._selector)
        if elem is None:
            raise ValueError(f"No element with selector: [{self._selector}]")
        if not (hasattr(elem, "text")):
            raise AttributeError("Invalid hmtl element caught by selector: "
                                 "element must have text attribute")
        return elem.text
