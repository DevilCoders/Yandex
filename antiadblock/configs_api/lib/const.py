# coding=utf-8
import re2

SECRET_DECRYPT_CRYPROX_TOKEN = 'ahphiPh4iesahNgu7Ahdoh4ni9naij9v'  # Токен для запросов на расшифровку ссылки в прокси. TODO: перейти на tvm2

# For url decryption
DECRYPT_TOKEN_HEADER_NAME = 'x-aab-debug-token'  # Заголовок, в который кладется SECRET_DECRYPT_CRYPROX_TOKEN
DECRYPT_RESPONSE_HEADER_NAME = 'x-aab-debug-response'  # В этот хедер кладется ответ на запрос на расшифровку ссылки
PARTNER_TOKEN_HEADER_NAME = 'x-aab-partnertoken'  # Хедер с токеном партнера в cryprox

# Messages
FAILED_TO_DECRYPT_MSG = 'Failed to decrypt url. Probably reasons: Url is not crypted, url is not belong to the partner, url is invalid.'
FORBIDDEN_URL_DECRYPT_MSG = 'Forbidden to decrypt current url.'
EMPTY_URL_DECRYPT_MSG = 'Query arg `url` is cannot be empty'
NONE_DECRYPT_URL_ARG_MSG_TEMPLATE = 'Query arg `url` is missing'
MAX_URLS_TO_DECRYPT = 1000
TOO_MANY_URL_TO_DECRYPT_MSG = 'Too many urls to decrypt. Max urls count is {}'.format(MAX_URLS_TO_DECRYPT)
SERVICE_IS_INACTIVE_MSG = u"Сервис отключен"

ROOT_LABEL_ID = 'ROOT'

ANTIADB_SUPPORT_QUEUE = 'ANTIADBSUP'
READY_FOR_FILL_TAG = "ready4autofill"

GET_COOKIES_RE = re2.compile(r'.*/(?P<cookies>\S+)/.*')
DEFAULT_COOKIE = 'bltsr'
DEFAULT_YAUID_COOKIE = 'crookie'
SUPPORTED_YAUID_COOKIES = [
    'crookie',
    'F5y8AXyUIS',
    'fUBMvPpesS',
    'Vd1URbKNns',
    'GOPmuGNOSq',
    'IGZtST094R',
    'Iu93OYsfUm',
    'xag1KkQUGc',
    'Sd0j32Tnel',
    'HRtwNvaEs5',
    'H7YMtxGfdy',
    'oS2GvGzEtd',
    'LhfDDwJR8O',
    'ZLPd9IBm9h',
    'fxYguKCegV',
    'HVHCZRNRWD',
    'G4ufM8iJmZ',
    'mt9ovs2y4R',
    'sX3J4lfLop',
    'EdC6GUuhJW',
]

EXPERIMENT_DATETIME_FORMAT = '%Y-%m-%dT%H:%M:%S'

# https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Security-Policy#fetch_directives
ALLOWED_CSP = {
    "default-src",
    "connect-src",
    "child-src",
    "frame-src",
    "font-src",
    "media-src",
    "img-src",
    "manifest-src",
    "object-src",
    "prefetch-src",
    "script-src",
    "style-src",
    "worker-src",
}
