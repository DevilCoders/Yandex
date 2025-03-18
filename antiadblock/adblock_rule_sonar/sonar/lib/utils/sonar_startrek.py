# encoding=utf8
from collections import defaultdict

import requests
from startrek_client import Startrek

from antiadblock.adblock_rule_sonar.sonar.lib.utils.sonar_logger import logger

TICKET_TITLE = 'New Adblock Rule For Cookie Of The Day'
ISSUE_TAG = "cookie_rule"
ABC_ANTIADBLOCK_SUPPORT_PERSONS = "https://abc-back.yandex-team.ru/api/v4/services/members/?service__slug=antiadblock&role__scope=support"


def create_tickets(rules, config):
    # 1. Собираем описание тикета для всех вышедших правил
    # 2. Ищем открытый тикет, если нашли, то обновляем компоненты и заканчиваем на этом
    # 3. Ищем закрытый тикет, если нашли, то перезаписываем компоненты и заканчиваем на этом
    # 4. Заводим новый тикет

    def get_components_for_services(services):
        """
        :param services: set of service_ids
        :return: set of component_ids
        """
        return {component['id'] for component in st_client.queues[config['queue']].components if component['name'] in services}

    st_client = Startrek(useragent='python', token=config['token'])

    # объединим правила в dict "{short_rule: {partners: {partners}, adblocks: {adblocks}}"
    _rules = defaultdict(lambda: {"partners": set(), "adblocks": set()})
    for rule in rules:
        _rules[rule.short_rule]["partners"].add(rule.partner)
        _rules[rule.short_rule]["adblocks"].add(rule.adblock_name)

    # сформируем описание для тикета и множество всех компонентов
    _partners = set()
    _description = []
    for short_rule, values in _rules.items():
        _partners.update(values["partners"])
        _description.append("New cookie-remover rule in {} : %%{}%%".format(", ".join(sorted(values["adblocks"])), short_rule))
    description = "\n\n".join(sorted(_description))
    description += '\n\n! Please do not edit the ticket description. !'
    components = get_components_for_services(_partners)

    # получить все открытые тикеты из очереди ANTIADBSUP по тэгу
    open_issues = st_client.issues.find(
        filter={
            "queue": config['queue'],
            "resolution": "empty()",
            "tags": [ISSUE_TAG]
        }
    )
    # если среди тикетов есть с описанием, которое содержит правила и он открыт, то обновим у него компоненты
    open_issue = next((issue for issue in open_issues if description == issue.description), None)
    if open_issue:
        logger.info('Rules already exist in issue {}'.format(open_issue.key))
        # update components if needed
        issue_components = {int(component.id) for component in open_issue.components}
        if not components.issubset(issue_components):
            logger.info("Update components in issue {}".format(open_issue.key))
            open_issue.update(components=sorted(issue_components | components))
        return

    # получить все закрытые тикеты по тэгу
    closed_issues = st_client.issues.find(
        filter={
            "queue": config['queue'],
            "resolution": 'fixed()',
            "tags": [ISSUE_TAG]
        },
        # сортировка по убыванию
        order=['-created']
    )
    # смотрим на последний тикет и переоткрываем его с комментом что заново написано
    closed_issue = next((issue for issue in closed_issues if description == issue.description), None)
    if closed_issue:
        # update components
        closed_issue.update(components=sorted(components))
        transition = closed_issue.transitions['reopen']
        comment = 'The rule has been released again'
        transition.execute(comment=comment)
        logger.info('Ticket {} reopened'.format(closed_issue.key))
        return

    # Заводим один тикет на новые правила
    logger.warning(description)
    # получаем суппортеров сервиса
    supporters = requests.get(ABC_ANTIADBLOCK_SUPPORT_PERSONS, headers={'Authorization': 'OAuth {}'.format(config['token'])})
    supporters_logins = [p['person']['login'] for p in supporters.json()['results']]
    new_issue = st_client.issues.create(
        queue=config['queue'],
        summary=TICKET_TITLE,
        followers=supporters_logins,
        description=description,
        tags=[ISSUE_TAG],
        components=sorted(components),
    )
    logger.info('Created ticket {}'.format(new_issue.key))
