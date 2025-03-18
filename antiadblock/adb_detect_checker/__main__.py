#!/usr/bin/env python
# -*- coding: utf8 -*-
import os
import json
import logging
import argparse
from queue import Queue
from threading import Thread
from time import sleep
from collections import defaultdict

from enum import Enum

from antiadblock.libs.adb_selenium_lib.browsers import Chrome, Firefox, Opera
from antiadblock.libs.adb_selenium_lib.schemas import BrowserInfo

from antiadblock.adb_detect_checker.config import AVAILABLE_ADBLOCKS_EXT_CHROME, AVAILABLE_ADBLOCKS_EXT_FIREFOX, \
    SELENIUM_EXECUTOR, AVAILABLE_ADBLOCKS_EXT_YABRO, AVAILABLE_ADBLOCKS_EXT_OPERA
from antiadblock.adb_detect_checker.detect_checker_browser import DetectCheckerBrowser
from antiadblock.adb_detect_checker.utils.stat_report import Report


class Browsers(Enum):
    OPERA = (Opera, 'opera', '70.0', AVAILABLE_ADBLOCKS_EXT_OPERA)
    CHROME = (Chrome, 'chrome', '83.0', AVAILABLE_ADBLOCKS_EXT_CHROME)
    YANDEX_BROWSER = (Chrome, 'yandex-browser', "20.7.2.51", AVAILABLE_ADBLOCKS_EXT_YABRO)
    FIREFOX = (Firefox, 'firefox', '79.0', AVAILABLE_ADBLOCKS_EXT_FIREFOX)

    def __init__(self, browser_type, selenium_browser_name, version, adb_extensions):
        self.browser_type = browser_type
        self.browser_info = BrowserInfo(
            name=selenium_browser_name,
            version=version
        )
        self.adb_extensions = adb_extensions


def thread_main(browser, adblock_class, selenium_executor, check_description, internal_extensions):
    with browser.browser_type(
        browser_info=browser.browser_info,
        adblock_class=adblock_class,
        selenium_executor=selenium_executor,
        internal_extensions=internal_extensions,
    ) as browser_instance:
        detect_instance = DetectCheckerBrowser(browser_instance)
        results = detect_instance.detect_check_list(SITES_TO_CHECK)
        for check in results:
            check.update(check_description)
        return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--local_run', action='store_true', help='Run without sending results to Stat')
    parser.add_argument('--has-cases-to-run', action='store_true', help='dry run printing "true" literal in case there are some checks to run')
    parser.add_argument('--extensions', help='Custom extensions json')
    parser.add_argument('--profile', help='Partner\'s profile json')
    parser.add_argument('--settings', help='Debug settings')
    args = parser.parse_args()

    if args.profile:
        with open(args.profile) as fp:
            results = json.load(fp)
            os.environ['PROFILE'] = json.dumps(results)

    if args.extensions:
        with open(args.extensions) as fp:
            results = json.load(fp)
            os.environ['EXTENSIONS'] = json.dumps(results)

    if args.settings:
        with open(args.settings) as fp:
            results = json.load(fp)
            for key in results:
                os.environ[key] = results[key]

    report = Report() if not args.local_run else None
    checked_cases = list()
    RESULTS = list()
    checks_results_queue = Queue()
    threads_list = list()

    extensions = json.loads(os.getenv('EXTENSIONS'))
    continue_previous = os.getenv("CONTINUE_PREVIOUS", "true").lower() == "true"
    # общий экзекутор, если нет отдельно для браузера, иначе {браузер:экзекутер}
    selenium_executors = json.loads(os.getenv('SELENIUM_EXECUTOR_JSON', '{}'))
    has_cases_to_run = args.has_cases_to_run

    if continue_previous and not args.local_run:
        checked_cases = report.get_checked_cases()

    for browser in Browsers:
        for adblock_class in browser.adb_extensions:
            check_description = dict(
                browser=browser.browser_info.name,
                browser_version=browser.browser_info.version,
                adblock=adblock_class.type.adb_name,
            )
            if check_description in checked_cases:
                continue

            # исполнитель может быть разным для разных бро
            selenium_executor = "http://{}/wd/hub".format(
                selenium_executors.get(browser.browser_info.name, (selenium_executors.get('all', SELENIUM_EXECUTOR)))
            )
            # Создаем по треду на связку [браузер - адблок]
            t = Thread(target=lambda q, args: q.put(thread_main(*args)),
                       args=(checks_results_queue, (browser, adblock_class, selenium_executor, check_description,
                                                    extensions)),
                       name='{}-{}'.format(browser.browser_info.name, adblock_class.type.adb_name))
            threads_list.append(t)

    if has_cases_to_run:
        print('true' if len(threads_list) > 0 else 'false')
        exit(code=0)

    SITES_TO_CHECK = json.loads(os.getenv('PROFILE'))

    for t in threads_list:
        t.start()

    # ждем окончания не более получаса. Каждые 30 сек проверяя очередь на наличие результатов, если они есть, то сразу выгружаем и постим в stat
    results_count = 0
    for i in range(60):
        while not checks_results_queue.empty():
            results = checks_results_queue.get()
            RESULTS += results
            if not args.local_run:
                report.upload_data(results)
            results_count += 1
        # если все потоки завершились - выходим
        if all([not t.is_alive() for t in threads_list]):
            break
        sleep(30)  # sleep half a minute

    if args.local_run:
        result_dict = defaultdict(list)
        for r in RESULTS:
            configuration = ' / '.join([r['browser'], r['browser_version'], r['adblock']])
            result_dict[configuration].append((r['service_id'], r['console_detect_result'], r['cookiewait_detect_result']))
        print()
        for configuration, results in result_dict.items():
            print(configuration.upper())
            print('{:^20} | {:^20} | {:^20}'.format('pid', 'console detect', 'cookiewait detect'))
            for r in results:
                print('{:<20} | {:^20} | {:^20}'.format(*r))
            print()

    # если не все треды вернули результат - выходим с ошибкой. TODO: сделать тут нормально
    if results_count != len(threads_list):
        exit(code=1)
