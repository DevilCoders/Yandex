# -*- coding: utf8 -*-
from enum import IntEnum

from .system import NOT_CRYPTED_EXTS, YANDEX_CCTLDS_RE

ACCEL_REDIRECT_URL_RE = [  # на такие урлы мы делаем Accel-Redirect, для того, чтобы nginx мог сам отдать ресурс
    r'^(?:https?:)?//avatars\.mds\.yandex\.net/',     # avatars.mds.yandex.ru отдает только картинки
]

YABS_URL_RE = r'yabs\.yandex\.(?:ru|by|kz|ua|uz)'

# массив урлов баннерных систем
BANNER_SYSTEM_URL_RE = [
    r'(?:an|bs)\.yandex\.ru/.*?',
    r'{}/.*?'.format(YABS_URL_RE),
    r'yandex\.(?:{})/an/.*?'.format(YANDEX_CCTLDS_RE),
    r'yandex\.(?:{})/ads/.*?'.format(YANDEX_CCTLDS_RE),
]

# https://st.yandex-team.ru/ANTIADB-2631
NANPU_HOST_RE = r'(?:mobile\.yandexadexchange\.net|adsdk\.yandex\.ru|mobile\-ads\-beta\.yandex\.ru|adlib\-mock\.yandex\.net)(?::\d+)?'
NANPU_CRYPT_URL_RE = [
    r'adsdk\.yandex\.ru(?::\d+)?/proto/report/.*?',
]

NANPU_BK_AUCTION_URL_RE = [
    r'an\.yandex\.ru/(?:meta|adfox)/\S*?nanpu\-version=\d.*',
    r'yandex\.(?:{})/ads/(?:meta|adfox)/\S*?nanpu\-version=\d.*'.format(YANDEX_CCTLDS_RE),
]

NANPU_URL_RE = NANPU_CRYPT_URL_RE + [
    NANPU_HOST_RE + r'/.*',
]

NANPU_STARTUP_URL_RE = [
    NANPU_HOST_RE + r'/v1/startup.*?',
]

NANPU_AUCTION_URL_RE = [
    NANPU_HOST_RE + r'/v\d/(?:ad|vmap|getvast|getvideo|vcset).*?',
]

NANPU_COUNT_URL_RE = [
    r'adsdk\.yandex\.ru(?::\d+)?/proto/report/\S+?(?:an|yabs|bs)\.yandex\.ru/(?:(?:rtb|meta)?count|tracking)/.*?',
]

# массив урлов аукционов баннерных систем, мы их проксируем и доклеиваем ADB_PARAMS EXTUID_PARAM_NAME EXTUID_TAG_PARAM_NAME
# сюда же клеим параметры ограничения на розыгрыш
RTB_AUCTION_URL_RE = [
    r'(?:an|bs)\.yandex\.ru/(?:meta|page|code)/.*',
    r'{}/(?:meta|page|code)/.*'.format(YABS_URL_RE),
    r'yandex\.(?:{})/ads/(?:meta|page|code)/.*'.format(YANDEX_CCTLDS_RE),
]
# и отдельно для ручки /meta, для перехода на callback=json
# https://st.yandex-team.ru/ANTIADB-1502
BK_META_AUCTION_URL_RE = [
    r'(?:an|bs)\.yandex\.ru/meta/.*',
    r'{}/meta/.*'.format(YABS_URL_RE),
    r'yandex\.(?:{})/ads/meta/.*'.format(YANDEX_CCTLDS_RE),
]
# https://st.yandex-team.ru/ANTIADB-2755
BK_META_BULK_AUCTION_URL_RE = [
    r'(?:an|bs)\.yandex\.ru/meta_bulk/.*',
    r'{}/meta_bulk/.*'.format(YABS_URL_RE),
]
# https://st.yandex-team.ru/ANTIADB-2142
BK_VMAP_AUCTION_URL_RE = [
    r'an\.yandex\.ru/vmap/.*',
    r'yandex\.(?:{})/ads/vmap/.*'.format(YANDEX_CCTLDS_RE),
]

# массив урлов счетчиков баннерных систем, мы их проксируем и доклеиваем ADB_PARAMS EXTUID_PARAM_NAME EXTUID_TAG_PARAM_NAME
RTB_COUNT_URL_RE = [
    r'(?:an|bs)\.yandex\.ru/(?:(?:rtb|meta)?count|tracking)/.*?',
    r'{}/(?:(?:rtb|meta)?count|tracking)/.*?'.format(YABS_URL_RE),
    r'yandex\.(?:{})/an/(?:(?:rtb|meta)?count|tracking)/.*?'.format(YANDEX_CCTLDS_RE),
]
# https://st.yandex-team.ru/ANTIADB-1940 не передавать параметр adb_enabled=1
YABS_COUNT_URL_RE = [
    r'{}/count/.*?'.format(YABS_URL_RE),
]

BANNER_SYSTEM_ABUSE_URL_RE = [
    r'(?:an|bs)\.yandex\.ru/abuse/.*?',
    r'{}/abuse/.*?'.format(YABS_URL_RE),
    r'yandex\.(?:{})/an/abuse/.*?'.format(YANDEX_CCTLDS_RE),
]

# Регулярка для server-side перевода jsonp ответа от БК в json
PJSON_RE = '(?:.{2,15}\\[\"{0,1}\\d+\"{0,1}\\]|json)'
META_JS_OBJ_EXTRACTOR_RE = PJSON_RE + '\\(\'(\\{[\\S\\s]*\\})\'\\)$'

# HTML5 (https://st.yandex-team.ru/ANTIADB-794)
# Регулярка для поиска относительных урлов в html5 баннерах Директа
CRYPT_HTML5_RELATIVE_URL_RE = [
    r'(?:(?:src|href)\s?[=:]|url\s?\(?|@import|<object[^>]+?data=|source=)\s?\\?[\'\"\(]([\w./]+?[\w.\-/%+?&=*;:~#]+?)(?:\.(?:{}))?(?:\?[\w\-=.&]+)?\\?[\'\"\)]'.format('|'.join(NOT_CRYPTED_EXTS)),
]

# Регулярка на признак html5-баннера Директа
HTML5_BANNER_RE = [
    r'"html5":\s?true',
]

# Регулярка для нахождения Base path для построения относительных урлов
HTML5_BASE_PATH_RE = [
    r'"basePath":\s?"([^"]*)"',
]

# Регулярки на ресурсы показывающего кода.
# В основном используется для подстановки в эти файлы конфига шифрования вместо макроса __ADB_CONFIG__
PCODE_JS_REPLACE_URL_RE = [
    r'(?:an|pc|yabs|bs)\.yandex\.ru/(?:system|resource|partner-code/loaders|partner-code-bundles)/.*?',
    r'yandex\.(?:{})/ads/(?:system|resource|partner-code/loaders|partner-code-bundles)/.*?'.format(YANDEX_CCTLDS_RE),
    r'yastat(?:ic)?\.net/(?:partner-code|safeframe|pcode)(?:-bundles)?/.*?',
    # JS-ки Яндекс Картинок для передачи им на фронт функций шифрования [https://st.yandex-team.ru/IMAGESUI-8320]
    r'(?:betastatic\.)?yastatic\.net/images-islands/[\w\-]+/pages-desktop/common/_common\.ru\.js',
    r'(?:betastatic\.)?yastatic\.net/q/crowdtest/serp-static-nanny/static/[\w\-]+/static/fiji/freeze/[\w\-]+\.js',
    r'(?:betastatic\.)?yastatic\.net/s3/fiji-static/\_/[\w\-]+\.js',
    r'serp-static-testing\.s3\.yandex\.net/fiji/static/fiji/freeze/[\w\-]+.js',
    # фриз статитки у Яндекс Картинок [https://st.yandex-team.ru/SAKHALIN-2335]
    r'(?:betastatic\.)?yastatic\.net/images-islands/_/\S+?\.js',
    # JS-ки Яндекс Плеера для передачи им на фронт функций шифрования
    r'(?:betastatic\.)?yastatic\.net/yandex-video-player-iframe-api(?:-bundles)?/.*?\.js',
    r'yastat(?:ic)?\.net/awaps-ad-sdk-js\S*?',
    r'(?:betastatic\.)?yastatic\.net/vas-bundles/\S*',
    # рекомендательный виджет https://st.yandex-team.ru/ANTIADB-1779
    r'yastat(?:ic)?\.net/pcode-native-bundles/\d+/(?:widget|loader)_adb\.js',
]

PCODE_LOADER_URL_RE = [
    r'an\.yandex\.ru/(?:system|resource)(?:\-debug)?/.*?',
    r'yandex\.(?:{})/ads/(?:system|resource)(?:\-debug)?/.*?'.format(YANDEX_CCTLDS_RE),
]

STORAGE_MDS_URL_RE = r'storage\.mds\.yandex\.net/get\-(?:canvas\-html5|bstor)/.*?'
# https://st.yandex-team.ru/ANTIADB-2513
# запросы, которые будут c тегом YANDEX даже если они будут заматчены регуляркой PARTNER_URL_RE
# PARTNER_URL_RE = self.partner_config.PROXY_URL_RE + self.partner_config.CRYPT_URL_RE
FORCE_YANDEX_URL_RE = [
    r"yastatic\.net/safeframe-bundles/[\d\.\-/]+/(?:protected/)?render(?:_adb)?\.html",
]

CRYPT_URL_RE = [
    r'(?:(?:an|pc)\.yandex\.ru|yandex\.(?:{})/(?:an|ads))/(?:system|resource|partner-code/loaders|partner-code-bundles|tracking)/?.*?'.format(YANDEX_CCTLDS_RE),
    r'{}/(?:system|resource|partner-code/loaders|partner-code-bundles|tracking)/?.*?'.format(YABS_URL_RE),
    r'(?:st\.)?yandexadexchange\.net/.*?',
    r'avatars\.mds\.yandex\.net/get\-(?:canvas|direct(?:-picture)?|rtb|yabs_performance)/.*?',
    r'avatars\-fast\.yandex\.net/.*?',
    r'(?:betastatic\.)?yastat(?:ic)?\.net/.*?',
    r'direct\.yandex\.ru/\?partner',
    STORAGE_MDS_URL_RE,
]

PROXY_URL_RE = FORCE_YANDEX_URL_RE + CRYPT_URL_RE + [
    r'favicon\.yandex\.net/favicon.*',
    r'(?:(?:an|bs)\.yandex\.ru|yandex\.(?:{})/(?:an|ads))/(?:count|rtbcount|meta|page|blkset)/.*'.format(YANDEX_CCTLDS_RE),
    r'{}/(?:count|rtbcount|meta|page|blkset)/.*'.format(YABS_URL_RE),
    r'direct\.yandex\.ru/.*',
    r'serp-static-testing\.s3\.yandex\.net/.*?',
]

# https://st.yandex-team.ru/ANTIADB-2732
AVATARS_MDS_URL_RE = [
    r'(?:storage|avatars)\.mds\.yandex\.net/.*?',
]

STATIC_URL_RE = [  # урлы статики, будут шифроваться под бескуковый домен
    r'favicon\.yandex\.net/favicon.*?',
    r'(?:st\.)?yandexadexchange\.net/.*?',
    r'avatars\-fast\.yandex\.net/.*?',
    r'direct\.yandex\.ru/\?partner',
    r'yastatic\.net/islands/.*?',
    # https://st.yandex-team.ru/ANTIADB-2803 матчим только бандлы
    r'yastat(?:ic)?\.net/(?:partner-code|safeframe|pcode(?:-native)?|awaps-ad-sdk-js)(?:-bundles)/.*?',
    # и лоадер медиа-баннеров
    r'yastat(?:ic)?\.net/pcode/media/.*?',
    # https://st.yandex-team.ru/SECAUDIT-3011 все swf/html с yastatic должны шифроваться под naydex, даже если злоумышленник пытается спрятать расширение файла
    r'(?:betastatic\.)?yastat(?:ic)?\.net/.*(?:\.|%2[Ee])(?:(?:s|%73)(?:w|%77)(?:f|%66)|(?:h|%68)(?:t|%74)(?:m|%6[Dd])(?:l|%6[Cc])?)(?:(?:\?|%3[Ff]).*)?',
]


# массив урлов для которых мы обрабатываем 302 Redirect серверно
FOLLOW_REDIRECT_URL_RE = []

# Массив урлов с дополнительными (по большей части внешними) счетчиками, которые присутствуют в HTML5 креативах БК
# https://st.yandex-team.ru/ANTIADB-996
COUNTERS_URL_RE = [
    r'mc\.yandex\.ru/pixel/\d+.*?',
    r'(?:\w+\.)*tns\-counter\.ru/.*?',
    r'ad\.doubleclick\.net/ddm/.*?',
    r'ad\.adriver\.ru/cgi\-bin/e?rle\.cgi.*?',
    r'bs\.serving\-sys\.com/serving/adServer\.bs.*?',
    r'wcm(?:\.solution|\-ru\.frontend)\.weborama\.fr/fcgi\-bin/dispatch\.fcgi.*?',
    r'rgi\.io/tr/mix/.*?',
    r'track\.rutarget\.ru/win.*?',
    r'ssl\.hurra\.com/TrackIt.*?',
    r'mc\.admetrica\.ru/show.*?',
    r'amc\.yandex\.ru/show.*?',
    r'\d+\.verify\.yandex\.ru/verify\S*?',
    r'gdeby\.hit\.gemius\.pl/.*?',
]

# массив урлов для которых мы отдаем 302 редирект на клиента (такие урлы мы не проксируем, не шифруем,
# e.g.: переходы по кликовым ссылкам)
CLIENT_REDIRECT_URL_RE = COUNTERS_URL_RE + [
    r'direct\.yandex\.ru/\?partner',
]

DENY_HEADERS_BACKWARD_RE = [  # эти хедеры не проксируются в ответе
    b'Set-Cookie',
    b'Access-Control-Allow-Credentials',
]

REPLACE_BODY_RE = {
    r'(?:partner-code(?:-bundles/\d+)?/loaders|system)/(context)\.js': b'context_adb',
    # рекомендательный виджет
    r'(yastat(?:ic)?\.net/pcode-native/loaders/loader\.js).*?': b'an.yandex.ru/system/context_adb.js',
    # виджет на куковом домене https://st.yandex-team.ru/ANTIADB-2542
    r'(?:an\.yandex\.ru|yandex\.(?:{})/ads)/system/(widget)\.js'.format(YANDEX_CCTLDS_RE): b'context_adb',
}

CRYPT_BODY_RE = [r'\byandex(?:_context_callbacks|ContextAsyncCallbacks|ContextSyncCallbacks)\b',
                 r'\b(Context)[^\w\-/]',
                 r'\byandex_(?:ad|rtb)',
                 ]

JSONP_PARAM = 'jsonp'  # https://st.yandex-team.ru/ANTIADB-2418
JSONP_PARAM_VALUE = '1'
DISABLED_FLAGS_PARAM = 'disabled-flags'
BK_EXTUID_PARAM = 'ext-uniq-id'
BK_EXTUID_TAG_PARAM = 'tag'
BK_URL_PARAMS = 'redir-setuniq=1'
ADB_ENABLED_FLAG = 'adb_enabled=1'  # https://st.yandex-team.ru/BSSERVER-1394
BK_DISABLE_AD_PARAM = 'disabled-ad-type'
BK_TGA_WITH_CREATIVES_PARAM = 'tga-with-creatives'
BK_AIM_BANNER_ID_PARAM = 'aim-banner-id'


# https://a.yandex-team.ru/arc/trunk/arcadia/yabs/server/libs/enums/ad_type.h?rev=7144456#L10
class DisabledAdType(IntEnum):
    (TEXT,
     MEDIA,
     MEDIA_PERF,
     VIDEO,
     VIDEO_PERF,
     VIDEO_MOTION,
     AUDIO,
     VIDEO_NONSKIPABLE) = range(8)


DEFAULT_DISABLED_AD_TYPES = [DisabledAdType.VIDEO, DisabledAdType.VIDEO_MOTION, DisabledAdType.VIDEO_PERF]
