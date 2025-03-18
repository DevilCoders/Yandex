# coding=utf-8
from antiadblock.adblock_rule_sonar.sonar.lib.utils.sonar_logger import logger

YANDEX_TLDS = r'(?:ru|ua|kz|by|kg|lt|lv|md|tj|tm|uz|ee|az|fr|com|com\.tr|com\.am|com\.ge|co\.il|\*)'
# регулярка на yandex.tld чтоб добавлять ее партнерам которые находятся так или иначе на yandex.ru, так как правила могут действовать на них
YANDEX_MORDA_REGEX = r'(?:www\.)?yandex\.{tld}'.format(tld=YANDEX_TLDS)
YANDEX_MORDA_REGEX_WITHOUT_WWW = r'yandex\.{tld}'.format(tld=YANDEX_TLDS)
QUEUE_NAME = 'ANTIADBSUP'


def _load_config():
    """
    Loading config merging env vars and local default config.
    Config can be reload with different env vars during tests, so we use this func to handle this.
    :return: dict with config for sonar
    """
    logger.debug('Loading sonar config')

    config = dict(
        search_regexps={
            'autoru': [r'auto\.(?:ru|\*)'],
            'yandex_pogoda': [
                r'(?:yandex\.{tld})?.*?/(?:weather|pogoda|hava)/'.format(tld=YANDEX_TLDS),  # TODO: usless regular - rules with $domain=yandex.ru/pogoda does not exist
                r'/(?:weather|pogoda|hava)/.*?(?:yandex\.{tld})?'.format(tld=YANDEX_TLDS),
                YANDEX_MORDA_REGEX,
            ],
            'sdamgia': [r'(?:[\w-]+\.)?sdamgia\.(?:ru|\*)', r'(?:[\w-]+\.)?reshu[eo]ge\.(?:ru|\*)'],
            'smi24': [r'24smi\.(?:org|\*)'],
            'razlozhi': [r'razlozhi\.(?:ru|\*)'],
            'kakprostoru': [r'kakprosto\.(?:ru|\*)'],
            'otzovik': [r'(?:[\w-]+\.)?otzovik\.(?:com|\*)'],
            'yandex_tv': [r'tv\.yandex\.{tld}'.format(tld=YANDEX_TLDS), YANDEX_MORDA_REGEX_WITHOUT_WWW],
            'drive2': [r'drive2\.(?:ru|\*)'],
            'gorod_rabot': [r'(?:[\w-]+\.)?gorodrabot\.(?:ru|by|\*)'],
            'gismeteo': [r'(?:(?:www|beta)\.)?gismeteo\.(?:ru|by|com|\*)'],
            'yandex_mail': [r'mail\.yandex\.{tld}'.format(tld=YANDEX_TLDS), YANDEX_MORDA_REGEX_WITHOUT_WWW],
            'yandex_afisha': [r'afisha\.yandex\.{tld}'.format(tld=YANDEX_TLDS), YANDEX_MORDA_REGEX_WITHOUT_WWW],
            'games.yandex.ru': [YANDEX_MORDA_REGEX],
            'yandex_news': [YANDEX_MORDA_REGEX],
            'turbo.yandex.ru': [r'(?:www\.)?turbopages\.(?:org|\*)', YANDEX_MORDA_REGEX],
            'zen.yandex.ru': [r'zen\.yandex\.{tld}'.format(tld=YANDEX_TLDS),
                              YANDEX_MORDA_REGEX],  # на виджет на морде действуют все правила морды
            'docviewer.yandex.ru': [r'docviewer\.yandex\.{tld}'.format(tld=YANDEX_TLDS), YANDEX_MORDA_REGEX_WITHOUT_WWW],
            'yandex_realty': [r'(?:m\.)?realty\.yandex\.{tld}'.format(tld=YANDEX_TLDS), YANDEX_MORDA_REGEX_WITHOUT_WWW],
            'liveinternet': [r'(?:www\.)?liveinternet\.(?:ru|\*)'],
            # топовые домены ЖЖ
            'livejournal': [r'(?:[\w-]+\.)?livejournal\.(?:com|net|\*)',
                            r'(?:[\w-]+\.)?lena\-miro\.(?:ru|\*)',
                            r'(?:[\w-]+\.)?levik\.(?:blog|\*)',
                            r'(?:[\w-]+\.)?olegmakarenko\.(?:ru|\*)'],
            'yandex_morda': [YANDEX_MORDA_REGEX],
            'yandex_player': [r'(?:www\.)?yastatic\.net'],
            'kinopoisk.ru': [r'(?:\w+\.)*kinopoisk\.(?:ru|\*)'],
            'nova.rambler.ru': [r'nova\.rambler\.(?:ru|\*)'],
            'sm.news': [r'(?:[\w-]+\.)?sm\.(?:news|\*)'],
            'spletnik.ru': [r'(?:[\w-]+\.)?spletnik\.(?:ru|\*)'],
        },
        blacklist_email_notification_rules=[  # Правила сматчавшиеся на short_rule регулярки не будут отправлены в письме сонара
            # При добавлении сюда исключений помните!!! что это регулярка и многие спецсимволы типа вертикальной черты(|) надо экранировать !!!
            r"\|\|threepercenternation\.com/[\w-]+/",
        ],
        yt={
            # https://staff.yandex-team.ru/robot-antiadb
            'token': None,
            'cluster': 'hahn',
            'quota': '//home/antiadb/sonar/',
        },
        startrek={
            'token': None,
            'queue': QUEUE_NAME,
        },
        # может касаться рекламных систем яндекса, а также наших безкуковых доменов
        yandex_related_rules=[
            r'yandex|awaps|yastatic|[aA]d[fF]ox|\bYa\b|(?:naydex|clstorage|static\-storage|cdnclab)\.net',
        ],
        cookies_of_the_day={
            'url': 'https://proxy.sandbox.yandex-team.ru/last/ANTIADBLOCK_ALL_COOKIES',
            'extended': '',  # расширение списка кук, вдруг понадобится + в тестах нужно. Формат: "кука\nкука2\nкука3"
        },
    )

    return config


CONFIG = _load_config()
