# -*- coding: utf8 -*-

from .system import EXTENSIONS_TO_ACCEL_REDIRECT, YANDEX_CCTLDS_RE

ACCEL_REDIRECT_URL_RE = [  # на такие урлы мы делаем Accel-Redirect, для того, чтобы nginx мог сам отдать ресурс
    r'^(?:https?:)?//banners\.adfox\.ru/.*?\.(?:{})(?:\?[\w\-=.&]+)?$'.format("|".join(EXTENSIONS_TO_ACCEL_REDIRECT)),
]

BANNER_SYSTEM_URL_RE = [  # массив урлов баннерных систем
    r'ads\.adfox\.ru/.*?',
    r'an\.yandex\.ru/adfox/.*?',
    r'yandex\.(?:{})/ads/adfox/.*?'.format(YANDEX_CCTLDS_RE),
]

# массив урлов аукционов баннерных систем, мы их проксируем и доклеиваем ADB_PARAMS EXTUID_PARAM_NAME EXTUID_TAG_PARAM_NAME
RTB_AUCTION_URL_RE = [
    r'ads\.adfox\.ru/\d+/(?:prepareCode|getCode|getBulk).*',
    r'an\.yandex\.ru/adfox/\d+/getBulk.*',
    r'yandex\.(?:{})/ads/adfox/\d+/getBulk.*'.format(YANDEX_CCTLDS_RE),
]

# массив урлов счетчиков баннерных систем, мы их проксируем и доклеиваем ADB_PARAMS EXTUID_PARAM_NAME EXTUID_TAG_PARAM_NAME
RTB_COUNT_URL_RE = [
    r'ads\.adfox\.ru/\d+/(?:event|goLink|clickURL).*',  # https://st.yandex-team.ru/ANTIADB-1199 goLink - это кликовые ссылки
    r'ads\.adfox\.ru/getid.*',  # getid - разметка пользователей adfox идентификаторами
]

CRYPT_URL_RE = [
    r'(?:ads6?|banners)\.adfox\.ru/.*?',
]

PROXY_URL_RE = CRYPT_URL_RE

# https://st.yandex-team.ru/ANTIADB-1793#5d414abd34894f001eb4b28d
FOLLOW_REDIRECT_URL_RE = [  # массив урлов для которых мы обрабатываем 302 Redirect серверно
    r'ads\.adfox\.ru/\d+/(?:event|prepareCode|getCode|getBulk).*',
    r'ads\.adfox\.ru/getid.*',
    r'yastat(?:ic)?\.net/pcode/adfox/header\-bidding\.js',  # https://st.yandex-team.ru/ANTIADB-2803
]

CLIENT_REDIRECT_URL_RE = [  # массив урлов для которых мы отдаем 302 редирект на клиента (такие урлы мы не проксируем, не шифруем, e.g.: переходы по кликовым ссылкам)
]

REPLACE_BODY_RE = {
    r'//ads\.adfox\.ru/\d+/(prepareCode|getCode)\?': 'getCodeTest',  # to prevent adfox cookie matching redirect
    r'//ads\.adfox\.ru/\d+/(?:getBulk|goLink|clickURL)()\?': 'Test',  # to prevent adfox cookie matching redirect https://st.yandex-team.ru/ANTIADB-1793#5d414abd34894f001eb4b28d
    r'pcode/adfox/(loader)\.js': 'loader_adb',
    r'//(?:an\.yandex\.ru|yandex\.(?:{})/ads)/system/(adfox)\.js'.format(YANDEX_CCTLDS_RE): 'context_adb',  # https://st.yandex-team.ru/ANTIADB-3061
}

CRYPT_BODY_RE = [r'\b(AdFox_banner)(?:[\w-]+)?\b']

ADFOX_EXTUID_PARAM = 'extid'
ADFOX_EXTUID_TAG_PARAM = 'extid_tag'
ADFOX_EXTUID_LOADER_PARAM = 'extid_loader'
ADFOX_EXTUID_TAG_LOADER_PARAM = 'extid_tag_loader'

ADFOX_S2SKEY = "9118308262628211612"  # TODO change to partner key (revoke this)
ADFOX_DEBUG = False
