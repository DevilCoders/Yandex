# encoding=utf8
import datetime
from collections import defaultdict

import yt.wrapper as yt

from antiadblock.adblock_rule_sonar.sonar.lib.config import CONFIG
from antiadblock.adblock_rule_sonar.sonar.lib.utils.sonar_logger import logger
from antiadblock.adblock_rule_sonar.sonar.lib.utils.parser import RuleHash, get_yandex_related_pattern, \
    is_this_partner_by_rulehash


class RuleInfo(object):

    def __init__(self, is_known_rule=False, raw_rule=''):
        self.is_known_rule = is_known_rule
        self.raw_rule = raw_rule


class SonarYT(object):

    def __init__(self, configuration=None, yt_client=None, is_local_run=False):
        if configuration is None:
            configuration = CONFIG['yt']
        self.yt_client = yt_client or yt
        self.configuration = configuration
        self.CLUSTER = configuration['cluster']
        self.TOKEN = configuration['token']
        # Rules already present in any table in YT
        self.known_rules = defaultdict(RuleInfo)

        self.QUOTA = configuration.get('quota', '//tmp/')
        self.GENERAL_RULES_TABLE = self.QUOTA + 'general_rules'
        self.RULES_TABLE = self.QUOTA + 'sonar_rules'
        self.SOURCES_TABLE = self.QUOTA + 'adblock_sources'

        self.yt_client.config['proxy']['url'] = self.CLUSTER
        self.yt_client.config['token'] = self.TOKEN
        self.is_local_run = is_local_run

        if self.is_local_run and self.TOKEN is None:
            logger.warning('Cannot connect to YT, because is_local_run is true.')
            return

        logger.info('Loading known rules from YT')
        try:
            # encode_utf8=false
            # В этом режиме при конверсии из YSON-строк в JSON-строки мы предполагаем, что в YSON-строке лежит валидная UTF8-последовательность и раскодируем ее в юникодную JSON-строку.
            # Иначе, мы каждый байт переводим в юникодный символ с соответствующим номером и кодируем его в UTF-8, что порождает кракозябры.
            for table in [self.RULES_TABLE, self.GENERAL_RULES_TABLE]:
                for row in self.yt_client.read_table(
                        self.yt_client.TablePath(table, columns=['short_rule', 'list_url', 'options', 'raw_rule']),
                        format=self.yt_client.JsonFormat(encoding="utf-8")):
                    rule_hash_obj = RuleHash(short_rule=row['short_rule'].encode('utf-8'),
                                             list_url=row['list_url'].encode('utf-8'),
                                             options=row['options'].encode('utf-8'))
                    self.known_rules.update({
                        rule_hash_obj: RuleInfo(is_known_rule=False, raw_rule=row['raw_rule'].encode('utf-8'))
                    })
        except Exception as e:
            logger.error('Cannot read table {} while getting known rules'.format(table))
            logger.error(e.message)
            raise
        logger.info('Known rules loaded from YT')

    def _insert_rows(self, table, rows):
        if self.is_local_run:
            return
        try:
            self.yt_client.insert_rows(table, rows, raw=False, format=yt.JsonFormat(encoding="utf-8"))
        except Exception as e:
            logger.error('Cannot insert into table {}'.format(table))
            logger.error(e.message)
            raise
        else:
            logger.info('Added {} rules into table {}'.format(len(rows), table))

    def _delete_rows(self, table, rows):
        if self.is_local_run:
            return
        try:
            self.yt_client.delete_rows(table, rows, format=yt.JsonFormat(encoding="utf-8"))
        except Exception as e:
            logger.error('Error. Cannot delete from table {}'.format(table))
            logger.error(e.message)
            raise
        else:
            logger.info('Removed {} rules from table {}'.format(len(rows), table))

    def add_partner_rules(self, rules):
        partner_rules = [{'partner': rule.partner,
                          'domains': rule.domains,
                          'list_url': rule.url,
                          'short_rule': str(rule),
                          'added': datetime.datetime.today().isoformat(),
                          'type': rule.type,
                          'value': rule.value,
                          'action': rule.action,
                          'raw_rule': rule.raw_rule,
                          'options': rule.options,
                          } for rule in rules]
        self._insert_rows(self.RULES_TABLE, partner_rules)

    def add_general_rules(self, rules, chunk_size=1000):
        general_rules = [{'list_url': rule.url,
                          'short_rule': str(rule),
                          'added': datetime.datetime.today().isoformat(),
                          'type': rule.type,
                          'value': rule.value,
                          'action': rule.action,
                          'raw_rule': rule.raw_rule,
                          'options': rule.options,
                          'is_yandex_related': rule.is_yandex_related,
                          } for rule in rules]

        for i in range(0, len(general_rules), chunk_size):
            self._insert_rows(self.GENERAL_RULES_TABLE, general_rules[i:i+chunk_size])

    # also return OBSOLETE_GENERAL_YANDEX_RELATED_RULES_AND_PARTNER_RULES in second param
    def get_obsolete_rules(self, config_for_search_yandex_related_by_regexp):
        yandex_related_pattern = get_yandex_related_pattern(config_for_search_yandex_related_by_regexp)
        obsolete_rules = [r for r, in_list in self.known_rules.items() if not in_list.is_known_rule]

        obsolete_yandex_related_rules = [rule_hash for rule_hash in obsolete_rules if is_this_partner_by_rulehash(rule_hash) or
                                         yandex_related_pattern.search(self.known_rules[rule_hash].raw_rule) is not None]
        obsolete_rules = [x for x in obsolete_rules if x not in obsolete_yandex_related_rules]
        return dict(general_rules=obsolete_rules, yandex_related_rules=obsolete_yandex_related_rules)

    def delete_rules(self, obsolete_rules):
        partner_rows_to_delete = []
        general_rows_to_delete = []
        for rule in obsolete_rules['yandex_related_rules']:
            if is_this_partner_by_rulehash(rule):
                partner_rows_to_delete.append({
                    'short_rule': rule.short_rule,
                    'list_url': rule.list_url,
                    'options': rule.options
                })
            else:
                general_rows_to_delete.append(rule)
        self._delete_rows(self.RULES_TABLE, partner_rows_to_delete)

        rows_to_delete = [{'short_rule': r.short_rule, 'list_url': r.list_url, 'options': r.options}
                          for r in obsolete_rules['general_rules'] + general_rows_to_delete]
        self._delete_rows(self.GENERAL_RULES_TABLE, rows_to_delete)

    def fetch_adblock_sources(self):
        try:
            adblock_sources = defaultdict(list)
            for row in self.yt_client.read_table(self.SOURCES_TABLE, format=self.yt_client.JsonFormat(encoding="utf-8")):
                adblock_sources[row['adblocker']].append(row['list_url'])
            return adblock_sources
        except Exception as e:
            logger.error('Cannot read table {} to fetch current list urls'.format(self.SOURCES_TABLE))
            logger.error(e.message)
            raise e
