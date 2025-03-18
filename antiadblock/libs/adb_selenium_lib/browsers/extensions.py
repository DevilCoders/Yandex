# -*- coding: utf8 -*-
import typing as t
from abc import ABC, abstractmethod
from time import sleep

from antiadblock.libs.adb_selenium_lib.config import AdblockTypes
from antiadblock.libs.adb_selenium_lib.utils.exceptions import ExtensionAdblockException, ExtensionInfoException


class ExtensionAdb(ABC):
    def __init__(self, adb_type: AdblockTypes) -> None:
        self.type: AdblockTypes = adb_type
        self.adblocker_url: t.Optional[str] = None

    def __eq__(self, other: 'ExtensionAdb') -> bool:
        if isinstance(other, ExtensionAdb):
            return self.name == other.name
        return False

    @property
    def info(self) -> dict:
        return dict(
            adblocker_url=self.adblocker_url,
        )

    @property
    def name(self) -> str:
        return self.type.adb_name

    @abstractmethod
    def get_download_url(self, browser_name):
        pass

    @abstractmethod
    def setup(self, browser, filters_list: list[str] = None):
        pass

    def _check_info(self):
        if self.adblocker_url is None:
            raise ExtensionInfoException("Extension info is empty")


class ChromeExtensionInternal(ExtensionAdb):
    _browser_scheme = 'chrome-extension'
    _browser_error_page = 'chrome-error://chromewebdata/'

    def __init__(self, adb_type, url):
        ExtensionAdb.__init__(self, adb_type)
        self.type = adb_type
        self._url = url

    def get_download_url(self, browser_name):
        return self._url

    def setup(self, browser, filters_list=None):
        self._check_info()
        url_ext_prefix = "{}://{}/".format(self._browser_scheme, self.__get_adblock_id(browser))
        browser.logger.info("Update {} filter lists".format(self.type.adb_name))
        if filters_list:
            self._add_custom_filters(browser, url_ext_prefix, filters_list)
        else:
            self._update_adblock_filters(browser, url_ext_prefix)
        sleep(5)
        browser.logger.info("\tComplete...")

    def __get_adblock_id(self, browser):
        browser.open_url('chrome://system/')
        sleep(10)
        browser.click_element_via_js('#extensions-value-btn')
        extensions = browser.get_element_text_by_element_id('extensions-value').split('\n')
        for ext in extensions:
            browser.logger.info(ext)
            if self.type.adb_name in ext:
                browser.logger.info(ext)
                return ext.split(' ')[0]
        raise ExtensionAdblockException("Can't find {} extension chrome id. Exit session...".format(self.type.adb_name))

    @abstractmethod
    def _update_adblock_filters(self, browser, url_ext_prefix):
        pass

    @abstractmethod
    def _add_custom_filters(self, browser, url_ext_prefix, filters_list):
        pass


class OperaExtensionInternal(ExtensionAdb):
    type = AdblockTypes.OPERA

    def __init__(self, _):
        ExtensionAdb.__init__(self, self.type)

    def get_download_url(self, browser_name):
        return "opera:/settings"

    def setup(self, browser, filters_list=None):
        browser.open_url("opera:/settings")
        sleep(3)  # ожидание прогрузки страницы
        browser.logger.info("\tTurn on opera adblock")
        browser.driver.execute_script(
            'document.querySelector("body > main-view")'
            '.shadowRoot.querySelector("section > settings-ui")'
            '.shadowRoot.querySelector("#main")'
            '.shadowRoot.querySelector("settings-basic-page")'
            '.shadowRoot.querySelector("#basicPage '
            '> settings-section:nth-child(2) > opera-ad-blocker")'
            '.shadowRoot.querySelector("#pages > neon-animatable '
            '> div.settings-box.first > cr-toggle")'
            '.shadowRoot.querySelector("#button").click()'
        )
        browser.logger.info("\tTurn on opera block trackers")
        browser.driver.execute_script(
            'document.querySelector("body > main-view")'
            '.shadowRoot.querySelector("section > settings-ui")'
            '.shadowRoot.querySelector("#main")'
            '.shadowRoot.querySelector("settings-basic-page")'
            '.shadowRoot.querySelector("#basicPage '
            '> settings-section:nth-child(2) > opera-ad-blocker")'
            '.shadowRoot.querySelector("#pages > neon-animatable '
            '> div:nth-child(3) > cr-toggle")'
            '.shadowRoot.querySelector("#button").click()'
        )
        browser.logger.info("\tDelete [*.]yandex.com from adblock-exception-list")
        browser.open_url("opera:/settings/adBlockerExceptions", clear_pages=False)
        sleep(3)
        browser.driver.execute_script(
            'document.querySelector("body > main-view").shadowRoot.querySelector("section > settings-ui")'
            '.shadowRoot.querySelector("#main").shadowRoot.querySelector("settings-basic-page").shadowRoot'
            '.querySelector("#basicPage > settings-section:nth-child(2) > opera-ad-blocker").shadowRoot'
            '.querySelector("#pages > settings-subpage > opera-ad-blocker-exceptions").shadowRoot'
            '.querySelector("#ad-blocker-allow-list").shadowRoot'
            '.querySelector(\'#category > #listContainer > iron-list > [aria-rowindex="6"\').shadowRoot'
            '.querySelector(".list-item > cr-icon-button").click()'
        )
        browser.logger.info("\tComplete...")


class FirefoxExtension(ExtensionAdb):
    def __init__(self, adb_type, url):
        ExtensionAdb.__init__(self, adb_type)
        self.type = adb_type
        self._url = url

    def get_download_url(self, browser_name):
        return self._url

    def setup(self, browser, filters_list=None):
        self._check_info()
        url_ext_prefix = 'moz-extension://{}/'.format(self.__get_adblock_id(browser))
        browser.logger.info("Update {} filter lists".format(self.type.adb_name))
        if filters_list:
            self._add_custom_filters(browser, url_ext_prefix, filters_list)
        else:
            self._update_adblock_filters(browser, url_ext_prefix)
        sleep(5)
        browser.logger.info("\tComplete...")

    def __get_adblock_id(self, browser):
        """
        Ищем id расширения:
        Open about:memory. Click "measure" in Show memory reports.
        In the Main Process section, scroll down to Other Measurements.
        There you will find the installed (active) extensions with their names and their ids displayed as
        baseURL=moz-extension://[random-ids].
        На самом деле это не id, а часть базового пути по которому мы можем обратиться к настройкам расширения,
        но для идентичности обозначения будем называть его id
        """
        browser.open_url('about:memory')
        browser.click_element_via_js('button[id="measureButton"]')
        browser.wait_element_available_to_click('[id*=":extensions"]', timeout=30)

        extensions = browser.driver.find_elements_by_css_selector(
            '[id*=":extensions"] + [class="kids"] > [class="mrName"]'
        )
        for extension in extensions:
            # skip modheader-ext
            if 'yandex-team' in extension.text:
                continue
            if extension.text.split('id=', 1)[1].split(', name=', 1)[0].rsplit('@', 1)[-1] not in [
                'mozilla.org',
                'search.mozilla.org',
            ]:
                return extension.text.split('baseURL=moz-extension://', 1)[1].split('/', 1)[0]

        raise ExtensionAdblockException(
            "Can't find {} extension firefox id. Exit session...".format(self.type.adb_name)
        )

    @abstractmethod
    def _update_adblock_filters(self, browser, url_ext_prefix):
        pass

    @abstractmethod
    def _add_custom_filters(self, browser, url_ext_prefix, filters_list):
        pass


class Incognito(ExtensionAdb):
    type = AdblockTypes.INCOGNITO

    def __init__(self, _):
        ExtensionAdb.__init__(self, self.type)

    def setup(self, browser, filters_list=None):
        pass

    def get_download_url(self, browser_name):
        pass


class WithoutAdblock(ExtensionAdb):
    type = AdblockTypes.WITHOUT_ADBLOCK

    def __init__(self, _):
        ExtensionAdb.__init__(self, self.type)

    def setup(self, browser, filters_list=None):
        pass

    def get_download_url(self, browser_name):
        pass


class WithoutAdblockCrypted(ExtensionAdb):
    type = AdblockTypes.WITHOUT_ADBLOCK_CRYPTED

    def __init__(self, _):
        ExtensionAdb.__init__(self, self.type)

    def setup(self, browser, filters_list=None):
        pass

    def get_download_url(self, browser_name):
        pass


def add_custom_filters_for_adblock(browser, setting_url, filters_list):
    browser.open_url(setting_url)
    browser.click_element('span[i18n="tabfilterlists"]')

    browser.logger.info("\tOff all filter lists...")
    # off all filter lists
    browser.wait_element_on_page('.no-bottom-line')
    browser.driver.execute_script(
        "document.querySelectorAll('input[type=\"checkbox\"]:checked').forEach(button=>button.click())"
    )

    try:
        alert = browser.driver.switch_to.alert
        alert.accept()
        browser.logger.info("\tAccept alert...")
    except:
        pass

    browser.logger.info('\tAdd custom filter lists...')
    for item in filters_list:
        browser.logger.info("\tAdd filter list {}".format(item))
        browser.driver.execute_script(
            "document.querySelector('[id=\"txtNewSubscriptionUrl\"]').value='{}'".format(item)
        )
        browser.click_element_via_js(css_selector="#btnNewSubscriptionUrl")


def add_custom_filters_for_adblockplus(browser, setting_url, filters_list):
    browser.open_url(setting_url)
    browser.wait_element_on_page('#all-filter-lists-table > li:nth-child(2) > div:nth-child(5) > button:nth-child(1)')
    browser.logger.info('\tClear old lists...')
    browser.driver.execute_script(
        "document.querySelectorAll('[class=\"control icon delete\"]').forEach(button=>button.click())"
    )

    browser.logger.info('\tAdd custom filter lists...')
    for item in filters_list:
        browser.logger.info("\tAdd filter list {}".format(item))
        browser.click_element_via_js('button[data-i18n=\"options_filterList_add\"]')
        browser.driver.execute_script("document.querySelector('[id=\"import-list-url\"]').value='{}'".format(item))
        browser.click_element_via_js('button[data-i18n=\"options_dialog_import_title\"]')


def add_custom_filters_for_adguard(browser, setting_url, filters_list):
    browser.open_url(setting_url)
    browser.logger.info('\tOff old lists...')
    sleep(1)
    browser.wait_element_on_page("#category1 > div:nth-child(2) > div:nth-child(2) > div:nth-child(2)")
    browser.driver.execute_script(
        "document.querySelectorAll('[id*=\"category\"] [class=\"toggler active\"]').forEach(button=>button.click())"
    )
    browser.wait_element_on_page("#category0 > div > div.toggler-wr > div")
    browser.click_element_via_js(css_selector="#category0 > div > div.toggler-wr > div")

    browser.logger.info('\tOpen custom preferences.')
    browser.click_element_via_js(css_selector='#aria-category0 > div.block-type__desc-title')
    browser.wait_element_on_page("button.button:nth-child(3)")

    browser.click_element_via_js(css_selector='#antibanner0 > div.settings-body > div > button')

    browser.logger.info('\tAdd custom filter lists...')
    for item in filters_list:
        browser.logger.info("\tAdd filter list {}".format(item))
        browser.wait_element_on_page("#custom-filter-popup-url")
        browser.driver.execute_script(
            "document.querySelector('[id=\"custom-filter-popup-url\"]').value='{}'".format(item)
        )
        browser.wait_element_on_page(".custom-filter-popup-next")
        browser.click_element_via_js(
            css_selector="#add-custom-filter-step-1 > div.option-popup__btns.option-popup__btns--sb > button"
        )
        browser.wait_element_on_page(".settings-checkboxes__label")
        browser.click_element_via_js(
            css_selector="#add-custom-filter-step-4 > div.option-popup__content > "
            "div.option-popup__checkbox > div:nth-child(1) > label"
        )
        browser.wait_element_on_page(xpath="//*[@id=\"custom-filter-popup-added-subscribe\"]")
        browser.wait_element_available_to_click("#add-custom-filter-step-4 > div:nth-child(3)")
        browser.click_element("#custom-filter-popup-added-subscribe")
        if item != filters_list[-1]:
            browser.wait_element_available_to_click('#antibanner0 > div.settings-actions > a', timeout=15)

    browser.open_url(setting_url)
    browser.logger.info("\tClicking Update list button...")
    browser.wait_element_on_page(".settings-actions__update-button")
    browser.click_element(".settings-actions__update-button")


def add_custom_filters_for_ublock(browser, setting_url, filters_list):
    browser.open_url(setting_url)
    # browser.wait_element_on_page("#dashboard-nav > span:nth-child(3)")
    # browser.click_element_via_js(css_selector="#dashboard-nav > span:nth-child(3)")
    browser.driver.switch_to_frame("iframe")
    browser.wait_element_on_page(xpath='/html/body/div[1]/div[4]/div/div/div[1]/div[2]/div')

    # off all filter lists
    browser.driver.execute_script(
        "document.querySelectorAll('input[type=\"checkbox\"]:checked').forEach(button=>button.click())"
    )

    # accept import lists
    browser.driver.execute_script(
        "document.querySelector('[type=\"checkbox\"][id=\"importLists\"]:not(:checked)').click()"
    )

    browser.driver.execute_script(
        "document.querySelector('[id=\"externalLists\"]').value='{}'".format("\\n".join(filters_list))
    )
    browser.wait_element_on_page("#buttonApply > span:nth-child(2)")
    browser.wait_element_available_to_click("#buttonApply > span:nth-child(2)")
    browser.click_element(css_selector="#buttonApply > span:nth-child(2)")
    sleep(2)


class AdblockChromeInternal(ChromeExtensionInternal):
    type = AdblockTypes.ADBLOCK
    _extension_page = 'options.html'

    def __init__(self, internal_extensions):
        ChromeExtensionInternal.__init__(self, self.type, internal_extensions["ADBLOCK_CHROME"])

    def _update_adblock_filters(self, browser, url_ext_prefix):
        browser.open_url(url_ext_prefix + self._extension_page)
        browser.click_element('span[i18n="tabfilterlists"]')
        browser.logger.info("\tClicking Update list button...")
        browser.wait_element_available_to_click('[id="btnUpdateNow"]')
        browser.click_element('[id="btnUpdateNow"]')

    def _add_custom_filters(self, browser, url_ext_prefix, filters_list):
        add_custom_filters_for_adblock(browser, url_ext_prefix + self._extension_page, filters_list)


class AdblockPlusChromeInternal(ChromeExtensionInternal):
    type = AdblockTypes.ADBLOCK_PLUS
    _extension_page = 'desktop-options.html'

    def __init__(self, internal_extensions):
        ChromeExtensionInternal.__init__(self, self.type, internal_extensions["ADBLOCK_PLUS_CHROME"])

    def _update_adblock_filters(self, browser, url_ext_prefix):
        browser.open_url(url_ext_prefix + self._extension_page + "#advanced")
        browser.logger.info('\tClicking Update list button...')
        browser.click_element('[id="update"]')

        # https://st.yandex-team.ru/ANTIADB-1352 без этого кода adblock+ иногда не работает
        # получаем статусы всех листов
        filters_statuses = browser.driver.find_elements_by_css_selector(
            '#all-filter-lists-table > li > div ~ div > span.last-update'
        )
        # ждем 30 сек пока статусы всех листов будут "Just now" или 'Только что', в зависимости от локали
        i = 0
        while i < 30:
            if all([status.text in ['Just now', 'Только что'] for status in filters_statuses]):
                return
            i += 1
            sleep(1)
        raise ExtensionAdblockException('Adblock+ could not update filters in 30 seconds')

    def _add_custom_filters(self, browser, url_ext_prefix, filters_list):
        add_custom_filters_for_adblockplus(browser, url_ext_prefix + self._extension_page + "#advanced", filters_list)


class AdguardChromeInternal(ChromeExtensionInternal):
    type = AdblockTypes.ADGUARD
    _extension_page = 'pages/options.html'

    def __init__(self, internal_extensions):
        ChromeExtensionInternal.__init__(self, self.type, internal_extensions["ADGUARD_CHROME"])

    def _update_adblock_filters(self, browser, url_ext_prefix):
        browser.open_url(url_ext_prefix + self._extension_page + '#antibanner7')
        sleep(2)
        russian_filter_checkbox_css_selector = (
            '#filter1 > div.opt-state > div.toggler-wr > div'  # filter1 - русский язык в адгарде
        )
        russian_filter_checkbox_classes = browser.wait_element_on_page(
            russian_filter_checkbox_css_selector
        ).get_attribute("class")
        # если русский не включен - добавляем, это последствие того что английская локаль на селениумГриде
        if "active" not in russian_filter_checkbox_classes:
            browser.logger.info('\tEnable Russian filter list')
            browser.click_element(russian_filter_checkbox_css_selector, use_js_click=True)
        # и далее принудительно обновляем фильтры
        browser.open_url(url_ext_prefix + self._extension_page + '#antibanner')
        browser.logger.info("\tClicking Update list button...")
        browser.click_element('a[i18n-title="options_update_antibanner_filters"]')

    def _add_custom_filters(self, browser, url_ext_prefix, filters_list):
        add_custom_filters_for_adguard(browser, url_ext_prefix + self._extension_page + '#antibanner', filters_list)


class UblockOriginChromeInternal(ChromeExtensionInternal):
    type = AdblockTypes.UBLOCK_ORIGIN
    # todo get latest version from sandbox
    _extension_page = 'dashboard.html'

    def __init__(self, internal_extensions):
        ChromeExtensionInternal.__init__(self, self.type, internal_extensions["UBLOCK_ORIGIN_CHROME"])

    def _update_adblock_filters(self, browser, url_ext_prefix):
        browser.open_url(url_ext_prefix + self._extension_page + '#3p-filters.html')
        # browser.click_element_via_js(css_selector="#dashboard-nav > span:nth-child(3)")
        browser.driver.switch_to_frame("iframe")
        browser.wait_element_on_page('[id="buttonUpdate"]')
        browser.logger.info("\tClicking Update list button...")
        browser.wait_element_available_to_click('[id="buttonUpdate"]')

        # ждем 30 сек, чтоб если кнопка не активна
        i = 0
        while i < 30:
            # если кнопка имеет класс `disabled` то нажатие на нее не сработает.
            # Это может быть при отрисовке страницы, пробуем подождать ее активации
            button_update_classes = browser.wait_element_on_page('[id="buttonUpdate"]').get_attribute("class")
            if 'disabled' not in button_update_classes:
                browser.click_element('[id="buttonUpdate"]')
                return
            i += 1
            sleep(1)
        raise ExtensionAdblockException(
            '{} Update button does not become active in 30 seconds'.format(self.type.adb_name)
        )

    def _add_custom_filters(self, browser, url_ext_prefix, filters_list):
        add_custom_filters_for_ublock(browser, url_ext_prefix + self._extension_page + '#3p-filters.html', filters_list)


class AdblockFirefox(FirefoxExtension):
    type = AdblockTypes.ADBLOCK

    def __init__(self, internal_extensions):
        FirefoxExtension.__init__(self, self.type, internal_extensions["ADBLOCK_FIREFOX"])

    def _update_adblock_filters(self, browser, url_ext_prefix):
        browser.open_url(url_ext_prefix + 'options.html')
        browser.wait_element_on_page('a[href="#filters"] > span[i18n="tabfilterlists"]')
        browser.click_element_via_js('a[href="#filters"] > span[i18n=\"tabfilterlists\"]')
        browser.wait_element_on_page('[id=\"btnUpdateNow\"]')
        browser.logger.info("\tAdd RuAdList...")
        from selenium.webdriver.support.ui import Select

        Select(browser.driver.find_element_by_css_selector('[id=\"language_select\"]')).select_by_value('russian')
        sleep(4)  # ждем пока скачает русский лист и установит, TODO переделай на ожидание элемента
        browser.logger.info('\tClicking Update list button...')
        browser.click_element_via_js('[id=\"btnUpdateNow\"]')

    def _add_custom_filters(self, browser, url_ext_prefix, filters_list):
        add_custom_filters_for_adblock(browser, url_ext_prefix + 'options.html', filters_list)


class AdblockPlusFirefox(FirefoxExtension):
    type = AdblockTypes.ADBLOCK_PLUS

    def __init__(self, internal_extensions):
        FirefoxExtension.__init__(self, self.type, internal_extensions["ADBLOCK_PLUS_FIREFOX"])

    def _update_adblock_filters(self, browser, url_ext_prefix):
        browser.open_url(url_ext_prefix + 'desktop-options.html#advanced')
        browser.wait_element_on_page('[id=\"update\"]')

        browser.logger.info("\tAdd RuAdList...")
        browser.click_element_via_js('button[data-i18n=\"options_filterList_add\"]')
        browser.driver.execute_script(
            "document.querySelector('[id=\"import-list-url\"]').value='https://easylist-downloads.adblockplus.org/ruadlist+easylist.txt'"
        )
        # confirm
        browser.click_element_via_js('button[data-i18n=\"options_dialog_import_title\"]')

        # update all lists
        browser.logger.info('\tClicking Update list button...')
        browser.click_element_via_js('[id=\"update\"]')

        browser.logger.info('\tWait new page from AdblockPlus...')
        # открывается лишняя вкладка в рандомное время
        sleep(30)

    def _add_custom_filters(self, browser, url_ext_prefix, filters_list):
        add_custom_filters_for_adblockplus(browser, url_ext_prefix + 'desktop-options.html#advanced', filters_list)


class AdguardFirefox(FirefoxExtension):
    type = AdblockTypes.ADGUARD

    def __init__(self, internal_extensions):
        FirefoxExtension.__init__(self, self.type, internal_extensions["ADGUARD_FIREFOX"])

    def _update_adblock_filters(self, browser, url_ext_prefix):
        # страница языковых фильтров
        browser.open_url(url_ext_prefix + 'pages/options.html#antibanner7')
        sleep(1)  # TODO: без этого не всегда работает ожидание элемента, кидает StaleElementReferenceError
        russian_filter_checkbox_css_selector = (
            '#filter1 > div.opt-state > div.toggler-wr > div'  # filter1 - русский язык в адгарде
        )
        russian_filter_checkbox = browser.wait_element_on_page(russian_filter_checkbox_css_selector)
        # если русский не включен - добавляем, это последствие того что английская локаль на селениумГриде
        if "active" not in russian_filter_checkbox.get_attribute("class"):
            browser.logger.info('\tEnable Russian filter list')
            browser.click_element(russian_filter_checkbox_css_selector, use_js_click=True)

        # и далее принудительно обновляем фильтры
        browser.open_url(url_ext_prefix + 'pages/options.html#antibanner')
        browser.wait_element_on_page('a[i18n-title=\"options_update_antibanner_filters\"]')
        browser.logger.info("\tClicking Update list button...")
        browser.driver.find_element_by_css_selector('a[i18n-title=\"options_update_antibanner_filters\"]').click()

    def _add_custom_filters(self, browser, url_ext_prefix, filters_list):
        add_custom_filters_for_adguard(browser, url_ext_prefix + 'pages/options.html#antibanner', filters_list)


class UblockOriginFirefox(FirefoxExtension):
    type = AdblockTypes.UBLOCK_ORIGIN

    def __init__(self, internal_extensions):
        FirefoxExtension.__init__(self, self.type, internal_extensions["UBLOCK_ORIGIN_FIREFOX"])

    def _update_adblock_filters(self, browser, url_ext_prefix):
        try:
            # кнопка Обновить не всегда становится кликабельной
            browser.open_url(url_ext_prefix + 'dashboard.html#3p-filters.html')
            browser.wait_element_on_page('[id=\"buttonUpdate\"]')
            browser.logger.info("\tClicking Update list button...")
            browser.wait_element_available_to_click('[id=\"buttonUpdate\"]')
            browser.click_element('[id=\"buttonUpdate\"]', True)
        except:
            browser.logger.info("\tUpdate button isn't clickable")

    def _add_custom_filters(self, browser, url_ext_prefix, filters_list):
        add_custom_filters_for_ublock(browser, url_ext_prefix + 'dashboard.html#3p-filters.html', filters_list)
