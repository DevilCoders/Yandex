# coding=utf-8
# Утилита для автоматической смены куки дня
import os
from datetime import datetime, timedelta

from enum import IntEnum

import startrek_client
from startrek_client.exceptions import NotFound
from tvmauth import TvmClient, TvmApiClientSettings

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.configs_api import post_tvm_request, get_configs, get_configs_from_api as get_tvm_request
from antiadblock.adblock_rule_sonar.sonar.lib.utils.sonar_startrek import ISSUE_TAG
from antiadblock.libs.utils.utils import get_abc_duty


logger = create_logger("change_current_cookie")

TVM_ID = int(os.getenv("TVM_ID", "2002631"))  # use SANDBOX monitoring tvm_id
TVM_SECRET = os.getenv("TVM_SECRET")
CONFIG_STATUS = os.getenv("CONFIG_STATUS", "test")  # "test" or "active"
CONFIGS_API_TVM_ID = int(os.getenv("CONFIGSAPI_TVM_ID", "2000627"))
CONFIGS_API_HOST = os.getenv("CONFIGS_API_HOST", "test.aabadmin.yandex.ru")
STARTREK_TOKEN = os.getenv('STARTREK_TOKEN')
ZEN_FOLLOWERS = [x.strip() for x in os.getenv('ZEN_FOLLOWERS', 'eugenepolitko').split(',')]
LIMIT_COOKIE_CHANGES = int(os.getenv("LIMIT_COOKIE_CHANGES", "3"))
DELTA_HOURS_COOKIE_CHANGES = int(os.getenv("DELTA_HOURS_COOKIE_CHANGES", "4"))
ABC_TOKEN = os.getenv('ABC_TOKEN')
QUEUE_NAME = 'ANTIADBSUP'

CURRENT_UTC_DATETIME = datetime.utcnow()


class CookieChangeStatus(IntEnum):
    """
    Simple CookieChangeStatus classes
    DoNotUpdate - куку пока не меняем,
    AlreadyUpdated - куку сменили раньше на другом сервисе (один родитель)
    Update - куку можно менять
    """
    (DoNotUpdate,
     AlreadyUpdated,
     Update) = range(3)


def get_issues_with_service_ids(st_client):

    # получим все тикеты без резолюции в саппортной очереди с соответствующим тегом
    issues = st_client.issues.find(
        filter={
            "queue": QUEUE_NAME,
            "resolution": "empty()",
            "tags": [ISSUE_TAG]
        }
    )
    # сервисы из админки
    services_from_configs_api = set(get_configs(TVM_ID, TVM_SECRET, CONFIGS_API_TVM_ID, CONFIGS_API_HOST))
    result = {}
    for issue in issues:
        _components = {int(component.id) for component in issue.components}
        # пересечем компоненты с сервисами из админки
        services = {component['name'] for component in st_client.queues[QUEUE_NAME].components if component['id'] in _components} & services_from_configs_api
        result[issue] = sorted(services)

    return result


def need_cookie_change(updated_label_ids, service_id, tvm_client):
    """
    :param updated_label_ids:
    :param service_id:
    :param tvm_client:
    :return: CookieChangeStatus
    """
    log_action_api_url = 'https://{}/audit/service/{}?offset=0&limit=50'.format(CONFIGS_API_HOST, service_id)
    change_cookie_counts = 0
    last_change = None
    for item in get_tvm_request(log_action_api_url, tvm_client)["items"]:
        item_date = datetime.strptime(item["date"], '%a, %d %b %Y %H:%M:%S GMT')
        if item_date < datetime.utcnow().replace(hour=0, minute=0, second=0):
            break
        if item["action"] != "config_mark_{}".format(CONFIG_STATUS) or "new cookie" not in item["params"]["config_comment"].lower():
            continue
        # так как конфиги имеют иерахическую структуру, возможно куку уже обновили
        if (item['label_id'], item["params"].get("new_cookie", "")) in updated_label_ids:
            return CookieChangeStatus.AlreadyUpdated
        last_change = last_change or item_date
        change_cookie_counts += 1
    # проверяем достижение дневного лимита
    if change_cookie_counts >= LIMIT_COOKIE_CHANGES:
        logger.info("Daily limit for cookie changes has been reached, service {}\n".format(service_id))
        return CookieChangeStatus.DoNotUpdate

    # меняем не раньше DELTA часов от предыдущей смены
    if change_cookie_counts > 0:
        if CURRENT_UTC_DATETIME - last_change < timedelta(hours=DELTA_HOURS_COOKIE_CHANGES):
            logger.info("Cookie changed less than {} hours ago, service {}\n".format(DELTA_HOURS_COOKIE_CHANGES, service_id))
            return CookieChangeStatus.DoNotUpdate

    return CookieChangeStatus.Update


if __name__ == "__main__":
    # Логика смены куки (время МСК, но в коде работаем с UTC)
    # 1. меняем только во временном диапозоне 9-21
    # 2. дневной лимит LIMIT_COOKIE_CHANGES
    # 3. не раньше DELTA_HOURS_COOKIE_CHANGES часов от предыдущей смены

    if CURRENT_UTC_DATETIME.hour < 6 or CURRENT_UTC_DATETIME.hour > 17:
        logger.info("This is not the time to change cookie")
        exit(0)

    assert CONFIG_STATUS in ("active", "test")

    logger.info("Task started")
    logger.info("Configs API host: {}, config status: {}".format(CONFIGS_API_HOST, CONFIG_STATUS))

    configs_api_url = "https://{}/v2/change_current_cookie".format(CONFIGS_API_HOST)

    tvm_settings = TvmApiClientSettings(
        self_tvm_id=TVM_ID,
        self_secret=TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=CONFIGS_API_TVM_ID)
    )

    tvm_client = TvmClient(tvm_settings)

    st_client = startrek_client.Startrek(useragent='python', token=STARTREK_TOKEN)
    duty_login, _ = get_abc_duty(ABC_TOKEN)
    #  множество пар (label_id, new_cookie), для которых уже сменили куку
    updated_label_ids = set()
    issues = get_issues_with_service_ids(st_client)
    exception = None
    for issue, service_ids in issues.items():
        logger.info("Issue {}, service_ids: {}\n".format(issue.key, ', '.join(service_ids)))
        not_fixed_services = set()
        new_yandex_cookie = ""
        comment = ""
        for service_id in service_ids:
            cookie_change_status = need_cookie_change(updated_label_ids, service_id, tvm_client)
            if cookie_change_status == CookieChangeStatus.DoNotUpdate:
                not_fixed_services.add(service_id)
                continue
            elif cookie_change_status == CookieChangeStatus.AlreadyUpdated:
                continue
            logger.info("Change current cookie on {}".format(service_id))
            data = {'service_id': service_id, 'test': CONFIG_STATUS == "test"}
            try:
                response = post_tvm_request(configs_api_url, tvm_client, data)
                if response.status_code == 201:
                    new_cookie = response.json()['new_cookie']
                    available_cookies_count = response.json()['available_cookies_count']
                    label_id = response.json()['label_id']
                    comment += "Label_id: {}, new cookie: {}, available cookies count: {}\n\n".format(label_id, new_cookie, available_cookies_count)
                    logger.info("New cookie: {}, available cookies count: {}".format(new_cookie, available_cookies_count))
                    if label_id == "yandex_domain":
                        new_yandex_cookie = new_cookie
                    updated_label_ids.add((label_id, new_cookie))

                logger.info("Result: {} {}\n".format(response.status_code, response.json().get("message", "Cookie changed")))
            except Exception as e:
                exception = e
                logger.exception("Exception occurred while executing the request: {}".format(str(e)))
        try:
            # добавление комментария приводит к смене статуса тикета (new|open -> inProgress)
            if comment:
                issue.comments.create(text=comment, summonees=[duty_login])
            if CONFIG_STATUS == "active":
                # сменили куку на любом сервисе yandex.ru, то призвать Дзен и добавить в наблюдатели
                if new_yandex_cookie:
                    issue = st_client.issues[issue.key]
                    zen_followers = ZEN_FOLLOWERS
                    issue.update(followers=issue.followers + zen_followers)
                    issue.comments.create(text="Поменяйте пожалуйста куку на {}".format(new_yandex_cookie), summonees=zen_followers)
                # если проверили все сервисы, то закроем тикет
                issue = st_client.issues[issue.key]
                if not not_fixed_services:
                    # если тикет в работе, его нужно решить, а затем закрыть
                    if issue.status.key == 'inProgress':
                        issue.transitions['resolve'].execute()
                        issue = st_client.issues[issue.key]
                        transition = issue.transitions['close']
                        transition.execute(resolution='fixed')
                    # иначе просто закрываем
                    else:
                        transition = issue.transitions['closed']
                        transition.execute(resolution='fixed')
                    logger.info("Ticket {} closed".format(issue.key))
                # иначе обновим компоненты
                else:
                    components = {int(component.id) for component in issue.components if component.name in not_fixed_services}
                    issue.update(components=sorted(components))
        except NotFound as ex:
            logger.exception(ex.message)
            logger.info("Status: {}".format(issue.status.key))
            transitions = ", ".join(str(transition) for transition in issue.transitions.get_all())
            logger.info("Transitions: {}".format(transitions))
    if exception is None:
        logger.info("Task has been successfully completed")
    else:
        raise exception
