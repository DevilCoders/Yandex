# coding=utf-8

from collections import defaultdict
import yt.wrapper as yt


class SonarClient(object):

    def __init__(self, oauth_token, yt_client=None):
        self.yt_client = yt_client or yt
        self.yt_client.config['proxy']['url'] = "hahn"
        self.yt_client.config['token'] = oauth_token
        self.DIRECTORY = "//home/antiadb/sonar"

    @staticmethod
    def filter_and_group_by_rules(rules, allowed_sources=None):
        def modify_dict(schema, current):
            return {
                'schema': schema,
                'data': {
                    'items': current,
                    'total': len(current)
                }
            }

        map_rules = defaultdict(lambda *_: {'list_url': set(), 'added': '', 'is_partner_rule': False})
        for item in rules:
            if allowed_sources is not None and item['list_url'] not in map(lambda r: r['list_url'], allowed_sources):
                continue
            rule = item['raw_rule'].strip()
            map_rules[rule]['list_url'].add(item['list_url'])
            map_rules[rule]['added'] = max(item['added'], map_rules[rule]['added'])
            map_rules[rule]['is_partner_rule'] = item.get('is_partner_rule', False)

        results = []
        for key, value in map_rules.items():
            results.append({
                'raw_rule': key,
                'list_url': list(value['list_url']),
                'added': value['added'],
                'is_partner_rule': value['is_partner_rule']
            })

        results.sort(reverse=True, key=lambda elem: elem['added'])
        schema = dict(raw_rule="raw-rule", added="added", list_url="list-url", is_partner_rule="is-partner-rule")
        return modify_dict(schema, results[:100])

    def _get_all_rules(self, service_id):
        rules = list(self.yt_client.select_rows('raw_rule, list_url, added FROM [{}/sonar_rules] '
                                                'WHERE partner = "{}"'.format(self.DIRECTORY, service_id)))
        ya_rules = list(self.yt_client.select_rows('raw_rule, list_url, added FROM[{}/general_rules] '
                                                   'WHERE is_yandex_related = true'.format(self.DIRECTORY)))
        for rule in rules:
            rule['is_partner_rule'] = True
        rules.extend(ya_rules)
        return rules

    def get_partner_rules(self, service_id):
        rules = list(self.yt_client.select_rows('raw_rule, list_url, added FROM [{}/sonar_rules] '
                                                'WHERE partner = "{}"'.format(self.DIRECTORY, service_id)))
        for rule in rules:
            rule['is_partner_rule'] = True
        return rules

    def get_rules_by_service_id(self, service_id, tags):
        sources = self._get_adblock_sources_with_tags(tags)
        return self.filter_and_group_by_rules(self._get_all_rules(service_id), allowed_sources=sources)

    def _get_adblock_sources_with_tags(self, tags):
        sources = self.yt_client.select_rows('list_url, adblocker, tags FROM [{}/adblock_sources]'.format(self.DIRECTORY))
        return list(filter(lambda source: all(tag in source['tags'] for tag in tags), sources))
