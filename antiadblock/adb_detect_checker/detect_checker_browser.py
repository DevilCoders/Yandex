# -*- coding: utf8 -*-
from time import sleep

from selenium.common import exceptions as selenium_exceptions

from antiadblock.libs.adb_selenium_lib.config import AdblockTypes
from antiadblock.adb_detect_checker.config import DETECT_SCRIPT, COOKIES_OF_THE_DAY_LIST, LUDCA_DECODE_SCRIPT


class DetectCheckerBrowser:

    def __init__(self, browser):
        self.browser = browser

    def detect_check_list(self, sites_to_check):
        result = []
        for site in sites_to_check:
            try:
                self.browser.open_url(site['url'])
                check = dict(
                    service_id=site['service_id'],
                    domain=site['url'],
                    # order of checks is important, this should go first
                    cookiewait_detect_result=self.__check_injected_detect(site['cookie']) if site['cookie'] else None,
                    console_detect_result=self.check_detect(site['service_id'])
                )
                result.append(check)
            except Exception as e:
                self.browser.logger.error("Check of {} failed. Traceback {}".format(site['service_id'], e.msg))
        self.browser.logger.info(result)
        return result

    def __cycada_exists(self, all_site_cookies):
        for cookie in all_site_cookies:
            if cookie["name"] == "cycada" and cookie["value"] != '':
                self.browser.logger.info("Find cycada: {}".format(cookie["value"]))
                return True

    def __get_cookies_of_the_day_on_site(self, all_site_cookies, cookie_name):
        self.browser.logger.info("Check cookie \'{}\' existing on site or one of cookie_of_the_day".format(cookie_name))
        self.browser.logger.info('Cookies of the day list {}'.format(COOKIES_OF_THE_DAY_LIST))
        actual_aab_cookies = list(set(all_site_cookies) & set(COOKIES_OF_THE_DAY_LIST + [cookie_name]))
        if actual_aab_cookies:
            self.browser.logger.info("Exists cookies of the day: " + ", ".join(actual_aab_cookies))
        return actual_aab_cookies

    @property
    def __has_adblock(self):
        return self.browser.adblock.type not in (AdblockTypes.INCOGNITO, AdblockTypes.WITHOUT_ADBLOCK)

    def __check_exception(self, cookies_of_the_day, cycada_exists):
        # если нет адблока и цикада не вставилась, или выставилась вместе с кукой заадблока,
        # или выставилась цикада под адблоком и кука адблока тоже
        if not self.__has_adblock and not cycada_exists:
            self.browser.logger.warning("Wrong situation: NO ADBLOCK, NO CYCADA")
        elif not self.__has_adblock and cookies_of_the_day and cycada_exists:
            self.browser.logger.warning("Wrong situation: NO ADBLOCK, CYCADA, COOKIE_OF_THE_DAY")
        elif self.__has_adblock and cycada_exists and cookies_of_the_day:
            self.browser.logger.warning("Wrong situation: ADBLOCK, CYCADA, COOKIE_OF_THE_DAY")
        else:
            return False
        self.browser.logger.warning("COOKIES_OF_THE_DAY " + ", ".join(cookies_of_the_day))
        return True

    def __check_injected_detect(self, cookie_name, timeout=10):
        """
        Сценарий:
        - заходим на сайт и ждем 10 секунд пока отработает детект без каких-либо внешних эффектов
        - перезагружаем страницу
        - проверяем что после перезагрузки появилась кука

        :param cookie_name: кука, которая ожидается на сайте
        :param timeout: время ожидания куки на сайте
        :return: строка BLOCKED или NOT_BLOCKED
        """
        self.browser.logger.info("Waiting for {} seconds to site's detect script worked".format(timeout))
        sleep(timeout)
        self.browser.refresh_page()

        all_site_cookies = self.browser.driver.get_cookies()
        all_site_cookies_names = [c['name'] for c in all_site_cookies]
        cookies_of_the_day = self.__get_cookies_of_the_day_on_site(all_site_cookies_names, cookie_name)
        cycada_exists = self.__cycada_exists(all_site_cookies)

        result = "BLOCKED" if cookies_of_the_day else "NOT_BLOCKED"
        if self.__check_exception(cookies_of_the_day, cycada_exists):
            result = "EXCEPTION"

        self.browser.logger.info("RESULT: " + result)
        ludka = self.browser.driver.execute_script(LUDCA_DECODE_SCRIPT)
        self.browser.logger.info("LUDKA value: " + (ludka if ludka is not None else "NO LUDKA"))

        return result

    def check_detect(self, pid):
        """
        Проверяет скрипт детекта в консоли сайта
        :param pid: партнер id для скрипта детекта
        :return строка - название блокировщика
        """
        browser = self.browser
        browser.driver.delete_all_cookies()
        # удаляем куки, которые мог проставить скрипт детекта и повлиять на результат
        # self.remove_cookies(['bltsr', 'blcrm', 'somecookie'])
        detect_script_full = DETECT_SCRIPT.replace('<PARTNER-ID>', pid)
        # необходимо переключится в дефолтный фрейм сайта, иначе не сработает адблок
        self.browser.switch_to(frame=None)
        # считываем лог с консоли, чтоб после детекта не выводить его полностью, а вывести только результат работы скрипта детекта
        _ = browser.get_console_log()
        # запустили скрипт детекта
        browser.logger.info('Check console detect')
        detect_result = None

        try:
            detect_result = browser.driver.execute_async_script(detect_script_full)['blocker']
        except selenium_exceptions.TimeoutException:
            detect_result = 'Script Timeout'
        except selenium_exceptions.JavascriptException as ex:  # TODO: иногда возникает какое-то исключение, повторить локально не выходит, пока залогируем чтоб понять что это
            browser.logger.error('JAVASCRIPT EXCEPTION EXIST: {}, trace: {}'.format(ex.msg, ex.stacktrace))
            detect_result = 'EXCEPTION'
        else:
            detect_console_log = browser.get_console_log()
            if detect_console_log:
                browser.logger.info('DETECT CONSOLE LOG:')
                for line in detect_console_log:
                    browser.logger.info('\t' + line['message'])
        finally:
            browser.logger.info('RESULT: ' + detect_result)
            # перед возвращением результата надо стереть куки, на всякий случай, чтоб как минимум blcrm не повлиял на результат
            browser.driver.delete_all_cookies()
        # self.remove_cookies(['bltsr', 'blcrm', 'somecookie'])
        return detect_result

