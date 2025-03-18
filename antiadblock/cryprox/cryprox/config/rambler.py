# -*- coding: utf8 -*-


# ресурсы SSP/DSP от Rambler
CRYPT_URL_RE = [r"ssp\.rambler\.ru/capirs_async\.js(?:\?v=\d*)?",
                r"ssp\.rambler\.ru/acp/capirs_main\.[a-zA-Z0-9]+\.js",
                r"ssp\.rambler\.ru/(?:userip|lpdid)\.jsp?",
                r"ssp\.rambler\.ru/blockstat\?log_visibility=0&",
                r"img\d\d\.ssp\.rambler\.ru/file\.jsp\?.*?",
                r"profile\.ssp\.rambler\.ru/sandbox\?",
                r"profile\.ssp\.rambler\.ru/sync2\.204\?",
                r"(?:\w+\.)?dsp[.-]rambler\.ru(?:\b|/.*?)",
                r"ssp\.rambler\.ru/?",
                ]

BANNER_SYSTEM_URL_RE = CRYPT_URL_RE

RTB_AUCTION_URL_RE = [
    r'ssp\.rambler\.ru/context.jsp.*',
]

# правка относительных урлов внутри зашифрованных ссылок
CRYPT_RELATIVE_URL_RE = [
    r'(?:(?:src|href)\s?(?:[=:]|%3[Dd])|url\s?\(?|@import|<object[^>]+?data=)\s?(?:[\'\"(]|%2[28]|%28%22)([\w./]+?[\w.\-/%+?&=*;:~#]+?' +
    r'\.(?:js|css|jpe?g|[jp]ng|gif|ico|svgz?|bmp|tiff?|woff2?|webp|swf|ttf|eot)(?:\?[\w\-=.&#]+)?)(?:[\'\")]|%2[29])',
]

# массив урлов для которых мы обрабатываем 302 Redirect серверно
FOLLOW_REDIRECT_URL_RE = [r'img\d\d\.ssp\.rambler\.ru/.*',
                          r'dsp-rambler\.ru/.*',
                          r'(?:\w+\.)?dsp\.rambler\.ru/.*',
                          ]

# массив урлов для которых мы отдаем 302 редирект на клиента (такие урлы мы не проксируем, не шифруем,
# e.g.: переходы по кликовым ссылкам)
CLIENT_REDIRECT_URL_RE = [r'click\d\d\.dsp\.rambler\.ru/click.*?']

PROXY_URL_RE = CRYPT_URL_RE + [r'(?:(?:img\d\d|profile)\.)?ssp\.rambler\.ru/.*']

# регулярка дополнительных mime-типов в которых мы криптуем ссылки
CRYPT_MIME_RE = [r'text/url|application/x-(?:[fm]ixed-iframe-html|html|iframe-html|shared-scripts)']

# JSON завернут в callback, например, Begun_Autocontext_oi1f2xr7p0({})
JS_OBJ_EXTRACTOR_RE = r'^.*?\((.*)\)$'
