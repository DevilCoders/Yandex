#!/usr/bin/env python3
"""This module contains a constants for library."""

DEFAULT_INTERVAL = 15 * 60  # 15 minutes
DEFAULT_MAX_LOGFILE_SIZE = 1 * 1024 ** 2  # 1MB
DEFAULT_LOG_BACKUP_COUNT = 1

BASE_URL = 'https://st.yandex-team.ru'
BASE_API_URL = 'https://st-api.yandex-team.ru'
BASE_FILTER = 'Queue: CLOUDSUPPORT and Updated: >= now() - 1d'
USERAGENT = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_6) ' + \
            'AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.132 ' + \
            'YaBrowser/18.1.1.841 Yowser/2.5 Safari/537.36'

ISSUE_TYPES = ('Incident', 'Problem', 'Improvement', 'Task', 'Question')
BOOLEAN_KEYS = ('issue_type_changed', 'secondary_response', 'reopened',
                'mistake_comment_type', 'sla_failed', 'managed', 'feedback_received')

FIRST_COMMENT_ROBOTS = ('robot-yc-support-api', 'spock')
REOPEN_ROBOTS = ('robot-srch-releaser', 'robot-ycloudsupport')
SYSTEM_ROBOTS = ('robot-yc-support-api', 'zomb-prj-191')

PS_TEAM = ('cayde', 'uplink', 'megaeee', 'histme', 'sergell',
            'lexsergeev', 'berty', 'alvint', 'asudarkin', 'vlku1221',
            'imartynow', 'yarins', 'denbodnar', 'staritsyna',
            'schukh', 'vit-andrienko', 'e3b3c0', 'eshkrig', 'vale76',
            'andrey-ulrikh', 'pslugin', 'rowdy', 'smamulin')
PREMIUM_TEAM = ('bitmaev', 'buhovcov', 'nkalniyazov', 'aakirillov', 'dlaur',
                'barbadjharxxx', 'sergell', 'akloginov', 'filromanov', 'trukot',
                'asverkaltsev', 'afazlyev')
SUPPORT_TEAM = ('yakovleva', 'amatol', 'vr-owl', 'krestunas', 'nooss',
                'katjrinn', 'hadesrage', 'yurovniki', 'd-penzov', 'sinkvey',
                'snowsage', 'nikmaslakov', 'dymskovmihail', 'the-nans', 'rkatsuba',
                'konst-bel', 'cameda', 'panchenkoss', 'oo-grigorev', 'panther',
                'ivan-galben', 'al-kurilov', 'v-pavlyushin', 'teminalk0',
                'ishadovsky', 'eremin-sv', 'hercules', 'alex-master', 'apnesmelov',
                'yaaol', 'wunderkater', 'lostghost1', 'vsvegner', 'mrduglas',
                'kasparv', 'a-moshin', 's-pankin', 'tonkas', 'atrutnev',
                'a-ulyev', 'kirillovnl', 'apereshein')
ASSESSORS = ('amletchenka', 'stefan92', 'mshelen768', 'iurusov', 'anastasiasdk',
                'svkut', 'purchata', 'abefimenkov', 'dvpanfilova', 'edregolar',
                'shevczov96', 'romang227', 'a-kseniya711', 'whateverkate',
                'veronikbor', 'great-gross', 'gavrilov63', 'd3fold', 'valeria99',
                'krismanukhova', 'blinov-ea', 'sbrusov', 'v-paramut', 'klaisens',
                'preda10r', 'natalyatru', 'gate62', 'nesterova-tmb', 'nst1337',
                'babushkin-evg', 'blink1994', 'lera-b-e', 'bessonova-v-s',
                'les-artemius', 'aogross', 'shevczov96', 'lapex91', 'vrmy87',
                'savelevaelena', 'r-panichev', 'bktdev', 'darthplcb', 'spec-supp0rt',
                'alexkorobkov', 'avmelenteva', 'klochkov-alex', 'avenastya',
                'valerymironov', 'novikovvn', 'plzgofarm', 'fefelov-ds', 'dmzpp',
                'fomind39', 'skrebkov', 'timatkova', 'el-romanova21', 'alexmesner',
                'shaulov', 'msemkin', 'podivilov', 'naseho', 'tesla254', 'u7pozd',
                'ninazhelnina', 'svtikhova', 'arty666')
TRACKER_SUPPORT = ('arvo', 'lurk', 'andrekk77', 'tipo-4ek', 'trozh', 'kotova6301',
                    'mkachaeva', 'grinch-b22', 'malyukov', 'alexparunov', 'employee-z',
                    'mariellefox')
YDB_SUPPORT = ('kickandrun', 'vadrozh', 'novikovvn', 'sabellus')

SUPPORTS = PS_TEAM + SUPPORT_TEAM + ASSESSORS + TRACKER_SUPPORT + YDB_SUPPORT + PREMIUM_TEAM

CLOSED = u'Пользователь сам закрыл тикет'

# TODO: move to object storage and use json
SLA = {
    'free': {
        'incident': 1440,
        'problem': 1440,
        'improvement': 1440,
        'task': 1440,
        'question': 1440
    },
    'standard': {
        'incident': 120,
        'problem': 120,
        'improvement': 480,
        'task': 480,
        'question': 480
    },
    'business': {
        'incident': 30,
        'problem': 30,
        'improvement': 240,
        'task': 240,
        'question': 240
    },
    'premium': {
        'incident': 15,
        'problem': 15,
        'improvement': 120,
        'task': 120,
        'question': 120
    }
}
