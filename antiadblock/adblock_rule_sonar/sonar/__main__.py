#!/usr/bin/env python
# encoding=utf8
import argparse
from os import getenv
from re import compile

from antiadblock.adblock_rule_sonar.sonar.lib.config import CONFIG
from antiadblock.adblock_rule_sonar.sonar.lib.utils.parser import ParserNewRules
from antiadblock.adblock_rule_sonar.sonar.lib.utils.sonar_yt import SonarYT
from antiadblock.adblock_rule_sonar.sonar.lib.utils.sonar_startrek import create_tickets
from antiadblock.adblock_rule_sonar.sonar.lib.utils.sonar_logger import logger
from antiadblock.adblock_rule_sonar.sonar.lib.utils.sonar_configs_api import get_service_ids_and_current_cookies


def get_args():
    parser = argparse.ArgumentParser(description='AdBlock Sonar. Tool for adblock rule parsing.')
    parser.add_argument('--yt_token',
                        metavar='TOKEN',
                        help='Using yt for storing new rules and getting current active. Expects oAuth token for YT as value')
    parser.add_argument('--send_rules',
                        action='store_true',
                        help='If this enables sonar will send rules to yt. USE ONLY FOR SANDBOX RUNS')
    parser.add_argument('--create_tickets',
                        action='store_true',
                        help='If this enables sonar will create tickets in StarTrack about cookie-remover rules. USE ONLY FOR SANDBOX RUNS')
    parser.add_argument('--domain',
                        help='This option allows to search rules for single domain. Expects domain name or regexp',
                        default=None)
    parser.add_argument('--list', help='This option allows to search rules in single list. Expects list url', default=None)
    parser.add_argument('--local_run', action='store_true', help='For local run')
    return parser.parse_args()


def init_configuration(configurations, domain, yt_token, do_create_tickets, is_local_run=False):
    if domain is not None:
        configurations['search_regexps'] = {domain: [domain]}

    if not is_local_run or yt_token:
        configurations['yt']['token'] = yt_token or getenv('YT_TOKEN', None)
        assert configurations['yt']['token']

    if do_create_tickets:
        configurations['startrek']['token'] = getenv('STARTREK_TOKEN', None)


def filter_rules_from_blacklist(rules, blacklist_email_notification_rules):
    """
    :param blacklist_email_notification_rules
    :param rules -- rules[new] = {partner_rules=[], general_rules=[]}
                    rules[obsolete] = {general_rules=[], yandex_related_rules=[]}
    фильтруем все четыре массива new_partner_rules, new_general_rules,
                                 obsolete_general_rules, obsolete_yandex_related_rules
    на наличие правил по которым не надо нотифицировать в письме
    :return:
    """
    exclude_rules = compile(r'.*(?:' + r'|'.join(blacklist_email_notification_rules) + ')')
    for new_or_obsolete_rules in rules.values():
        for type_of_rules in new_or_obsolete_rules:
            new_or_obsolete_rules[type_of_rules] = filter(lambda x: exclude_rules.match(str(x)) is None, new_or_obsolete_rules[type_of_rules])


def get_file_content_to_send(rules, service_ids):
    rules_list = []
    partner_rules = filter(lambda r: r.partner in service_ids and not r.is_cookie_remove_rule, rules['new']['partner_rules'])
    if partner_rules:
        rules_list += ['!NEW PARTNER RULES:'] + map(repr, partner_rules) + ['']
    new_yandex_related_rules = filter(lambda r: r.is_yandex_related and not r.is_cookie_remove_rule, rules['new']['general_rules'])
    if new_yandex_related_rules:
        rules_list += ['!NEW YANDEX RELATED GENERAL RULES:'] + map(repr, new_yandex_related_rules) + ['']
    if rules['obsolete']['yandex_related_rules']:
        rules_list += ['!DELETED PARTNER AND YANDEX RELATED GENERAL RULES:'] + map(str, rules['obsolete']['yandex_related_rules']) + ['']
    if rules['new']['general_rules']:
        rules_list += ['!NEW GENERAL RULES:'] + map(repr, rules['new']['general_rules']) + ['']
    if rules['obsolete']['general_rules']:
        rules_list += ['!DELETED RULES:'] + map(str, rules['obsolete']['general_rules']) + ['']

    # устанавливаем важность письма, если есть правила что могут аффектить нас и/или партнеров
    priority = 'urgent' if partner_rules or new_yandex_related_rules or rules['obsolete']['yandex_related_rules'] else 'normal'
    rules_list.insert(0, priority)
    file_content = '\n'.join(rules_list)
    return file_content


def send_rules(rules, configurations, service_ids):

    if configurations['blacklist_email_notification_rules']:
        filter_rules_from_blacklist(rules, configurations['blacklist_email_notification_rules'])

    file_content = get_file_content_to_send(rules, service_ids)
    logger.info('Changes since last run:')
    logger.info(file_content)

    with open('new_rules.txt', 'w') as f:  # File contains email message that Sandbox sends.
        f.write(file_content)


if __name__ == "__main__":
    args = get_args()

    configuration = CONFIG
    init_configuration(configuration, args.domain, args.yt_token, args.create_tickets)
    service_ids, cookies_of_the_day = get_service_ids_and_current_cookies()
    logger.debug(service_ids)
    logger.debug(cookies_of_the_day)

    syt = SonarYT(configuration=configuration['yt'], is_local_run=args.local_run)

    if args.list:
        adblock_sources = {'custom': [args.list]}
        logger.info('Using {} as custom adblock list'.format(args.list))
    else:
        adblock_sources = syt.fetch_adblock_sources()
        logger.info('Fetched current adblock sources')

    parsed_rules = dict()
    rules_parser_from_lists = ParserNewRules(configuration=configuration, known_rules=syt.known_rules,
                                             cookies_of_the_day=cookies_of_the_day, adblock_sources=adblock_sources)
    parsed_rules.update({'new': rules_parser_from_lists.parse_new_rules()})
    parsed_rules.update({'obsolete': syt.get_obsolete_rules(configuration)})

    syt.delete_rules(parsed_rules['obsolete'])
    if args.send_rules:
        send_rules(parsed_rules, configuration, service_ids)
    else:
        logger.info(get_file_content_to_send(parsed_rules, service_ids))
    syt.add_partner_rules(parsed_rules['new']['partner_rules'])
    syt.add_general_rules(parsed_rules['new']['general_rules'])

    if args.create_tickets:
        cookie_remove_rules = rules_parser_from_lists.get_cookie_remove_rules()
        if cookie_remove_rules:
            logger.info("{} new cookie-remover rules found".format(','.join(map(repr, cookie_remove_rules))))
            create_tickets(cookie_remove_rules, configuration['startrek'])
        else:
            logger.info('No new cookie-remover rules found')
