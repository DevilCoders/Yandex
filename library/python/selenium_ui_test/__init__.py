# coding: utf-8

import base64
import logging
import time
import os

from selenium import webdriver as wd
from selenium.common.exceptions import WebDriverException
from selenium.webdriver.common.desired_capabilities import DesiredCapabilities


class Drivers(object):
    DEFAULT_GRID_URL = "http://selenium:selenium@sg.yandex-team.ru:4444/wd/hub"

    chrome_capabilities = DesiredCapabilities.CHROME.copy()
    chrome_capabilities["version"] = "99.0"
    # https://nda.ya.ru/3VntX4
    chrome_capabilities["goog:loggingPrefs"] = {"browser": "ALL"}

    yandex_browser_capabilities = DesiredCapabilities.CHROME.copy()
    yandex_browser_capabilities["version"] = "22.3.0.127"

    # firefox = DesiredCapabilities.FIREFOX.copy()
    # firefox["version"] = "69.0"

    capabilities = [
        chrome_capabilities,
        # yandex_browser_capabilities,
        # firefox,
    ]

    CHROME_OPTIONS = wd.ChromeOptions()
    CHROME_OPTIONS.add_argument("--ignore-ssl-errors=yes")
    CHROME_OPTIONS.add_argument("--ignore-certificate-errors")
    CHROME_OPTIONS.capabilities["version"] = "99.0"

    YANDEX_OPTIONS = wd.ChromeOptions()
    YANDEX_OPTIONS.add_argument("--ignore-ssl-errors=yes")
    YANDEX_OPTIONS.add_argument("--ignore-certificate-errors")
    YANDEX_OPTIONS.capabilities["version"] = "22.3.0.127"

    OPTION_LIST = [
        CHROME_OPTIONS,
        # YANDEX_OPTIONS,
    ]

    def __init__(self, url=None):
        logging.info("Initializing Selenium drivers...")

        self.grid_url = url or self.DEFAULT_GRID_URL

        self.drivers = [
            wd.Remote(
                wd.remote.remote_connection.RemoteConnection(
                    self.grid_url,
                    resolve_ip=False,
                ),
                options=driver_options,
            ) for driver_options in self.OPTION_LIST
        ]

        logging.info("Initializing %s Selenium drivers completed.", len(self.drivers))


class Navigator(object):
    def __init__(self, main_url, driver, results_folder=None, timeout=60):
        self.main_url = main_url
        self.timeout = timeout
        self.driver = driver
        self.results_folder = os.path.abspath(results_folder or ".")
        if not os.path.exists(self.results_folder):
            os.mkdir(self.results_folder)

    @staticmethod
    def _navigation_log(url, wait_for_text):
        if wait_for_text:
            logging.info("Navigating to `%s` with text checking `%s`...", url, wait_for_text)
        else:
            logging.warning("Navigating to `%s` without wait...", url)

    def check_text(self, url_path, text):
        # type: (str, str) -> int
        # TODO: rename. In some cases we need to check multiple phrases/items/etc within
        # single navigation.
        """
        Load page with the given URL and search for the given text

        :param url_path: URL to load
        :param text: text to search for
        :return: waster time (seconds)
        """
        url = self.main_url + url_path
        self._navigation_log(url, text)
        self.driver.get(url)

        if text:
            wasted_time = 0
            sleep_time = 0.1
            while wasted_time < self.timeout:
                if text in self.driver.page_source:
                    return wasted_time  # OK, we found what we need

                logging.info("Text `%s` was not detected after %.3f seconds", text, wasted_time)
                sleep_time *= 1.2
                time.sleep(sleep_time)
                wasted_time += sleep_time

            file_id_str = "{}_{}_{}".format(url_path, text, int(time.time()))
            file_id = base64.urlsafe_b64encode(file_id_str.encode("utf-8")).decode("utf-8")
            self.driver.get_screenshot_as_file(os.path.join(self.results_folder, "{}_screenshot.png".format(file_id)))
            self.save_info("{}_page_source.html".format(file_id), self.driver.page_source)
            self.save_info("{}_console_log.txt".format(file_id), "\n".join(map(str, self.driver.get_log("browser"))))

            raise WebDriverException("Text was not found: {}".format(text), screen=True)

    def save_info(self, file_name, info):
        with open(os.path.join(self.results_folder, file_name), "w") as fo:
            fo.write(info)
