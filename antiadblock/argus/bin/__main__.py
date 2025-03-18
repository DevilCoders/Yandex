#!/usr/bin/env python
# -*- coding: utf8 -*-
import hashlib
import json
import logging
import os
import typing as t
from itertools import chain
from time import time
from urllib.parse import urlparse

import requests
from dotenv import load_dotenv

from antiadblock.argus.bin.browser_pool import BrowserPool
from antiadblock.argus.bin.config import (
    AVAILABLE_ADBLOCKS_EXT_CHROME,
    AVAILABLE_ADBLOCKS_EXT_FIREFOX,
    AVAILABLE_ADBLOCKS_EXT_OPERA,
    AVAILABLE_ADBLOCKS_EXT_YABRO,
    FILTERS_LISTS,
    SELENIUM_EXECUTOR,
)
from antiadblock.argus.bin.schemas import Browser, BrowserInfo, Profile
from antiadblock.argus.bin.task import ArgusResult, BrowserTask
from antiadblock.argus.bin.utils.s3_interactions import S3Uploader
from antiadblock.argus.bin.utils.utils import parse_cookies
from antiadblock.libs.adb_selenium_lib.browsers.chrome import Chrome
from antiadblock.libs.adb_selenium_lib.browsers.firefox import Firefox
from antiadblock.libs.adb_selenium_lib.browsers.opera import Opera
from antiadblock.libs.adb_selenium_lib.browsers.extensions import ExtensionAdb, WithoutAdblockCrypted
from antiadblock.libs.adb_selenium_lib.schemas import Cookie

logging.basicConfig(format='%(levelname)-8s [%(asctime)s] %(name)-30s: %(message)s', level=logging.INFO)

load_dotenv()

if path := os.getenv('SETTINGS_PATH'):
    with open(path) as f:
        for k, v in json.load(f).items():
            os.environ[k] = v

if path := os.getenv('PROFILE_PATH'):
    with open(path) as f:
        os.environ['PROFILE'] = f.read()

if path := os.getenv('EXTENSIONS_PATH'):
    with open(path) as f:
        os.environ['EXTENSIONS'] = f.read()

if path := os.getenv('BROWSERS_PATH'):
    with open(path) as f:
        os.environ['SELENOID_CONFIG'] = f.read()


def get_selenoid_hub(selenium_executors: dict, browser_name: str) -> str:
    host = selenium_executors.get(browser_name, selenium_executors.get('all', SELENIUM_EXECUTOR))
    return f'http://{host}/wd/hub'


# Looks like some deprecated code
def upload_filter_lists():
    result = []
    # пока треды поднимаются сохраняем текущие листы в s3
    # TODO: если не смогли скачать - надо пропустить будет лист, возможно ретрай, но обычно это не помогает
    for list in FILTERS_LISTS:
        r = requests.get(list)
        # добавим хэш, тогда не придется одни и те же листы по много раз сохранять
        list_name = urlparse(r.url).path.rsplit('/', 1)[1].rsplit('.', 1)  # реальное имя листа [имя, расширениие]
        list_name.insert(1, hashlib.md5(r.content).hexdigest())  # hash md5 добавляем после имени листа
        list_name = '.'.join(list_name)
        with open(list_name, 'wb') as f:
            f.write(r.content)
        if s3_client:
            result.append(s3_client.upload_file(list_name, rewrite=True))
        else:
            result.append(list_name)
    return result


def main(
    profile: Profile,
    browsers: t.Iterable[Browser],
    extensions: dict[BrowserInfo, t.Iterable[ExtensionAdb]],
    sandbox_task_id: str,
    s3_client: t.Optional[S3Uploader] = None,
) -> int:
    result = ArgusResult(sandbox_task_id, s3_client)
    browser_pool = BrowserPool(browsers, extensions)

    reference_tasks: list[BrowserTask] = []
    for browser in browsers:
        for reference_task in _create_reference_tasks(profile.run_id, profile, browser, result):
            browser_pool.submit(reference_task)
            reference_tasks.append(reference_task)

    replay_tasks: list[BrowserTask] = []
    for reference_task in reference_tasks:
        for replay_task in _create_replay_tasks(profile.run_id, profile, reference_task, extensions, result):
            browser_pool.submit(replay_task, reference_task)
            replay_tasks.append(replay_task)

    browser_pool.wait_idle()
    browser_pool.stop()
    result.get_and_save()

    excepted_tasks = [task for task in chain(reference_tasks, replay_tasks) if task.exception() is not None]
    return len(excepted_tasks)


def _create_reference_tasks(
    run_id: int, profile: Profile, browser: Browser, result: ArgusResult
) -> t.Iterable[BrowserTask]:
    for url_settings in map(lambda x: x.copy(deep=True), profile.url_settings):
        url_settings.headers = profile.headers
        url_settings.invert_detect = profile.invert_detect

        cookies = profile.cookies + [Cookie(name='argus_standard', value=_get_random_cookie_value())]
        if not url_settings.invert_detect:
            cookies += profile.proxy_cookies
        url_settings.cookies = cookies

        url_settings.is_reference = True

        task = BrowserTask(
            run_id=run_id,
            url_settings=url_settings,
            browser_info=browser.browser_info,
            extension=WithoutAdblockCrypted,
            argus_result=result,
        )
        yield task


def _get_random_cookie_value() -> str:
    return hashlib.sha224(str(time()).encode()).hexdigest()[:15]


def _create_replay_tasks(
    run_id: int, profile: Profile, reference_task: BrowserTask, extensions: t.Iterable[ExtensionAdb], result: ArgusResult
) -> t.Iterable[BrowserTask]:
    url_settings = reference_task.url_settings.copy(deep=True)

    argus_standard_cookie = _pop_cookie(url_settings.cookies, 'argus_standard')
    argus_replay_cookie = Cookie(name='argus_replay', value=argus_standard_cookie.value)

    url_settings.cookies = profile.cookies + [argus_replay_cookie]
    url_settings.is_reference = False

    for extension in extensions[reference_task.browser_info]:
        if extension != WithoutAdblockCrypted:
            task = BrowserTask(
                run_id=run_id,
                url_settings=url_settings,
                browser_info=reference_task.browser_info,
                extension=extension,
                argus_result=result,
            )
            yield task


def _pop_cookie(cookies: list[Cookie], name: str) -> Cookie:
    for i in range(len(cookies)):
        if cookies[i].name == name:
            return cookies.pop(i)
    raise ValueError(f'No {name} cookie in list')


if __name__ == '__main__':
    local_run: bool = os.getenv('LOCAL_RUN', 'false').lower() == 'true'

    selenoid_config: dict = json.loads(os.getenv('SELENOID_CONFIG', '{}'))

    profile_dict = json.loads(os.getenv('PROFILE'))
    profile_dict['cookies'] = parse_cookies(str(profile_dict['cookies']))
    profile_dict['proxy_cookies'] = parse_cookies(str(profile_dict['proxy_cookies']))
    profile = Profile(**profile_dict)

    # Страшный костыль, из-за того что в админке схема фиксированная
    # и новое поле надо добавлять и туда
    for item in profile.url_settings:
        if 'auto.ru' in item.url:  # Сейчас такую схему использует только auto.ru
            profile.invert_detect = True
            break

    util_extensions = json.loads(os.getenv('EXTENSIONS'))

    selenium_executors: dict = json.loads(os.getenv('SELENIUM_EXECUTOR_JSON', '{}'))
    sandbox_task_id: str = os.getenv('SANDBOX_TASK_ID', '0')
    s3_client: t.Optional[S3Uploader] = None if local_run else S3Uploader(logging)

    max_retries: int = int(os.getenv('MAX_RETRIES', 1))

    chrome = Browser(
        browser_type=Chrome,
        browser_info=BrowserInfo(name='chrome', version=selenoid_config['chrome']['default']),
        util_extensions=util_extensions,
        additional_filters=profile.filters_list,
        selenoid_url=get_selenoid_hub(selenium_executors, 'chrome'),
    )
    firefox = Browser(
        browser_type=Firefox,
        browser_info=BrowserInfo(name='firefox', version=selenoid_config['firefox']['default']),
        util_extensions=util_extensions,
        additional_filters=profile.filters_list,
        selenoid_url=get_selenoid_hub(selenium_executors, 'firefox'),
    )
    yandex_browser = Browser(
        browser_type=Chrome,
        browser_info=BrowserInfo(name='yandex-browser', version=selenoid_config['yandex-browser']['default']),
        util_extensions=util_extensions,
        additional_filters=profile.filters_list,
        selenoid_url=get_selenoid_hub(selenium_executors, 'yandex-browser'),
    )
    opera = Browser(
        browser_type=Opera,
        browser_info=BrowserInfo(name='opera', version=selenoid_config['opera']['default']),
        util_extensions=util_extensions,
        additional_filters=profile.filters_list,
        selenoid_url=get_selenoid_hub(selenium_executors, 'opera'),
    )
    browsers = (chrome, firefox, yandex_browser, opera)
    browsers_extensions = {
        chrome.browser_info: AVAILABLE_ADBLOCKS_EXT_CHROME + [WithoutAdblockCrypted],
        firefox.browser_info: AVAILABLE_ADBLOCKS_EXT_FIREFOX + [WithoutAdblockCrypted],
        yandex_browser.browser_info: AVAILABLE_ADBLOCKS_EXT_YABRO + [WithoutAdblockCrypted],
        opera.browser_info: AVAILABLE_ADBLOCKS_EXT_OPERA + [WithoutAdblockCrypted],
    }

    logging.info(f'BROWSERS: {browsers}')
    logging.info(f'PROFILE: {profile}')
    logging.info(f'EXTENSIONS: {util_extensions}')
    if local_run:
        logging.info('RUNNING ARGUS LOCALLY')

    status = main(
        profile=profile,
        browsers=browsers,
        extensions=browsers_extensions,
        sandbox_task_id=sandbox_task_id,
        s3_client=s3_client,
    )
    exit(status)
