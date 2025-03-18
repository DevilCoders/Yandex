# -*- coding: utf8 -*-
import os
import re

from selenium import webdriver
from selenium.webdriver.remote.remote_connection import RemoteConnection

from antiadblock.libs.adb_selenium_lib.config import AdblockTypes

from .base_browser import BaseBrowser


class Chrome(BaseBrowser):  # chrome based (Chrome, yandex-browser)
    def _init_remote_options(self, browser_info, chrome_options):
        capabilities = chrome_options.to_capabilities()
        capabilities.update(
            {
                'env': [
                    "LANG=ru_RU.UTF-8",
                    "LANGUAGE=ru:en",
                    "LC_ALL=ru_RU.UTF-8",
                ],  # установка русской локали, для листов
                'version': browser_info.version,
                'browserName': 'chrome',
                'loggingPrefs': {'browser': 'ALL'},
            }
        )

        if os.getenv("VIDEO") == "ENABLE":
            capabilities.update(
                {
                    "sessionTimeout": "5m",
                    "enableVNC": True,
                    "enableVideo": True,
                    "enableLog": True,
                }
            )

        return chrome_options, capabilities

    def init_options(self, browser_info, internal_extensions: dict):
        chrome_options = webdriver.ChromeOptions()
        if self.adblock.type == AdblockTypes.INCOGNITO:
            chrome_options.add_argument('--incognito')
        elif self.adblock.type not in (AdblockTypes.WITHOUT_ADBLOCK, AdblockTypes.WITHOUT_ADBLOCK_CRYPTED):
            self._download_extension(chrome_options)

        if internal_extensions:
            if internal_extensions.get('modheaders') is not None:
                chrome_options.add_extension(internal_extensions['modheaders']['CHROME_MODHEADER'])
            if internal_extensions.get('modcookies') is not None:
                chrome_options.add_extension(internal_extensions['modcookies']['CHROME_MODCOOKIE'])

        if os.getenv("ARGUS_ENV") == "DEBUG":
            return chrome_options, None
        return self._init_remote_options(browser_info, chrome_options)

    def _start_remote_selenium(self, selenium_executor, capabilities):
        rc = RemoteConnection(selenium_executor)
        rc.set_timeout(180)  # seconds
        self.driver = webdriver.Remote(command_executor=rc, desired_capabilities=capabilities)

    def _start_local_selenium(self, options):
        self.driver = webdriver.Chrome(chrome_options=options)

    def start_selenium(self, selenium_executor, options, capabilities):
        if os.getenv("ARGUS_ENV") == "DEBUG":
            self._start_local_selenium(options)
        else:
            self._start_remote_selenium(selenium_executor, capabilities)

    def get_extension_version(self, filename):
        return re.match(r'[a-zA-Z_-]+(?P<version>[\d_.]+)\.\w{3}', filename).group('version').replace('_', '.')
