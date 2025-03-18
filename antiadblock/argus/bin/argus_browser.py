# -*- coding: utf8 -*-
import hashlib
import re
from copy import deepcopy
from functools import partial
from time import sleep, time

from antiadblock.argus.bin.config import DETECT_TIMEOUT
from antiadblock.argus.bin.schemas import Result, SiteUrl, UrlSettings
from antiadblock.argus.bin.utils.utils import current_time, get_cookie_domain
from antiadblock.libs.adb_selenium_lib.browsers.base_browser import BaseBrowser
from antiadblock.libs.adb_selenium_lib.config import AdblockTypes
from antiadblock.libs.adb_selenium_lib.schemas import Cookie


def get_logs_dict() -> dict[str, dict]:
    result = {}
    for log_type in (
        'cryprox',
        'nginx',
        'balancer',
        'bs-event-log',
        'bs-dsp-log',
        'bs-hit-log',
        'elastic-count',
        'elastic-auction',
    ):
        result[log_type] = dict(status='new', url='', meta={})
    return result


EXTENSIONS_WITHOUT_RULES: list[AdblockTypes] = [
    AdblockTypes.INCOGNITO,
    AdblockTypes.WITHOUT_ADBLOCK,
    AdblockTypes.WITHOUT_ADBLOCK_CRYPTED,
    AdblockTypes.OPERA,  # no patch
]


class ArgusBrowser:
    def __init__(self, browser: BaseBrowser) -> None:
        self.browser: BaseBrowser = browser
        self.detect_pass_list: set[str] = set()

    def get_result_for_task(self, run_id: int, case_id: int, url_settings: UrlSettings) -> Result:
        headers = deepcopy(url_settings.headers)
        for retry_iter in range(url_settings.count_of_retries):
            start = current_time()

            # Reference launch already has detect cookies
            if not url_settings.is_reference:
                self._update_detect_pass_list(url_settings.url)

            self._add_task_case_settings(run_id, case_id, url_settings, headers)

            self.browser.open_url(
                url_settings.url,
                _on_fail=partial(self._add_task_case_settings, run_id=run_id, case_id=case_id, url_settings=url_settings, headers=headers),
            )
            sleep(15)  # придется это делать, загрузка страницы с точки зрения селениума может завершится,
            # но медийка, например, может появится через секунду-две

            screenshot_path = self.browser.get_page_full_screenshot(
                callback_functions=[partial(self._hide_elements_by_css, url_settings.selectors)],
                wait_sec=url_settings.wait_sec,
                scroll_count=url_settings.scroll_count,
            )
            cookies = self.browser.get_cookies()

            result = Result(
                browser=self.browser.name,
                browser_version=self.browser.version,
                adblocker=self.browser.adblock.name,
                adblocker_url=self.browser.adblock.info['adblocker_url'],
                url=url_settings.url,
                start=start,
                end=current_time(),
                headers=headers,
                logs=get_logs_dict(),
                cookies=cookies,
                ludca=self.browser.get_ludca(),
                adb_bits=cookies.get('adb_bits'),
                img_url=screenshot_path,
                rules=self.browser.get_triggered_rules(),
                console_logs=self.browser.get_console_log(),
            )
            if result.cookies is None:
                self.browser.logger.warning(f'No cookies {url_settings.url} session_id: {self.browser.driver.session_id}')
            if result.adb_bits is None:
                self.browser.logger.warning(f'No adb_bits {url_settings.url} session_id: {self.browser.driver.session_id}')

            if result.rules is None and not self._is_no_rules_case(url_settings.url):
                self.browser.logger.warning(f'No rules {url_settings.url} session_id: {self.browser.driver.session_id}')
                if retry_iter == url_settings.count_of_retries - 1:
                    self.browser.logger.error(f'BAD {url_settings.url} session_id: {self.browser.driver.session_id}')
                    raise RuntimeError(f'Failed task {url_settings.url}. Retrying')  # Restart whole session
                continue

            self.browser.logger.info(f'OK {url_settings.url} session_id: {self.browser.driver.session_id}')
            return result

    def _add_task_case_settings(
        self,
        run_id: int,
        case_id: int,
        url_settings: UrlSettings,
        headers: dict[str, str],
    ) -> None:
        self.browser.clear_pages()
        self.browser.clear_headers()

        self.browser.logger.info('Add case settings')

        request_id = self._gen_x_aab_request_id(url_settings.url)
        adbbits = self._gen_adbbits(case_id, run_id, request_id)

        # Clear cookies before setting new
        remove_cookies = [
            Cookie(name='adb_bits'),
            Cookie(name='argus_standard'),
            Cookie(name='argus_replay'),
        ]

        for cookie in url_settings.cookies:
            if cookie.name in ('argus_standard', 'argus_replay'):
                execution_name = cookie.name.split('_')[1]
                headers[f'x-aab-argus-{execution_name}'] = cookie.value
                break
        else:
            raise RuntimeError('Nor argus_standard, nor argus_replay set')

        add_cookies = [
            Cookie(name='adb_bits', value=str(adbbits)),
            *url_settings.cookies,
        ]

        block_cookies = []
        if url_settings.invert_detect and self.browser.adblock.type != AdblockTypes.WITHOUT_ADBLOCK:
            block_cookies.append(Cookie(name='cycada'))

        self.browser.logger.info(f'Add cookies {request_id}')
        self.browser.set_cookies(
            add=add_cookies, remove=remove_cookies, block=block_cookies, domain=get_cookie_domain(url_settings.url)
        )

        self.browser.logger.info(f'Add headers {request_id}')
        self._add_headers(headers, request_id)

    def _is_no_rules_case(self, url: SiteUrl) -> bool:
        """
        Когда yabro и сайт на домене yandex.ru отключается блокировщик. Смысла ретраить нет.
        Так же не ретраить при WITHOUT_ADBLOCK
        """
        domain = self._get_url_domain(url)
        return (domain in ('yandex.ru', 'zen.yandex.ru') and self.browser.browser_info.name == 'yandex-browser') or (
            self.browser.adblock.type in EXTENSIONS_WITHOUT_RULES
        )

    def _update_detect_pass_list(self, url: SiteUrl) -> None:
        domain = self._get_url_domain(url)
        if domain not in self.detect_pass_list:
            self.detect_pass_list.add(domain)
            self.browser.open_url(url)
            self.browser.logger.info(f'Waiting for {DETECT_TIMEOUT} seconds to site\'s detect script worked')
            sleep(DETECT_TIMEOUT)

    def _hide_elements_by_css(self, selectors: list[str]) -> None:
        for selector in selectors:
            self.browser.set_display_none_by_css_selector(selector)

    def _gen_adbbits(self, case_id: int, run_id: int, request_id: str) -> int:
        """
        [10 bits run_id_mask][10 bits case_id_mask][11 bits request_id_mask][1bit offset]
        """
        run_id_mask = run_id & ((1 << 10) - 1)
        case_id_mask = case_id & ((1 << 10) - 1)
        request_id_mask = int(request_id, 16) & ((1 << 11) - 1)
        return ((run_id_mask << 22) | (case_id_mask << 12) | request_id_mask) << 1

    def _gen_x_aab_request_id(self, url: SiteUrl) -> str:
        case_str = url + self.browser.name + self.browser.adblock.name + str(time())
        return hashlib.sha224(case_str.encode()).hexdigest()[:15]

    def _get_url_domain(self, url: str) -> str:
        return re.match(r'(?:https?://)?(?:www\.)?([\w.\-]*)/?', url).group(1)

    def _gen_cookie_per_page(self, url_settings: UrlSettings) -> Cookie:
        if url_settings.is_reference:
            url_settings.cookie_per_page.value = hashlib.sha224(str(time()).encode()).hexdigest()[:15]
            url_settings.cookie_per_page.domain = get_cookie_domain(url_settings.url)
        return url_settings.cookie_per_page

    def _add_headers(self, headers: dict[str, str], request_id: str) -> None:
        headers['x-aab-requestid'] = request_id + '-{cnt}'
        self.browser.add_headers(headers)
