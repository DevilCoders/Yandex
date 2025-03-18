# encoding=utf8
import re
from collections import namedtuple, defaultdict

import requests
from retry import retry

from antiadblock.adblock_rule_sonar.sonar.lib.utils.rule_parser import Filter, parse_filterlist, SelectorType, \
    FilterAction, FilterOption
from antiadblock.adblock_rule_sonar.sonar.lib.utils.sonar_logger import logger


RuleHash = namedtuple('RuleHash', ['short_rule', 'list_url', 'options'])
RuleHash.__str__ = lambda self: '{r.short_rule} {r.options}\nURL: {r.list_url}'.format(r=self)
SPLITTER_SYMBOL = '-'


def is_this_partner_by_rulehash(rule_hash):
    """
    Так как short_rule объявляется как
    u'-'.join([self.partner, self.action, self.type, self.value])
    Соответственно если правило partner, то short_rule не начинается с '-'
    Добавляется отдельно функция для удобства изменений
    :param rule_hash:
    :return:
    """
    return not str(rule_hash).startswith(SPLITTER_SYMBOL)


class ParsedRule(object):

    def __init__(self, domains, partner, url, rule_type, value, action, raw_rule, is_yandex_related=False, options=None,
                 adblock_name=None, is_cookie_remove_rule=False):
        """
            :param domains: Domains affected by rule
            :param partner: Partner config.search_regexps.keys()
            :param url: Adblock source list url
            :param rule_type: Type of rule (block-url/ememhide/script/etc)
            :param value: Value of rule without domains ands options
            :param action: block | allow
            :param raw_rule: Row from source list
            :param is_yandex_related: rule can affect any yandex or yandex's partners sites
            :param options: Additional rule options (such as popup etc)
            :param adblock_name: Adblock name
            :param is_cookie_remove_rule:
        """
        self.adblock_name = adblock_name
        self._domains = domains
        self.partner = partner
        self.url = url
        self.type = rule_type
        self.value = value.replace('\\"', '"')
        self.action = action
        self.raw_rule = raw_rule.encode('utf-8')
        self.is_general = not domains
        self.is_yandex_related = is_yandex_related
        self.is_cookie_remove_rule = is_cookie_remove_rule
        self._options = options or dict()

    def __str__(self):
        return SPLITTER_SYMBOL.join([self.partner, self.action, self.type, self.value]).encode('utf-8')

    def __repr__(self):
        return u'{rule.partner} {rule.action}-{rule.type}: {rule.value} {rule.options}\nURL: {rule.url}'.\
            format(rule=self).encode('utf-8')

    @property
    def short_rule(self):
        return u'{rule.action}-{rule.type}: {rule.value} {rule.options}'.format(rule=self).encode('utf-8')

    @property
    def domains(self):
        return ','.join(self._domains)

    @property
    def options(self):
        return ' '.join(['{}: {}'.format(opt, val.encode('utf-8') if isinstance(val, unicode) else val) for (opt, val) in sorted(self._options.items(), key=lambda item: item[0])]).decode('utf-8')

    @property
    def general(self):
        return str(self.is_general).lower()

    @property
    def yandex_related(self):
        return str(self.is_yandex_related).lower()

    @property
    def hash(self):
        return RuleHash(short_rule=str(self), list_url=self.url, options=self.options.encode('utf-8'))


class ParserNewRules(object):

    def __init__(self, configuration, known_rules, cookies_of_the_day, adblock_sources):
        self.new_rules = defaultdict(list)
        self.configuration = configuration
        # Предварительно компилируем регулярки для определения id партнера для правила
        # Create big regexp from all partner regexp
        # () - for grouping matches
        # ^$ for strict search e.g ^yandex.ru$ will not match afisha.yandex.ru
        self.partners_regexps = get_partners_regexps(configuration)
        # Делаем большую регулярку из регулярок доменов партнера и yandex_related_rules регулярки
        # префикс (?<![-\w]) и суфикс [^\-\w] отсекают ложные срабатывания, например, ложный матчинг auto.ru
        # на vse-auto.ru, до и после домена ожидаются любые разделители
        self.yandex_related_regexps = get_yandex_related_pattern(configuration)
        self.cookies_of_the_day = cookies_of_the_day
        logger.debug("Current cookies of the day used to check: {}".format(', '.join(self.cookies_of_the_day)))
        self.known_rules = known_rules
        self.adblock_sources = adblock_sources

    def parse_new_rules(self):
        for adblock_name, filters_lists in self.adblock_sources.items():
            for list_url in filters_lists:
                logger.debug('Parsing rules-list for `{}` from url: {}'.format(adblock_name, list_url))

                filter_list = request_list(list_url, token=self.configuration['yt']['token'] if adblock_name == 'Internal' else None)
                for rule in parse_filterlist(filter_list):
                    if not isinstance(rule, Filter):
                        continue
                    self.init_rule(rule, adblock_name, list_url)
        return self.new_rules

    def init_rule(self, rule, adblock_name, list_url):
        rule_domains_included, rule_domains_excluded = get_rule_domains_affected(rule, self.partners_regexps)
        is_cookie_remove_rule = is_yandex_cookie_remove_filter(rule, adblock_name, self.cookies_of_the_day)
        if rule_domains_included:
            for partner, search_regexp in self.partners_regexps:
                domains_blocked = search_regexp.findall(rule_domains_included)
                domains_unblocked = search_regexp.findall(rule_domains_excluded)
                add_new_rule = True if domains_blocked else False
                if domains_blocked and domains_unblocked:
                    # проверяем на то, что в domains_unblocked есть хотя бы один домен
                    # который разблокирует текущий domain_block
                    for domain_block in domains_blocked:
                        add_new_rule = not any(domain.endswith('.' + domain_block) for domain in domains_unblocked)
                        if add_new_rule:
                            break
                if add_new_rule:
                    new_rule = ParsedRule(domains=domains_blocked, partner=partner, url=list_url,
                                          rule_type=rule.selector['type'], value=rule.selector['value'],
                                          action=rule.action, raw_rule=rule.text, options=dict(rule.options),
                                          adblock_name=adblock_name)
                    self.add_new_rules(new_rule, is_cookie_remove_rule)
        else:
            new_rule = ParsedRule(partner='', domains=None, url=list_url, rule_type=rule.selector['type'],
                                  value=rule.selector['value'], action=rule.action, raw_rule=rule.text,
                                  options=dict(rule.options), adblock_name=adblock_name)
            self.add_new_rules(new_rule, is_cookie_remove_rule)

    def add_new_rules(self, new_rule, is_cookie_remove_rule):
        if new_rule.hash not in self.known_rules:
            logger.warning('Found new{} rule: \n{} -> \n{}'.format(' general' if new_rule.is_general else '',
                                                                   new_rule.raw_rule, str(new_rule)))
            new_rule.is_cookie_remove_rule = is_cookie_remove_rule
            new_rule.is_yandex_related = self.yandex_related_regexps.search(new_rule.raw_rule) is not None
            if new_rule.is_general:
                self.new_rules['general_rules'].append(new_rule)
            else:
                self.new_rules['partner_rules'].append(new_rule)
        else:
            self.known_rules[new_rule.hash].is_known_rule = True    # rule presence in list validated

    def get_cookie_remove_rules(self):
        return filter(lambda r: r.is_cookie_remove_rule,
                      self.new_rules['partner_rules'] + self.new_rules['general_rules'])


def get_yandex_related_pattern(configuration):
    """
    Нужен для того, чтобы можно было проверить, что правило является is_yandex_related
    (например в sonar_yt для проверки удаленных правил)
    :param configuration:
    :return:
    """
    global_partner_regex = r'(?<![-\w])(?:' + r'|'.join(
        sum(configuration['search_regexps'].values(), [])) + r')(?:[^\-\w]|$)'
    return re.compile(r'|'.join(configuration['yandex_related_rules'] + [global_partner_regex]))


def get_partners_regexps(configuration):
    return [(partner, re.compile(r'^(?:' + r'|'.join(search_regexps) + r')$', re.M))
            for partner, search_regexps in configuration['search_regexps'].items()]


def get_domain_from_selector_value(selector_value):
    match = re.match(r"\|\|([\d\w\.\-\*]+\.[\d\w\.\-\*]+)[\^/]*", selector_value.decode('utf-8'))
    try:
        return match.groups()[0]
    except Exception as e:
        logger.info("Failed match domain {}\n".format(selector_value) + str(e))


def get_rule_domains_affected(rule, partners_regexps):
    """
        :rule: Filter()
        Filter.domains = [(u'domain1', True), (u'domain2', True), (u'domain3', False) ... ]
        :return 'domain1\ndomain2'
    """
    def www_remover(domain):
        return domain[:4].replace('www.', '') + domain[4:]

    include_domains = []
    exclude_domains = []
    for new_domain in rule.domains:
        if new_domain[1]:
            include_domains.append(www_remover(new_domain[0]))
        else:
            exclude_domains.append(www_remover(new_domain[0]))

    def is_cookie_remove_rule(_rule):
        for item in _rule.options:
            if item[0] == FilterOption.COOKIE:
                return True
        return False

    if rule.selector.get("type") == SelectorType.URL_PATTERN and rule.action == FilterAction.BLOCK \
            and is_cookie_remove_rule(rule):
        domain = get_domain_from_selector_value(rule.selector["value"])
        if domain:
            for partner, search_regexp in partners_regexps:
                include_domains.extend(search_regexp.findall(domain))
    return '\n'.join(include_domains), '\n'.join(exclude_domains)


def is_yandex_cookie_remove_filter(rule, adblock_type, current_cookies):
    try:
        current_cookies = '\n'.join(current_cookies)
        # чтоб исключить лишние проверки будем проверять только в адгард и ублок
        if adblock_type in ['Ublock', 'Adguard']:
            # вариант1 - один из сниппетов вырезания кук
            cookies_for_remove = None
            if rule.selector['type'] in [SelectorType.SNIPPET, SelectorType.ADGUARD_SCRIPT]:
                # language=RegExp
                extract_cookies_re = re.compile(r'(?:\+js\(cookie-remover(?:\.js)?,|//scriptlet\([\'\"]remove-cookie[\'\"],|AG_removeCookie\()\s*[\'\"]?/(\S*?)/[\'\"]?\s*\)')
                cookies_for_remove = extract_cookies_re.match(rule.selector['value'])

            # вариант2 - опция cookie в правиле, к сожалению они впихнули это в URL_PATTERN правило
            cookies_option = None
            if rule.selector['type'] == SelectorType.URL_PATTERN and adblock_type == 'Adguard':
                cookies_option = [opt[1] for opt in rule.options if opt[0] == 'cookie']

            if cookies_for_remove is not None:
                cookies_for_remove_pattern = cookies_for_remove.group(1)
            elif cookies_option:
                cookies_for_remove_pattern = cookies_option[0]
            else:
                return False

            # в регулярках на куки заэкранированы концы строк($), снимаем экранирование
            cookies_for_remove_re = re.compile(cookies_for_remove_pattern.replace(r'\$', '$'), re.M)
            match = cookies_for_remove_re.search(current_cookies)

            return match is not None
        return False
    except:
        logger.error("Exception while parsing cookie from rule: {}".format(rule.text))
        return False


@retry(tries=3, delay=10)
def request_list(list_url, token=None):

    headers = {'Accept': 'application/json', 'Authorization': 'OAuth {}'.format(token)} if token is not None else {}
    # Нужно обрабатывать редиректы так как, запрос на hahn за внутрияндексовыми правилами редиректится некорректно
    # Это приводит к появлению ответа с сообщением 'User "guest" is banned'.
    session = requests.Session()
    session.max_redirects = 10
    try:
        response = session.get(list_url, headers=headers)
    except requests.TooManyRedirects:
        logger.error('Error while requesting list {}. Too many redirects'.format(list_url))
        raise
    else:
        assert response.status_code == 200
    finally:
        session.close()
    if response.url != list_url:
        logger.warning("Final list url is '{}'".format(response.url))
    return response.text.encode('utf-8').splitlines()
