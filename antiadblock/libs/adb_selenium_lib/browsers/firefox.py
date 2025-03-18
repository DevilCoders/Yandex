# -*- coding: utf8 -*-
import json
import os
import re
import sys

from selenium import webdriver
from selenium.webdriver.firefox.firefox_profile import AddonFormatError

from antiadblock.libs.adb_selenium_lib.config import AdblockTypes

from .base_browser import BaseBrowser


class Firefox(BaseBrowser):
    def init_options(self, browser_info, internal_extensions):
        ff_profile = FirefoxProfileWithWebExtensionSupport()
        if self.adblock.type == AdblockTypes.INCOGNITO:
            ff_profile.set_preference("browser.privatebrowsing.autostart", True)
        elif self.adblock.type not in (AdblockTypes.WITHOUT_ADBLOCK, AdblockTypes.WITHOUT_ADBLOCK_CRYPTED):
            self._download_extension(ff_profile)

        if internal_extensions:
            if internal_extensions.get('modheaders') is not None:
                ff_profile.add_extension(internal_extensions['modheaders']['FIREFOX_MODHEADER'])
            if internal_extensions.get('modcookies') is not None:
                ff_profile.add_extension(internal_extensions['modcookies']['FIREFOX_MODCOOKIE'])

        # попытка установки русских локалей позволяет в большинстве адблоков автоматически подтягивать русские листы,
        # к сожалению нету в ФФ русского языкового пакета и adblock/adblock+ не ставят ruadlist, это делается вручную
        ff_profile.set_preference('intl.accept_languages', 'ru-ru,ru')
        ff_profile.set_preference('intl.locale.matchOS', False)
        ff_profile.set_preference('intl.locale.requested', 'ru')
        ff_profile.set_preference('xpinstall.signatures.required', False)
        ff_profile.set_preference('extensions.langpacks.signatures.required', False)

        capabilities = webdriver.DesiredCapabilities.FIREFOX
        capabilities.update(
            {
                'version': browser_info.version,
                'browserName': browser_info.name,
                'loggingPrefs': {'browser': 'ALL'},
                'marionette': False,
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

        return ff_profile, capabilities

    def start_selenium(self, selenium_executor, options, capabilities):
        if selenium_executor:
            self.driver = webdriver.Remote(
                command_executor=selenium_executor, desired_capabilities=capabilities, browser_profile=options
            )
        else:
            self.driver = webdriver.Firefox(firefox_profile=options)

    def get_extension_version(self, filename):
        return (
            re.match(r'[a-zA-Z_-]+(?P<version>[\d_.]+)(?:-(?:an\+)?fx)?\.\w{3}', filename)
            .group('version')
            .replace('_', '.')
        )

    # TODO багу больше года, пока без логов в FF https://github.com/w3c/webdriver/issues/406
    def get_console_log(self):
        return None


# Добавляем класс изменяющий процедуру поиска метаданных в расширениях
# read https://intoli.com/blog/firefox-extensions-with-selenium/
class FirefoxProfileWithWebExtensionSupport(webdriver.FirefoxProfile):
    def _addon_details(self, addon_path):
        try:
            return super(FirefoxProfileWithWebExtensionSupport, self)._addon_details(addon_path)
        except AddonFormatError:
            try:
                with open(os.path.join(addon_path, 'manifest.json'), 'r') as f:
                    manifest = json.load(f)
                    return {
                        'id': manifest.get('applications', manifest.get('browser_specific_settings', {}))
                        .get('gecko', {})
                        .get('id', None),
                        'version': manifest['version'],
                        'name': manifest['name'],
                        'unpack': False,
                    }
            except (IOError, KeyError) as e:
                raise AddonFormatError(str(e), sys.exc_info()[2])
