# coding=utf-8
import enum
from datetime import timedelta


class Scales(enum.Enum):
    minute = 'i'
    hour = 'h'
    day = 'd'

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return str(self.value)


YT_CLUSTER = 'arnold'
YT_TABLES_DAILY_FMT = '%Y-%m-%d'
YT_TABLES_STREAM_FMT = '%Y-%m-%dT%H:%M:00'


MSK_UTC_OFFSET = timedelta(hours=3)
STARTREK_DATETIME_FMT = '%Y-%m-%dT%H:%M:%S'
STAT_FIELDDATE_I_FMT = '%Y-%m-%d %H:%M:00'
SOLOMON_TS_I_FMT = '%Y-%m-%dT%H:%M:00Z'
STAT_FIELDDATE_H_FMT = '%Y-%m-%d %H:00:00'
SOLOMON_TS_H_FMT = '%Y-%m-%dT%H:00:00Z'
STAT_FIELDDATE_D_FMT = '%Y-%m-%d 00:00:00'
STAT_FIELDDATE_FMT = {
    Scales.minute: STAT_FIELDDATE_I_FMT,
    Scales.hour: STAT_FIELDDATE_H_FMT,
    Scales.day: STAT_FIELDDATE_D_FMT,
}
SOLOMON_SCALES_MAP = {
    Scales.minute: '10min',
    Scales.hour: 'hour',
    Scales.day: 'day',
}

SOLOMON_PUSH_API = 'http://solomon.yandex.net/api/v2/push?'
OLD_STAT_MONEY_REPORT = "AntiAdblock/partners_money"
CHEVENT_STAT_MONEY_REPORT = "AntiAdblock/partners_money2"
UNITED_STAT_MONEY_REPORT = "AntiAdblock/partners_money_united"
MORDA_AWAPS_MONEY_REPORT = "AntiAdblock/morda_awaps_money"
MORDA_ACTIONS_REPORT = "AntiAdblock/morda_actions"
STAT_VH_REPORT = 'AntiAdblock/vh2'
STAT_DETAILED_VH_REPORT = 'AntiAdblock/vh_stats'
STAT_INAPP_REPORT = 'AntiAdblock/money_inapp'
STAT_INAPP_DETAILED_REPORT = 'AntiAdblock/money_inapp_detailed'

HOURS_DELTA = 4

YT_ANTIADB_PARTNERS_PAGEIDS_PATH = "//home/antiadb/monitorings/antiadb_partners_pageids"

STAT_TURBO_REPORT = 'AntiAdblock/turbo_money'
YT_TURBO_PAGEID_IMPID_PATH = '//home/antiadb/monitorings/turbo_pageid_impid'

STAT_GAMES_REPORT = 'AntiAdblock/games_money'

UIDS_STATE_TABLE_PATH = '//home/antiadb/uids_adb_state/default'
NO_ADBLOCK_DOMAINS_PATH = '//home/antiadb/no_adblock_domains'
NO_ADBLOCK_UIDS_PATH = '//home/antiadb/no_adblock_uids'

SERVICE_ID_TO_PAGEID_NAMES = {
    'autoru': ['auto.ru'],
    'drive2': ['www.drive2.ru', 'drive2.ru'],
    # 'gismeteo': ['gismeteo.' + root for root in ('ru', 'by', 'kz', 'md', 'com')],
    'gorod_rabot': ['gorodrabot.by', 'gorodrabot.ru'],
    'kakprostoru': ['kakprosto.ru'],
    'music.yandex.ru': ['music.yandex.ru'],
    'otzovik': ['otzovik.com'],
    'razlozhi': ['razlozhi.ru'],
    'sdamgia': ['sdamgia.ru'],
    'sm.news': ['sm.news'],
    'spletnik.ru': ['spletnik.ru'],
    'yandex_afisha': ['afisha.yandex.ru', 'm.afisha.yandex.ru'],
    'yandex_images': [prefix + 'images.yandex.' + root for prefix in ('', 'm.') for root in ('by', 'com', 'com.tr', 'kz', 'ru', 'ua', 'uz', 'com.kz')],
    'yandex_mail': ['mail.yandex.com.tr', 'mail.yandex.ru'],
    'yandex_news': ['m.news.yandex.ru', 'news.yandex.ru'],
    'yandex_pogoda': [
        'm.pogoda.yandex.ru',
        'pogoda.yandex.ru',
        'pogoda.yandex.ua',
    ],
    'yandex_realty': ['realty.yandex.ru'],
    'yandex_tv': ['m.tv.yandex.ru', 'tv.yandex.ru'],
    'yandex_sport': ['sport.yandex.ru', 'm.sport.yandex.ru'],
}
SERVICE_ID_TO_CUSTOM_PAGEID_NAMES = {
    'yandex_morda': {
        'morda_direct': [270715, 275464, 345620, 291962, 674099, 674114, 674117, 1638660, 1638690, 1638691],
        'morda_touch_direct': [674122, 674124, 674127],
        # пейджи для экспериментов на desktop
        'experiment_desktop': [
            477696, 509868, 509869, 509870, 509871, 511319, 511320, 511321, 511322, 511323, 511324, 530009, 530010, 535840,
            535841, 542751, 594355, 594356, 594357, 594359, 594360, 594361, 594366, 594367, 594368, 594371, 594372,
            477697, 509526, 509527, 509771, 509879, 509881, 511329, 511330, 511331, 527122, 530007, 530006, 530008,
            535842, 542752, 594362, 594363, 594364, 594369, 594370
        ],
        # chromenewtab
        'chrome_ntp': [508636, 508649, 526196, 674136, 674137],
        # ntp YaBro
        'yabro_ntp': [674132, 674134],
    },
    # не все пейджи привязанные к доменам yandex_video проксируются через Антиадблок
    # считать деньги будем только по тем пейджам, которые проксируются
    'yandex_video': {
        'yandex_video_direct': [48058, 474674],
        'yandex_video_mobile': [146642],
        'video': [381176],
    },
    'nova.rambler.ru': {
        'rambler_search_serp': [324760, 408562, 481611, 481614, 481615, 601520, 601526, 601527, 601528],
        'rambler_search_client_side': [159243, 294230],
        'rambler_search_adb': [455538, 444688],
    },

    'livejournal': {
        'livejournal_direct': [348677, 348679, 348680, 348681, 348687, 348688, 348689],
        'livejournal_antiadblock': [441519],
    },
    'kinopoisk.ru': {
        'kinopoisk_web': [
            120106, 124085, 124086, 127137, 128343, 130005, 139072, 139994, 139995, 140337, 142239, 142732, 142752,
            144204, 145511, 150170, 150171, 151499, 158937, 159580, 159732, 159733, 163851, 163852, 165788, 184932,
            227232, 227511, 236159, 236315, 236329, 236330, 237742, 238244, 238724, 238726, 238730, 238732, 238735,
            238736, 241835, 242764, 242802, 2547, 258359, 259601, 259701, 259773, 259856, 259864, 259865, 259869,
            260002, 260077, 260087, 260099, 260103, 260105, 260165, 260166, 260167, 260214, 260215, 260223, 260224,
            260225, 260226, 260227, 260228, 260229, 260230, 260232, 260243, 260244, 261444, 261452, 261606, 265656,
            265681, 281585, 42630, 53100, 55848, 55849, 58800, 58802
        ],
        "kinopoisk_touch": [204310]
    },
    'smi24': {
        '24smi.org': [458419, 458492, 1598226],
    },
    'liveinternet': {
        'liveinternet.ru': [125905],
    },
    'zen.yandex.ru': {
        'rtb_zen-lib_morda': [265882],
        # Дзен на мобильной морде, и трафик на Дзене, который пришёл с мобильной морды
        'zen_mobile_morda': [288427],
        'zen_without_morda': [
            216651, 238158, 277039, 290890
        ],
        'zen_video_site': [1487846],
    },
    'dzen.ru': {
        'morda_desktop': [1681979],
        'morda_touch': [1683450],
        'site': [1688615, 1693862, 1753982, 1754054],
        'video_site': [1682513],
        'news_desktop': [1684727],
        'news_touch': [1684747],
    }
}
SERVICE_ID_TO_CUSTOM_BLOCKID_NAMES = {
    'docviewer.yandex.ru': {
        'disk.yandex.ru': {
            '104220': [132, 134, 136, 138, 148]
        }
    }
}
INNER_SERVICE_IDS = [
    'autoru',
    'docviewer.yandex.ru',
    'kinopoisk.ru',
    'music.yandex.ru',
    'yandex.question',
    'yandex_afisha',
    'yandex_images',
    'yandex_mail',
    'yandex_morda',
    'yandex_news',
    'yandex_pogoda',
    'yandex_realty',
    'yandex_sport',
    'yandex_tv',
    'yandex_video',
    'zen.yandex.ru',
    'turbo.yandex.ru',
    'collections.yandex.ru',
    'yandex_player',
    'yandex.adlibsdk',
    'o.yandex.ru',
    'games.yandex.ru'
]
ADDITIONAL_COOKIES_FOR_COOKIE_MANAGER = [
    'cycada',
    'bltsr',
    'crookie',
    'los',
    'kpunk',
    'cmtchd',
    'tvoid',
]
