# -*- coding: utf8 -*-
# основные общие параметры и константы
from enum import IntEnum
from netaddr import IPNetwork

from re2 import escape as re_escape, compile as re_compile

from library.python import resource
from antiadblock.cryprox.cryprox.common.tools.js_minify import Minify
from antiadblock.cryprox.cryprox.common.tools.ip_utils import get_yandex_nets

# Common headers names
ARGUS_STANDARD_HEADER_NAME = 'x-aab-argus-standard'
ARGUS_REPLAY_HEADER_NAME = 'x-aab-argus-replay'
DEBUG_TOKEN_HEADER_NAME = 'x-aab-debug-token'  # Токен для дебаг-запросов
DEBUG_RESPONSE_HEADER_NAME = 'x-aab-debug-response'  # В этот хедер кладется ответ на дебаг-запрос
REQUEST_ID_HEADER_NAME = 'X-AAB-RequestId'  # id запроса
PARTNER_TOKEN_HEADER_NAME = 'X-AAB-PartnerToken'  # Токен партнера
SEED_HEADER_NAME = 'X-AAB-CRY-SEED'
PROXY_SERVICE_HEADER_NAME = 'X-AAB-Proxy'  # Заголовок, которым помечаются запросы прокси
DECRYPT_YAUID_HEADER = 'X-AAB-CryptedUid'  # хедер, в котором передается зашифрованный yandexuid в yauid decrypt api
CRYPTED_HOST_HEADER_NAME = 'x-aab-crypted-host'  # Хедер, в котором передается хост для шифрования ссылок
CRYPTED_HOST_OVERRIDE_HEADER_NAME = 'x-aab-host'  # хедер, который выставляет партнер при ответе на запрос, чтобы переопределять хост для шифрования ссылок
CRYPTED_URL_HEADER_NAME = 'x-aab-request-url'  # Хедер, в котором передается url для шифрования ссылок
NGINX_SERVICE_ID_HEADER = 'x-aab-serviceid'  # хедер в котором передается для логирования в NGINX service_id партнера
EXPERIMENT_ID_HEADER = 'X-Aab-Exp-Id'  # хедер в котором передается значение для выбора экспериментального конфига
FULL_RESPONSE_LOGGABLE_HEADER = 'X-AAB-Loggablefull'  # хедер в котором NGINX прокидывает параметр для вычисления логгировать ли не обработанный ответ
FETCH_URL_HEADER_NAME = 'X-Aab-Fetch-Url'  # хедер, в котором передается урл, который необходимо сфетчить
STRM_TOKEN_HEADER_NAME = 'X-Strm-Antiadblock'

CRYPTED_HOST_OVERRIDE_SERVICE_WHITELIST = ['zen.yandex.ru']

CSP_HEADER_KEYS = ['content-security-policy', 'content-security-policy-report-only']

INAPP_REQUEST_URL_PATHS = ["/appcry/", "/appcry"]

# для исправления относительных ссылок в html5 креативах (там по своему раскладываются файлы)
# расширения которые не шифруем (особенность библиотеки adobe, которую используют клиенты), подробнее:
# https://st.yandex-team.ru/ANTIADB-314 + https://st.yandex-team.ru/ANTIADB-1026
# https://st.yandex-team.ru/ANTIADB-1606
NOT_CRYPTED_EXTS = ('jpg', 'jpeg', 'png', 'gif', 'tif', 'tiff', 'wbmp', 'ico', 'jng', 'bmp', 'svg')

EXTENSIONS_TO_ACCEL_REDIRECT = ['gif', 'jpg', 'jpeg', 'png', 'tif', 'tiff', 'wbmp', 'ico', 'jng', 'bmp', 'svg', 'svgz', 'webp', 'woff', 'woff2']
ACCEL_REDIRECT_URL_RE = [  # на такие урлы мы делаем Accel-Redirect, для того, чтобы nginx мог сам отдать ресурс
    r'^(?:https?:)?//yastatic\.net/.*?\.(?:{})(?:\?[\w\-=.&]+)?$'.format("|".join(EXTENSIONS_TO_ACCEL_REDIRECT)),
    r'^(?:https?:)?//favicon\.yandex\.net/favicon/',  # favicon.yandex.net/favicon отдает favicon доменов
    r'^(?:https?:)?//static-mon\.yandex\.net/',
    r'^(?:https?:)?//storage\.mds\.yandex\.net/get\-(?:canvas\-html5|bstor)/.*?\.(?:{})(?:\?[\w\-=.&]+)?$'.format("|".join(EXTENSIONS_TO_ACCEL_REDIRECT)),
]
ACCEL_REDIRECT_PREFIX = '/proxy/'  # Accel-Redirect: ${ACCEL_REDIRECT_PREFIX}${url}
PARTNER_ACCEL_REDIRECT_PREFIX = '/aab-accel-redirect/'  # Accel-Redirect for partner: ${PARTNER_ACCEL_REDIRECT_PREFIX}${url}
FOLLOW_REDIRECT_URL_RE = []
CLIENT_REDIRECT_URL_RE = []

# TODO: check out those https://wiki.yandex-team.ru/LegalDep/domain/ccTLD/
# YANDEX_CCTLDS_RE = '|'.join([
#     'ru', 'by', 'kz', 'ua', 'md', 'ee', 'lt', 'lv', 'de', r'co\.il', 'pl', 'bg',
#     'rs', 'ro', 'az', 'uz', r'com\.am', 'kg', 'tj', 'tm', r'com\.ge', 'sk', 'fr',
#     'it', 'es', 'eu', 'ar', r'com\.au', 'dk', 'do', 'fi', 'ga', 'gt', 'id', r'co\.id',
#     'ie', 'in', r'jp\.net', 'lu', 'mx', 'my', 'no', 'nu', 'pt', 'qa', 'so', 'st',
#     'sx', r'com\.tc', r'com\.tr',
# ])
# YANDEX_CCTLDS_RE = r'ru|ua|by|kz|com\.tr|fr|com|pl|fi|com\.am|az|kg|lv|md|tj|tm|ee|com\.ge|uz|lt|eu|co\.il'
YANDEX_CCTLDS_RE = r'ru|ua|by|kz|com\.am'

# Параметры расположения скрипта детекта адблока
DETECT_LIB_HOST = 'aab-pub.s3.yandex.net'
DETECT_LIB_HOST_RE = re_escape(DETECT_LIB_HOST)
COOKIE_MATCHER_DOMAIN_RE = r'(partner-webmon|real-ping-mon|cluster-webstatus|http-check-headers)\.yandex\.ru'
DEFAULT_DETECT_LIB_PATH = '/lib.browser.min.js'
TEST_DETECT_LIB_PATH = '/beta.lib.browser.min.js'
AUTOREDIRECT_DETECT_LIB_PATH = '/turbo.lib.browser.min.js'
WITHOUT_CM_DETECT_LIB_PATH = '/without_cm.lib.browser.min.js'
WITH_CM_DETECT_LIB_PATH = '/with_cm.lib.browser.min.js'
DETECT_LIB_PATHS = [DEFAULT_DETECT_LIB_PATH,
                    TEST_DETECT_LIB_PATH,
                    AUTOREDIRECT_DETECT_LIB_PATH,
                    WITHOUT_CM_DETECT_LIB_PATH,
                    WITH_CM_DETECT_LIB_PATH,
                    ]
NO_CACHE_URL_RE = [r'aab-pub\.s3\.yandex\.net/(?:(?:beta|turbo|internal|external)\.)?lib\.browser\.min\.js\S*?']
# регулярка mime-типов в которых мы криптуем ссылки
CRYPT_MIME_RE = [r'text/(?:html|css|plain|javascript|xml)|application/(?:javascript|json|x-javascript)']

# расширения файлов, которые могут кэшироваться
CACHE_EXTENTIONS = ('.js', '.css')

# глобальная правка относительных урлов внутри зашифрованных ссылок
CRYPT_RELATIVE_URL_RE = [
    r'(?:(?:src|href)\s?[=:]|url\s?\(?|@import|<object[^>]+?data=)\s?[\'\"\(]([\w./]+?[\w.\-/%+?&=*;:~#]+?' +
    r'\.(?:js|css|jpe?g|[jp]ng|gif|ico|svgz?|bmp|tiff?|woff2?|webp|swf|ttf|eot)(?:\?[\w\-=.&#]+)?)[\'\"\)]',
]

# Добавлять / после шифрования ссылки (может быть переопределено в конфиге партнера)
CRYPT_ENABLE_TRAILING_SLASH = False

CRYPTED_URL_MIXING_STEP = 7
DEFAULT_URL_TEMPLATE = (('/', 3000),)  # symbols allowed in templates: /?&.*![]~$
# https://st.yandex-team.ru/ANTIADB-2078

CRYPT_URL_RE = [
    #    b'.*\.yandex\.net/js/style\.css',
    #    b'.*\.yandex\.net/js/main\.js',
    #    b'.*\.yandex\.net/js/images/.*',
    r'static-mon\.yandex\.net/.*?',  # домен для раздачи расширенной либы с детектом и куки-матчингом
]

PROXY_URL_RE = CRYPT_URL_RE + [  # NB: единственный массив урлов для проксирования, поведение отличается от партнёрского PROXY_URL_RE
    r'{}/.*'.format(DETECT_LIB_HOST_RE),  # здесь лежат либы для партнеров (детектор, либа шифрования и тп)
]

YANDEX_METRIKA_URL_RE = [
    # нужно для корректного шифрования объекта Ya, в этом файле определяется его метод Metrika https://st.yandex-team.ru/ANTIADB-1422, https://st.yandex-team.ru/ANTIADB-2782
    r'mc\.yandex\.(?:ru|ua|by|kz|az|kg|lv|md|tj|tm|uz|ee|fr|co\.il|com\.ge|com\.am|com\.tr)/(?:metrika|watch|webwisor)/.*?'
]

STRM_URL_RE = [
    r'strm\.yandex\.net/int/enc/srvr\.xml',
]

# JS-ки Дзен Плеера для передачи им на фронт функций шифрования
DZEN_PLAYER_URL_RE = [
    r'static\.dzeninfra\.ru/yandex-video-player-iframe-api(?:-bundles)?/.*?\.js',
]

# Регулярное выражение для поиска ресурсов и замены их на xhr запрос https://st.yandex-team.ru/ANTIADB-1255 , https://st.yandex-team.ru/ANTIADB-1369
REPLACE_RESOURCE_WITH_XHR_RE = [
    r'(?:<|\\u003c|\\x3c)script(?P<attrs1>[^>]*?)\s?src=\\?"(?P<src_script>{SRC_RE})\\?"\s*(?P<attrs2>[^>]*?)\s?(?:>|\\u003e|\\x3e)\s*(?:<|\\u003c|\\x3c)/script(?:>|\\u003e|\\x3e)',
    r'<link\s(?:[^>]*?)rel="(?P<style>style)sheet"(?:[^>]*?)\shref="(?P<src_style>{SRC_RE})"(?:[^>]*?)\s?>']

HIDE_GRAB_HEADER_RE = r'..a.a.b.g.r.a.b.a.r.g'  # r'jBafadbMgarSaVbDaIrDgc' matches
HIDE_URI_PATH_HEADER_RE = r'..a.a.b.u.r.i.p.a.t.h'
HIDE_META_ARGS_HEADER_MAX_SIZE = 6144  # 6kB

DENY_HEADERS_FORWARD_RE = [
    b'Host',
    b'Proxy-Connection',
]

EXTUID_COOKIE_NAMES = [
    'addruid',  # кука, которую ставит кукиматчинг либа для разметки пользователей
]

EXCLUDE_COOKIE_FORWARD = [
    'bltsr',  # кука детекта блокировщика, проставляемая нашей либой детекта
]

# кука, в которой хранится зашифрованный идентификатор пользователя (yandexuid) на площадках партнеров
CRYPTED_YAUID_COOKIE_NAME = 'crookie'
CRYPTED_YAUID_TTL = 14 * 24  # TTL куки с шифрованным yandexuid (в часах)
CRYPTED_YAUID_URL = 'https://http-check-headers.yandex.ru'  # доступные домены https://st.yandex-team.ru/ANTIADB-577

# Кука с uid-ом пользователя на площадках Яндекса
YAUID_COOKIE_NAME = 'yandexuid'
YAUID_COOKIE_MAX_LENGTH = 20

DENY_HEADERS_BACKWARD_RE = [  # эти хедеры не проксируются во всех ответах
    b'Content-Length',
    b'Transfer-Encoding',
    b'Content-Encoding',
    b'Connection',
    b'Etag',
]

DENY_HEADERS_BACKWARD_PROXY_RE = [  # эти хэдеры удаляются для запросов типа PROXY (НЕ партнерские урлы)
    b'Strict-Transport-Security',  # see https://st.yandex-team.ru/ANTIADB-345
]

# список content-types, которые нужно изменить, если запрашивается js-файл или css-файл
# https://st.yandex-team.ru/ANTIADB-1788
CONTENT_TYPES_FOR_FIX = [
    '',
    'text/html',
    'text/plain',
]
FIXED_CONTENT_TYPE = {
    '.js': 'application/javascript',
    '.css': 'text/css',
}

# Список тэгов, которые мы шифруем на странице
COMMON_TAGS_REPLACE_LIST = ['div']
# Шаблон регулярки на тэги
TAG_REPLACE_REGEXP_TEMPLATE_LIST = [
    # HTML
    r'(?:</?|\\u003[cC]|\\x3[cC])({})\b',  # 3c = '<'
    # CSS
    r'\b({})(?:[\s\.\:{{#>,+\[\"]|\\u003[Ee]|\\x3[eE])',  # 3e = '>'
]
# Регулярка на поиск использования тэгов в js-коде
FIX_JS_TAGS_RE = r'(\.{})\W'
# Шаблон для вставки стиля перед закрытием head
INSERT_STYLE_TEMPLATE = '<style>{} {{display: block}}</style></head>'
# Элементы HTML которые могут быть самозакрытыми и корректно обрабатываться браузером
# https://www.w3.org/TR/2012/WD-html-markup-20120320/syntax.html#syntax-elements
VOID_HTML_ELEMENTS = ['area', 'base', 'br', 'col', 'command', 'embed', 'hr', 'img', 'input', 'keygen', 'link', 'meta', 'param', 'source', 'track', 'wbr',
                      # при закрытии этих элементов svg иногда ломаются сайты на react https://st.yandex-team.ru/ANTIADB-1339
                      'path', 'circle']
SELF_CLOSING_TAGS_RE = r'<([a-zA-Z][\w\-]*)(?:\s[^<>]*)?/>'


YANDEX_NETS = [IPNetwork(network) for network in get_yandex_nets()]

# TTL куки детекта по-умолчанию для всех партнеров (в часах)
DETECT_COOKIE_TTL = 24 * 14  # 14 дней

# Набор общих для всех блоков HTML для детекта адблока
# DETECT_ELEMS -> DETECT_HTML
# https://st.yandex-team.ru/ANTIADB-1323
DETECT_HTML = [u'<div class="adbanner" id="AdFox_banner"></div>',
               u'<div class="advblock" id="yandex_ad"></div>',
               u'<div class="b-adv" id="yandex_rtb"></div>',
               u'<div class="b-banner"></div>', u'<div class="bannerad"></div>',
               u'<div class="pagead"></div>', u'<div class="pub_300x250m"></div>',
               u'<div class="pub_728x90"></div>', u'<div class="reklama"></div>',
               u'<div class="sideads"></div>', u'<div class="smart-info-wrapper"></div>',
               u'<div class="sponsoredlinks"></div>', u'<div class="text-ad"></div>']


# https://st.yandex-team.ru/PCODE-5677
# Набор общих ссылок для проверки наличия детекта. Ссылки будут пытаться загружаться на стороне показывающего кода.
# Если ссылка не загрузилась, то адблок присутствует. Описание формата и нюансы - в тикете.
DETECT_LINKS = [
    {'type': 'get', 'src': 'https://an.yandex.ru/system/context.js'},
]

DEBUG_API_TOKEN = 'ahphiPh4iesahNgu7Ahdoh4ni9naij9v'


class CookieMatching(IntEnum):
    """
    Какой вариант куки матчинга необходимо использовать для партнера
    """
    disabled = 0  # Кукиматчинг не нужен
    image = 1  # Кукиматчинг только при помощи картинки
    refresh = 2  # Кукиматчинг только при помощи редиректа
    image_or_refresh = 3  # Кукиматчинг сначала при помощи картинки, если не получилось, то при помощи редиректа
    crypted_uid = 4  # Кукиматчинг через шифрованный yandexuid

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return str(self.value)


class DetectCookieType(IntEnum):
    """
        https://st.yandex-team.ru/ANTIADB-495
        Тип установки куки детекта
    """
    current = 0  # Кука ставится только на текущий домен
    children = 1  # Кука ставится так же и на все поддомены
    list = 2  # Кука ставится на список доменов

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return str(self.value)


# Способ куки-матчинга для партнеров по-умолчанию
CM_DEFAULT_TYPE = [CookieMatching.image, CookieMatching.refresh]
# Ссылка для куки-матчинга пользователя через редирект для партнеров по-умолчанию
CM_DEFAULT_REDIRECT_URL = '//an.yandex.ru/mapuid/'
# Ссылка на картинку для куки-матчинга через загрузку картинки для партнеров по-умолчанию
CM_DEFAULT_IMAGE_URL = 'https://statchecker.yandex.ru/mapuid/'

COOKIE_DOMAIN_DEFAULT_TYPE = DetectCookieType.current


class EncryptionSteps(IntEnum):
    """
    Допустимые этапы шифрования контента страниц партнера, числовое значение - номер бита в битовой маске
    """
    (pcode_replace,  # замена паттернов в скриптах детекта/отрисовки рекламы
     advertising_crypt,  # шифрование рекламы на странице, работает только совместно с pcode_replace
     partner_content_url_crypt,  # шифрование урлов партнера на статику
     partner_content_class_crypt,  # шифрование классов партнера
     tags_crypt,  # шифрование тэгов
     loaded_js_crypt,  # шифрование содержимого загружаемых js скриптов
     inline_js_crypt,  # шифрование содержимого inline js скриптов
     crypt_relative_urls_automatically) = range(8)  # шифрование относительных ссылок в файлах автоматически, работает только совместно с partner_content_url_crypt


# Шифрование по умолчанию - только производим замену в скриптах детекта/отрисовки рекламы партнерского кода
DEFAULT_ENCRYPTION_STEPS = [EncryptionSteps.pcode_replace]

ENCODE_CSS_FUNC = Minify(resource.find("/cryprox/common/js_func/encode_css_func.js")).getMinify()
ENCODE_URL_FUNC = Minify(resource.find("/cryprox/common/js_func/encode_url_func.js")).getMinify()
DECODE_URL_FUNC = Minify(resource.find("/cryprox/common/js_func/decode_url_func.js")).getMinify()
IS_ENCODED_URL_FUNC = Minify(resource.find("/cryprox/common/js_func/is_encoded_url_func.js")).getMinify()


IMAGE_TO_IFRAME_CRYPTING_URL_FRAGMENT = '#DSD'

SCRIPT_KEY_PLACEHOLDERS_RE = re_compile(r'\b[a-zA-Z_]\w*=\"__scriptKey([01])Value__\"[;,]?')

COOKIELESS_HOST_RE = r'([^.\_]+)\.(?:test\.)?(?:naydex|static\-storage|cdnclab|clstorage)\.net'

STATIC_URL_RE = [r"aab\-pub\.s3\.yandex\.net\/iframe\.html"]

# Формат даты задающей время начала эксперимента в конфигах
EXPERIMENT_START_TIME_FMT = '%Y-%m-%dT%H:%M:%S'

# https://st.yandex-team.ru/ANTIADB-1968
AUTOREDIRECT_SCRIPT_RE = r'.+?\..+?/detect\?domain=.*?&path=.*?&proto=https?'
AUTOREDIRECT_SERVICE_ID = 'autoredirect.turbo'
AUTOREDIRECT_KEY = AUTOREDIRECT_SERVICE_ID + "::active::None::None"
AUTOREDIRECT_REPLACE_RE = r'^().+?'
AUTOREDIRECT_QARGS = ("proto", "domain", "path")
AUTOREDIRECT_FIND_TEMPLATE_RE = re_compile(r'\${(.*?)}')


class Experiments(IntEnum):
    NONE = 0
    BYPASS = 1
    FORCECRY = 2
    NOT_BYPASS_BYPASS_UIDS = 3

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return str(self.value)


class UserDevice(IntEnum):
    DESKTOP = 0
    MOBILE = 1

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return str(self.value)


BYPASS_UIDS_TYPES = {
    UserDevice.DESKTOP: 'ANTIADBLOCK_BYPASS_UIDS_DESKTOP',
    UserDevice.MOBILE: 'ANTIADBLOCK_BYPASS_UIDS_MOBILE',
}

INTERNAL_EXPERIMENT_CONFIG = 'ANTIADBLOCK_CRYPROX_INTERNAL_EXPERIMENT_CONFIG'

# При шифровании инлайн-скриптов мы используем grave accent кавычки ``, есть браузеры, которые не поддерживают их в JS
# В этих браузерах мы не шифруем инлайн-скрипты
BYPASS_CRYPT_INLINE_JS_BROWSERS = ('MSIE', 'Edge')


class InjectInlineJSPosition(IntEnum):
    HEAD_BEGINNING = 0
    HEAD_END = 1

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return str(self.value)
