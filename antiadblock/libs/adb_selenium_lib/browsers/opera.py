# -*- coding: utf8 -*-
import os

from selenium import webdriver
from selenium.webdriver.remote.remote_connection import RemoteConnection

from antiadblock.libs.adb_selenium_lib.config import AdblockTypes

from .base_browser import BaseBrowser


class Opera(BaseBrowser):
    def _init_remote_options(self, browser_info, opera_options):
        capabilities = opera_options.to_capabilities()
        capabilities.update(
            {
                'env': [
                    "LANG=ru_RU.UTF-8",
                    "LANGUAGE=ru:en",
                    "LC_ALL=ru_RU.UTF-8",
                ],  # установка русской локали, для листов
                'version': browser_info.version,
                'browserName': 'opera',
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

        return opera_options, capabilities

    def init_options(self, browser_info, internal_extensions):
        opera_options = webdriver.ChromeOptions()
        if self.adblock.type == AdblockTypes.INCOGNITO:
            opera_options.add_argument('--incognito')

        if internal_extensions:
            if internal_extensions.get('modheaders') is not None:
                opera_options.add_extension(internal_extensions['modheaders']['CHROME_MODHEADER'])
            if internal_extensions.get('modcookies') is not None:
                opera_options.add_extension(internal_extensions['modcookies']['CHROME_MODCOOKIE'])

        return self._init_remote_options(browser_info, opera_options)

    def start_selenium(self, selenium_executor, options, capabilities):
        rc = RemoteConnection(selenium_executor)
        rc.set_timeout(180)  # seconds
        self.driver = webdriver.Remote(command_executor=rc, desired_capabilities=capabilities)

    def get_extension_version(self, filename):
        return "no_version"
