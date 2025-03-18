# coding=utf-8
import sys
import random
import string
from time import time
from urllib import quote, unquote
from mimetypes import types_map
from collections import Counter
from itertools import izip_longest
from htmlentitydefs import name2codepoint
from urlparse import urljoin, urlparse, urlunparse, ParseResult
from base64 import b64encode, urlsafe_b64decode, urlsafe_b64encode

import re2
import cchardet as chardet
from numpy.random import RandomState

from antiadblock.libs.decrypt_url.lib import get_key, decrypt_xor, decrypt_url, SEED_LENGTH, URL_LENGTH_PREFIX_LENGTH, DecryptionError
from .cry import encrypt_xor, crypto_xor
from antiadblock.cryprox.cryprox.url.transform.rule_evade_step import encode as rule_evade_step_encode
from antiadblock.cryprox.cryprox.config.system import CRYPTED_URL_MIXING_STEP, VOID_HTML_ELEMENTS, DEFAULT_URL_TEMPLATE, IMAGE_TO_IFRAME_CRYPTING_URL_FRAGMENT
from antiadblock.cryprox.cryprox.config.service import ENV_TYPE, END2END_TESTING
from antiadblock.cryprox.cryprox.common.tools.regexp import re_completeurl, re_merge
from antiadblock.cryprox.cryprox.common.tools.misc import not_none, replace_matched_group, check_key_in_dict
from antiadblock.cryprox.cryprox.service.action import REGEX_STAT

# объект специально для body_crypt, нельзя фиксировать его seed в проде, иначе собьются проценты шифрования картинок
random_obj = random.Random()
rs = RandomState()

VARIANTS_PER_INT = 2 ** 32
BYTE_TO_LETTER_TRANSLATION_TABLE = (string.ascii_letters * 5)[:256]

HTML_PREFIXES = "<\\"


class UnicodeErrorAfterDecode(Exception):
    def __init__(self, regex, rpart, cause=None):
        super(UnicodeErrorAfterDecode, self).__init__(
            'Decoding exception substituting in already decoded body: %s; replace: %s : %s' + str(cause), regex.pattern, rpart)
        self.cause = cause


# we need this because browser strange behaviour https://st.yandex-team.ru/ANTIADB-985
HTML_ENTITIES_TO_AVOID_RE = re2.compile(r'&({})'.format(re_merge([char for char, code in name2codepoint.iteritems() if code < 256])), re2.IGNORECASE)
MIMETYPES_LIKE_PARTS_RE = re2.compile(r'\.((?:{})(?:[^a-zA-Z\d]|\b))'.format(re_merge([re2.escape(mtype.lstrip('.')) for mtype in types_map])), re2.IGNORECASE)
IMAGE_EXTENTIONS = [ext.lstrip('.') for ext, mtype in types_map.items() if mtype.split('/', 1)[0] == 'image']
PARTNER_IMAGE_URLS_PATTERNS = [
    r'avatars\.mds\.yandex\.net/.*',
    r'im\d\-tub\-(?:ru|by|ua|kz|com)\.yandex\.net/.*',  # хранилище Яндекс Картинок
]
IMAGE_EXTENTIONS_PATTERN = r'.*?\.(?:{})(?:$|\?.*)'.format(re_merge(IMAGE_EXTENTIONS))  # matches on urls like '*.jpg', '*.jpg?*'
IMAGE_EXTENTIONS_RE = re2.compile(IMAGE_EXTENTIONS_PATTERN, re2.IGNORECASE)
PARTNER_IMAGE_URLS_RE = re2.compile(re_merge((re_completeurl(PARTNER_IMAGE_URLS_PATTERNS, True), IMAGE_EXTENTIONS_PATTERN)), re2.IGNORECASE)
PARTNER_IMAGE_URLS_RE_MATCH = re2.compile(re_merge((re_completeurl(PARTNER_IMAGE_URLS_PATTERNS, True, match_only=True), IMAGE_EXTENTIONS_PATTERN)), re2.IGNORECASE)

# Регулярное выражение для поиска inline js скриптов
# Здесь мы ожидаем любое количество аттрибутов со значением аля (type="" src="")
# Но не селектим скрипты со standalone аттрибутами типа async, crossorigin и тд, тк их и не должно быть в inline скриптах
JS_TAG_REGEXP = \
    re2.compile(r'(?:<|\\u003c|\\x3c)script(?P<attrs> (?:\w+=\\?"[^>]+\\?"))?(?P<add_nonce>)(?:>|\\u003e|\\x3e)(?P<script>(?:<!--|[^<])[\s\S]*?)(?:<|\\u003c|\\x3c)/script(?:>|\\u003e|\\x3e)',
                re2.IGNORECASE)
# Типы inline-скриптов, которые поддерживаются функцией шифрования inlinе-скриптов
JS_INLINE_TYPES = ['application/javascript', 'text/javascript', 'application/x-javascript', 'application/ecmascript', 'text/ecmascript']

SCRIPT_TEMPLATE = r'''<script {NONCE}type="text/javascript"{ATTRS}>
        (function(){{
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","{SRC}",!0);
            xhr.onload=function(){{var a=document.createElement("{TAG_TYPE}");{TYPE}
            {NONCE_2}a.innerHTML=xhr.responseText;document.head.appendChild(a)}};xhr.send();
        }})();{SCRIPT_REMOVER}
        </script>'''.replace(' ' * 4, '').replace('\n', '')

SCRIPT_TEMPLATE_SYNC = r'''<script {NONCE}type="text/javascript"{ATTRS}>
        (function(){{
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","{SRC}",!1);
            xhr.send();
            document.write('<{TAG_TYPE}{NONCE_2}>'+xhr.responseText+'</'+'{TAG_TYPE}>');
        }})();{SCRIPT_REMOVER}
        </script>'''.replace(' ' * 4, '').replace('\n', '')

SCRIPT_REMOVER = ";var cs=document.currentScript;cs&&cs.id!=='butterfly'&&cs.parentElement&&cs.parentElement.removeChild(cs);"


class CryptUrlPrefix(object):
    """
    Provides object with attributes for generating URLs crypting prefix.
    """

    def __init__(self, scheme, host, seed, prefix, second_domain=None):
        self.seed = bytes(seed)
        self.scheme = scheme
        # один раз на запрос формируем регулярку для проверки урла на "зашифрованность"
        # TODO: Find another way to check this, without manual support
        self.is_crypted_re = re2.compile(r'(?:https?:)?//(?:{host}){prefix}[A-Za-z\d]{{{url_len_prefix}}}/{seed}'.format(host=host if second_domain is None else re_merge([host, second_domain]),
                                                                                                                         prefix=prefix,
                                                                                                                         url_len_prefix=URL_LENGTH_PREFIX_LENGTH,
                                                                                                                         seed=seed))
        self.common_crypt_prefix = ParseResult(scheme=scheme, netloc=host, path=prefix, params='', query='', fragment='')

    def crypt_prefix(self, scheme=None, prefix=None):
        link_crypt_prefix = self.common_crypt_prefix
        if scheme == '':
            link_crypt_prefix = link_crypt_prefix._replace(scheme=scheme)
        if prefix is not None:
            link_crypt_prefix = link_crypt_prefix._replace(path=prefix)
        return bytes(link_crypt_prefix.geturl())

    def is_crypted(self, url):
        return self.is_crypted_re.match(url) is not None


def mix_string_using_template(crypted_string, seed, crypted_url_mixing_template, step=CRYPTED_URL_MIXING_STEP):
    """
        Divide crypted_string to parts with length = step and insert non BASE64 symbols according to template.
        >>> mix_string_using_template('solongbase64stringneedtobexedwithmae-xxxx-stobesimilartoquerystringasdasdas', 'my2007', (('/', 100),))
        'so/longbase64s/tringne/edtobe/xedwi/thmae-x/xxx-stobes/imila/rtoquery/stringasdasdas'
        >>> mix_string_using_template('solongbase64stringneedtobexedwithmae-xxxx-stobesimilartoquerystringasdasdas', 'my2007', (('/', 1), ('!', 30)))
        'so/longbase64s!tringne!edtobe!xedwi!thmae-x!xxx-stobes!imila!rtoquery!stringasdasdas'

        В итоговой строке не должны встречаться HTML Character Entities: "&cent" заменяется на "cent"
        >>> mix_string_using_template('solongbase64stringneedtobexedwwcent-stobesimilartoquerystringasdasdas', 'my2007', (('/', 1), ('?', 1), ('&=', 4)))
        'so/longbase64s?tringne&edtobe=xedwwcent-st=obesimilar&toque=rystringasdasdas'

        В итоговой строке не должны встречаться расширения файлов: ".jpeg" заменяется на ".=jpeg"
        >>> mix_string_using_template('solongbase64stringnpegobexedwithmae-cent-stobesimilartoquerysjpeg-gasdasdas', 'my2007', (('/..', 20),))
        'so/longbase64s.tringnp.egobex/edwit.hmae-ce.nt-stobesi/milar.toquerys.=jpeg-gasdasdas'
    """
    if len(crypted_string) < step:
        return crypted_string
    rs.seed(int(seed.encode('hex'), 16) % VARIANTS_PER_INT)  # Random function depends on seed
    position = 0
    string_parts = []
    symbols = ''.join([symbol[0] * symbol[1] for symbol in crypted_url_mixing_template])
    insert_positions = rs.randint(2, step, min(len(symbols), len(crypted_string)/step - 1))
    # rebuild string from equal-length chuncks randomly splitted with mixing_template symbols:
    for insert_position, symbol in zip(insert_positions, symbols):
        chunk = crypted_string[position:position + step]
        string_parts.extend([chunk[:insert_position], symbol, chunk[insert_position:]])
        position += step
    string_parts.append(crypted_string[position:])
    new_string = ''.join(string_parts)
    if '&' in ''.join([s[0] for s in crypted_url_mixing_template]):
        new_string = HTML_ENTITIES_TO_AVOID_RE.sub(r'\1', new_string)
    if '.' in ''.join([s[0] for s in crypted_url_mixing_template]):
        new_string = MIMETYPES_LIKE_PARTS_RE.sub(r'.=\1', new_string)
    return new_string


def crypt_number(number_to_crypt, seed, result_length):
    """
    Шифрует произвольное число в строку из чисел и букв, например `42 -> 4U6X33`
    :param number_to_crypt: число, которое надо зашифровать
    :param seed: seed для шифрования (используется в качестве соли)
    :param result_length: итоговая длина строки с зашифрованным числом
    """
    k, b = ord(seed[0]), ord(seed[-1])
    data = list(str(number_to_crypt * k + b))
    letters_count_to_add = result_length - len(data)
    if letters_count_to_add > 0:
        rs.seed(int(seed.encode('hex'), 16) % VARIANTS_PER_INT)  # Random function depends on seed
        random_letters = rs.bytes(letters_count_to_add).translate(BYTE_TO_LETTER_TRANSLATION_TABLE)
        insert_positions = rs.randint(0, len(data) - 1, letters_count_to_add)
        for l, p in zip(random_letters, insert_positions):
            data.insert(p, l)
    return b''.join(data)


def generate_hide_meta_args_header_name(seed, pattern):
    """
    Generates hide meta args http header name. Pattern is a string with letters and '.' symbols. Every '.' will exchanged
    with random symbol from string.ascii_letters

    >>> generate_hide_meta_args_header_name(seed="my2007", pattern="A.A.B")
    'ANASB'
    >>> generate_hide_meta_args_header_name(seed="my2007", pattern=r'..a.a.b.m.e.t.a.a.r.g.s')
    'NSaXatbWmkedtXaSajrwgVs'
    >>> generate_hide_meta_args_header_name(seed="my2007", pattern=r'..a.a.b.g.r.a.b.a.r.g')
    'NSaXatbWgkrdaXbSajrwg'
    """
    rs.seed(int(seed.encode('hex'), 16) % VARIANTS_PER_INT)  # Random function depends on seed
    splitted = pattern.split('.')
    random_letters = rs.bytes(len(splitted) - 1).translate(BYTE_TO_LETTER_TRANSLATION_TABLE)
    return reduce(lambda res, (s1, s2): res + s1 + s2,
                  izip_longest(splitted, random_letters, fillvalue=''),
                  '')


def expand_crypted_url(url, seed, min_length):
    """
    Expand url param to the required length (min_length).
    >>> expand_crypted_url('shot', 'my2007', 0)
    'shot'
    >>> min_length = 100
    >>> expanded_url = expand_crypted_url('http://ya.ru/', 'my2007', min_length)
    >>> expanded_url
    'http://ya.ru/ NSXtWkdXSjwVklpbJjzIawKhVXHnbCZAzFrsQDUkKswJbCNEMtahXyzcFISZxWJtYapHJmXOFRqDJAVJkQyyUE'
    >>> abs(len(expanded_url) - min_length) <= 1
    True
    """
    if min_length > len(url):
        rs.seed(int(seed.encode('hex'), 16) % VARIANTS_PER_INT)  # Random function depends on seed
        rest_length = min_length - len(url) - 1  # 1 - space length
        random_part = rs.bytes(rest_length).translate(BYTE_TO_LETTER_TRANSLATION_TABLE) if rest_length > 0 else ''
        url += " " + random_part
    return url


def crypt_url(binurlprefix, binurl, key, enable_trailing_slash=False, min_length=0, crypted_url_mixing_template=DEFAULT_URL_TEMPLATE, origin=None):
    # Добавляем в урл origin партнера
    origin = origin or binurlprefix.common_crypt_prefix.netloc
    enable_trailing_slash = enable_trailing_slash or urlparse(binurl).path.endswith('/')
    binurl += '__AAB_ORIGIN{}__'.format(origin)
    # шифруем урл, если нужно то предварительно удлинняем
    crypted_part = encrypt_xor(expand_crypted_url(binurl, binurlprefix.seed, min_length) if min_length != 0 else binurl, key)
    # шифруем длину полученого выше зашифрованного урла в последовательность символов
    crypted_url_length = crypt_number(len(crypted_part), binurlprefix.seed, URL_LENGTH_PREFIX_LENGTH)
    # собираем префикс зашифрованного урла
    cryptprefix = binurlprefix.crypt_prefix(urlparse(binurl).scheme) + crypted_url_length + b'/' + binurlprefix.seed
    # размешиваем шифрованную часть символами и проверяем что в зашифрованном урле нету последовательностей их общих правил, если есть - убираем
    crypted_part = rule_evade_step_encode(mix_string_using_template(crypted_part, binurlprefix.seed, crypted_url_mixing_template))

    return cryptprefix + crypted_part + (b'/' if enable_trailing_slash else b'')


def body_crypt(body,
               binurlprefix,
               key,
               crypt_url_re,
               enable_trailing_slash,
               file_url=b'',
               min_length=0,
               crypted_url_mixing_template=DEFAULT_URL_TEMPLATE,
               image_urls_crypting_ratio=1,
               partner_url_re_match=None,
               image_to_iframe_url_re_match=None,
               image_to_iframe_url_length=0,
               image_to_iframe_url_is_relative=False,
               image_to_iframe_changing_ratio=1,
               config=None,
               partner_cookieless_url_re_match=None,
               bypass_url_re_match=None,
               ):
    """
    :param body: text you want to crypt
    :param binurlprefix: site prefix object for a crypted part, for ex. http://auto.ru/<cry_part>
    :param key: key to crypt all urls that we found
    :param crypt_url_re: regex to find urls in body
    :param enable_trailing_slash: if True, then add trailing slash to the end of crypted url
    :param file_url: for relative url crypting ONLY! use full file url to fix relative urls in body text
    :param min_length: min crypted url length, if less we expand it by some symbols
    :param crypted_url_mixing_template: if not None mixing crypted part according to template
    :param image_urls_crypting_ratio: if < 1 we will crypt only image_urls_crypting_ratio * 100% of image urls
    :param partner_url_re_match: if image_urls_crypting_ratio < 1 we will use it to find partner image urls
    :param image_to_iframe_url_re_match: image regex by marking url with query args, to change img tag to iframe on client. https://st.yandex-team.ru/ANTIADB-1250
    :param image_to_iframe_url_length: another one url-length-related, need this to turn on img->iframe feature when all {300,} subdocument links blocked
    :param image_to_iframe_url_is_relative: if True we img urls in iframe src will be relative
    :param image_to_iframe_changing_ratio: if < 1 we will change img tag to iframe only on image_to_iframe_changing_ratio * 100% of images suitable for image_to_iframe_url_re
    :param config: partner config
    :param partner_cookieless_url_re_match: паттерн ссылок партнера которые будут под найдекс зашифрованы
    :param bypass_url_re_match: паттерн ссылок которые не будут зашифрованы
    :return: encrypted body
    """
    if ENV_TYPE in ['testing'] or END2END_TESTING:
        random_obj.seed(int(binurlprefix.seed.encode('hex'), 16))  # for "test_partner_images_random_crypting" test and end2end testing in Sandbox

    binurlprefix_cookieless = None
    if config is not None:
        binurlprefix_cookieless = CryptUrlPrefix(scheme=binurlprefix.scheme, host=config.second_domain, seed=binurlprefix.seed,
                                                 prefix=config.cookieless_path_prefix,
                                                 second_domain=binurlprefix.common_crypt_prefix.netloc)

    def skip_img_crypting(link):
        """
        Для уменьшения нагрузки на прокси часть партнерских ссылок на картинки мы можем не шифровать (% устанавливается в конфиге)
        https://st.yandex-team.ru/ANTIADB-1035
        """
        return image_urls_crypting_ratio < 1 and \
               random_obj.random() > image_urls_crypting_ratio and \
               partner_url_re_match is not None and \
               partner_url_re_match.match(link) and \
               PARTNER_IMAGE_URLS_RE_MATCH.match(link)

    def should_change_to_iframe(link):
        """
        На большинстве площадок не iframe используются только в рекламе, поэтому мы оборачиваем ими картинки
        https://st.yandex-team.ru/ANTIADB-1250
        """
        return image_to_iframe_url_re_match is not None and \
               (image_to_iframe_changing_ratio == 1 or random_obj.random() < image_to_iframe_changing_ratio) and \
               image_to_iframe_url_re_match.match(link)

    def crypt_func(m):
        for group in m.groups():
            if group:
                # Skipping url crypting if we expecting relative url but its not
                # or if url starts with crypting prefix (binurlprefix), cause its mean that url is already crypted
                if file_url and any(group.lower().startswith(url_prefix) for url_prefix in ['http://', 'https://', '//', 'data:', 'mailto:', 'tel:', 'skype:']):
                    return m.group()
                if binurlprefix.is_crypted(group) or binurlprefix_cookieless is not None and binurlprefix_cookieless.is_crypted(group):
                    return m.group()
                if skip_img_crypting(group):
                    return m.group()

                joined_url = urljoin(file_url, group)
                # excluding urls
                # https://st.yandex-team.ru/ANTIADB-1758, https://st.yandex-team.ru/ANTIADB-1952
                if bypass_url_re_match is not None and bypass_url_re_match.match(joined_url):
                    return m.group().replace(group, joined_url)

                # определяем хост под который шифруем, None - первоначальный, иначе - тот под который шифруем, он высокоприоритетнее чем crypted_host
                result_binurlprefix = binurlprefix
                origin = None
                if config is not None and config.ENCRYPT_TO_THE_TWO_DOMAINS:
                    is_partner_static_url = (partner_cookieless_url_re_match is not None and partner_cookieless_url_re_match.match(joined_url))
                    is_yandex_static_url = (config.yandex_static_url_re_match is not None and config.yandex_static_url_re_match.match(joined_url))
                    if is_yandex_static_url or is_partner_static_url:
                        origin = binurlprefix.common_crypt_prefix.netloc
                        result_binurlprefix = binurlprefix_cookieless

                if should_change_to_iframe(group):
                    crypted_url = crypt_url(result_binurlprefix, joined_url, key, enable_trailing_slash, image_to_iframe_url_length, crypted_url_mixing_template, origin)
                    if image_to_iframe_url_is_relative:
                        crypted_url = urlparse(crypted_url)._replace(scheme='', netloc='').geturl()
                    crypted_url += IMAGE_TO_IFRAME_CRYPTING_URL_FRAGMENT
                else:
                    crypted_url = crypt_url(result_binurlprefix, joined_url, key, enable_trailing_slash, min_length, crypted_url_mixing_template, origin)
                return m.group().replace(group, crypted_url)
        return m.group()

    if file_url and config is not None and (binurlprefix.is_crypted(file_url) or binurlprefix_cookieless is not None and binurlprefix_cookieless.is_crypted(file_url)):
        try:
            decrypted = decrypt_url(urlparse(file_url.encode('utf-8'))._replace(scheme='', netloc='').geturl(), str(config.CRYPT_SECRET_KEY),
                                    str(config.CRYPT_PREFFIXES), config.CRYPT_ENABLE_TRAILING_SLASH)[0]
            if decrypted is not None:
                file_url = decrypted.decode("utf-8")
        except DecryptionError:
            # сюда попадаем если ссылка сшита неправильно, например, относительная, и похожа на зашифрованную
            pass

    result = crypt_url_re.sub(crypt_func, body)
    return result


LOWERCASE_ALPHABET = string.ascii_lowercase + string.digits


def body_replace(body, key, replace_dict, count=0, charset=None, crypt_in_lowercase=False, url=None, logger=None):
    """
    :param body: text you want to crypt
    :param key: key to crypt all classes that we found
    :param replace_dict: dict where key is pattern to find, and value is None to crypt matched group or string to replace it
    :param count: is the maximum number of each pattern occurrences to be replaced, 0-all occurrences will be replaced
    :param charset: body charset
    :param crypt_in_lowercase: generate crypt sequence in lowercase
    :return: body with replaced values
    """
    def gen_crypt_func(r_part):
        def encrypt_text(text, crypt_in_lowercase):
            crypt_prefix = chr(ord(key[0]) % 26 + ord('a'))
            if crypt_in_lowercase:
                random.seed(long((text + key).encode('hex'), 16))
                return crypt_prefix + "".join([random.choice(LOWERCASE_ALPHABET) for _ in xrange(len(text))])
            else:
                # Escape `minus` to avoid evaluation by javascript, see https://st.yandex-team.ru/ANTIADB-338
                return crypt_prefix + encrypt_xor(text, key).replace('-', 'a')

        def crypt_func(m):
            if logger is not None:
                logger.info('', action=REGEX_STAT, url=url, regex=str(regex))
            if m.lastindex is not None:
                return replace_matched_group(m, m.lastindex, r_part if r_part is not None else encrypt_text(m.group(m.lastindex), crypt_in_lowercase))
            else:
                return r_part if r_part is not None else encrypt_text(m.group(), crypt_in_lowercase)
        return crypt_func

    decoded = False  # Флаг "body уже задекожено"
    for regex, rpart in replace_dict.items():
        # Делать decode/encode безусловно для всех запросов слишком дорого
        # время выполнения функции вырастает в 100 раз, потому сделано через try/except
        try:
            body = regex.sub(repl=gen_crypt_func(rpart), string=body, count=count)
        except (UnicodeDecodeError, TypeError) as e:
            if isinstance(e, TypeError) and "an integer or string of size 1 is required" not in str(e):
                raise
            if decoded:
                raise UnicodeErrorAfterDecode(regex, rpart, e), None, sys.exc_info()[2]
            charset = charset if charset is not None else 'utf-8'
            try:
                body = body.decode(charset)
            # Если мы свалились с попыткой декодить текст кодировкой из заголовка, то пробуем определить ее через chardet
            except UnicodeDecodeError:
                body = body.decode(chardet.detect(body)['encoding'] or 'utf-8', errors='ignore')
            decoded = True
            body = regex.sub(gen_crypt_func(rpart), body, count)

    return body.encode(charset, errors='replace') if decoded else body  # некорректные символы будут заменены на '?'


def encrypt_js_body_with_xor(body, seed, charset=None):
    """
    :param body: javascript body
    :param seed: seed using for xoring body
    :param charset: Charset from 'Content-Type' response header
    :return: crypted javascript body
    """
    # JS функция decodeURIComponent работает только с utf-8
    if charset != 'utf-8':
        try:
            body = body.decode(charset if charset else 'utf-8').encode('utf-8')
        except (UnicodeDecodeError, UnicodeEncodeError):
            body = body.decode(chardet.detect(body)['encoding']).encode('utf-8')

    body = quote(body)

    try:
        _body = unicode(body).encode('unicode-escape')
        crypted_body = b64encode(crypto_xor(_body, seed))
    except UnicodeDecodeError:
        if charset is not None:
            try:
                _body = unicode(body, charset).encode('unicode-escape')
                crypted_body = b64encode(crypto_xor(_body, seed))
            # Если мы свалились с попыткой декодить текст кодировкой из заголовка, то пробуем определить ее через chardet
            except UnicodeDecodeError:
                _body = unicode(body, chardet.detect(body)['encoding']).encode('unicode-escape')
                crypted_body = b64encode(crypto_xor(_body, seed))
        else:
            _body = unicode(body, chardet.detect(body)['encoding']).encode('unicode-escape')
            crypted_body = b64encode(crypto_xor(_body, seed))
    return crypted_body


CRYPTED_JS_TEMPLATE = """eval(decodeURIComponent({xor_func}(atob(`{body}`),`{seed}`)));
function {xor_func}(data,key){{var result=[];for(var i=0;i<data.length;i++){{var xored=data.charCodeAt(i)^key.charCodeAt(i%key.length);result.push(String.fromCharCode(xored));}}
return result.join(``);}}{script_remover}""".replace('\r', '').replace('\n', '')


def crypt_js_body(body, seed, charset=None, remove_after_run=False):
    """
    https://st.yandex-team.ru/ANTIADB-1032
    :param body: javascript body
    :param seed: seed using for xoring body
    :param charset: Charset from 'Content-Type' response header
    :param remove_after_run: add part to js script that will remove it from the DOM after it is run
    :return: crypt javascript body and returns js code that evaling that crypted body
    """
    crypted_func_name = encrypt_xor('xor', seed)

    crypted_body = encrypt_js_body_with_xor(body, seed, charset)

    return CRYPTED_JS_TEMPLATE.format(xor_func=crypted_func_name, body=crypted_body, seed=seed, script_remover=SCRIPT_REMOVER if remove_after_run else "")


def crypt_inline_js(body, seed, charset=None, do_encode=False, remove_after_run=False):
    """
    https://st.yandex-team.ru/ANTIADB-1032
    :param body: html body where we need to crypt js script
    :param seed: seed to use with `crypt_js_body` function
    :param do_encode: do encode in case of json
    :param remove_after_run: add part to js script that will remove it from the DOM after it is run
    :return: body with crypted inline js scripts
    """
    def crypt_func(m):
        # Шифруем скрипты с пустым типом или из JS_INLINE_TYPES, кроме скриптов содержащих в теле только '\n'
        if (m.group('attrs') is None or ' type=' not in m.group('attrs') or m.group('attrs').rsplit(' type=', 1)[1].split()[0].strip('\\"') in JS_INLINE_TYPES) and m.group('script').strip() != '':
            new_script_part = crypt_js_body(m.group('script').decode('unicode_escape') if do_encode else m.group('script'), seed, charset, remove_after_run)
            return replace_matched_group(m, 'script', new_script_part.encode('unicode_escape').replace('"', '\\"') if do_encode else new_script_part)
        else:
            return m.group()

    return JS_TAG_REGEXP.sub(crypt_func, body)


def add_nonce_to_scripts(body, nonce):
    """
    https://st.yandex-team.ru/ANTIADB-2203
    :param body: html body where we need to add nonce in inline js
    :return: body with inserted nonce
    """

    def insert_nonce(m):
        #  Вставляем nonce в скрипты (стили) с пустым типом или из JS_INLINE_TYPES и без nonce
        attrs = m.group('attrs') or ''
        if ' nonce=' not in attrs:
            if (not attrs or ' type=' not in attrs or attrs.rsplit(' type=', 1)[1].split()[0].strip('\\"') in JS_INLINE_TYPES) and m.group('script').strip() != '':
                return replace_matched_group(m, 'add_nonce', ' nonce="{}"'.format(nonce))
        return m.group()

    return JS_TAG_REGEXP.sub(insert_nonce, body)


def replace_resource_with_xhr(body, regexp, nonce_js, nonce_css, remove_after_run=False, sync=True):
    """
    https://st.yandex-team.ru/ANTIADB-1255
    :param body: html body where we need to replace script with xhr
    :param regexp: regular expression for matched script with src
    :param nonce_js: header["nonce"] for javascripts
    :param nonce_css: header["nonce"] for stylesheets
    :param remove_after_run: add part to js script that will remove it from the DOM after it is run
    :param sync: send xhr sync or async
    :return: body with replaced script by xhr
    """

    def replace_resource(m):
        SCRIPT = SCRIPT_TEMPLATE_SYNC if sync else SCRIPT_TEMPLATE
        attrs = ' '.join(not_none([m.group('attrs1'), m.group('attrs2')]))
        if ' type' not in attrs:
            type_script = "text/javascript"
        else:
            old_type_value = attrs.rsplit(' type=', 1)[1].split()[0]
            type_script = old_type_value.strip('\\"') or "text/javascript"
            attrs = attrs.replace(' type=' + old_type_value, '')
        nonce = nonce_css if m.group('style') else nonce_js
        new_script = SCRIPT.format(NONCE="nonce=\"{}\" ".format(nonce_js) if nonce_js else "",
                                   NONCE_2=(" nonce=\"{}\"".format(nonce) if sync else "a.setAttribute(\"nonce\", \"{}\");".format(nonce)) if nonce else "",
                                   TAG_TYPE=m.group('style') or "script",  # style если это стиль, или скрипт, если скрипт
                                   TYPE="a.type=\"{}\";".format(type_script) if type_script and m.group('style') is None and not sync else "",
                                   SRC=m.group('src_style') if m.group('style') else m.group('src_script'),
                                   ATTRS=attrs.rstrip(),
                                   SCRIPT_REMOVER=SCRIPT_REMOVER if remove_after_run else "")
        return new_script

    return regexp.sub(replace_resource, body)


def close_tags(body, tags_regex):
    """
    Close all tags in document, exclude void elements
    <p/> -> <p></p>
    :param body: text to correct tags
    :param tags_regex: regexp to find tags
    :return: body with closed tags
    """

    def close_tags_func(m):
        if m.group(1) not in VOID_HTML_ELEMENTS:
            return m.group()[:-2] + b'></{}>'.format(m.group(1))
        return m.group()

    return tags_regex.sub(close_tags_func, body)


def meta_html_b64_repack(meta_body, config, binurlprefix, key):
    """
    https://st.yandex-team.ru/ANTIADB-899
    Ищем в ответе от RTB хоста поле html, в котором лежит base64encoded реклама
    Если такого поля нет, считаем, что в ответе лежит код баннеров: https://st.yandex-team.ru/ANTIADB-1262
    Декодим, обрабатываем, шифруем содержимое и енкодим обратно

    :param meta_body: содержимое ответа от РТБ хоста в формате python dict или строка для новых ответов
    :param config: конфиг партнера
    :param binurlprefix: текущий binurlprefix запроса
    :param key: текущий key для шифрования
    :return: входной словарь с перешифрованным содержимым поля html
    """
    if isinstance(meta_body, dict):
        # Баннер нужно достать из словаря, пошифровать и упаковать обратно в base64
        return base64_html_repack(meta_body, binurlprefix, config, key)
    else:
        # Это уже раздекоженный из base64 баннер, надо просто пошифровать его
        return crypt_decoded_banner(meta_body, binurlprefix, config, key)


def crypt_decoded_banner(banner, binurlprefix, config,  key, base_path=""):
    # шифруем контент
    if config.replace_body_with_tags_re is not None:
        banner = body_replace(body=banner,
                              key=key,
                              replace_dict=config.replace_body_with_tags_re,
                              crypt_in_lowercase=config.CRYPT_IN_LOWERCASE)
    # Шифруем сперва относительные урлы в html5 креативах canvas-а
    if not base_path:
        base_path = config.html5_base_path_re.search(banner)
        if base_path is not None:
            base_path = base_path.group(1)

    if base_path:
        banner = body_crypt(body=banner,
                            binurlprefix=binurlprefix,
                            key=key,
                            crypt_url_re=config.crypt_html5_relative_url_re,
                            enable_trailing_slash=False,
                            file_url=base_path,
                            min_length=config.RAW_URL_MIN_LENGTH,
                            crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
                            config=config,
                            )
    # шифруем остальное
    banner = body_crypt(body=banner,
                        binurlprefix=binurlprefix,
                        key=key,
                        crypt_url_re=config.crypt_url_re_with_counts,
                        enable_trailing_slash=config.CRYPT_ENABLE_TRAILING_SLASH,
                        min_length=config.RAW_URL_MIN_LENGTH,
                        crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
                        config=config,
                        )
    if config.crypt_body_re is not None:
        banner = body_replace(body=banner,
                              key=key,
                              replace_dict={config.crypt_body_re: None},
                              crypt_in_lowercase=config.CRYPT_IN_LOWERCASE,
                              )
    return banner


def base64_html_repack(meta_body_dict, binurlprefix, config, key):
    if "rtb" in meta_body_dict:
        rtb = meta_body_dict["rtb"]
        # Удаление поля `url` заставляет PCODE брать рекламный контент из поля html вместо того, чтобы получать его, дернув ссылку из поля `url`
        rtb.pop("url", None)
        if "html" in rtb:
            html = rtb["html"].strip()
            is_smart_banner_ssr = rtb.get("isSmartBanner", False)
            # get basePath
            base_path = rtb.get("basePath", "")
            # https://st.yandex-team.ru/ANTIADB-2602 отрываем base64 в смарт баннерах (SSR)
            # Это костыль https://st.yandex-team.ru/ANTIADB-1578 потому что на стороне БК захардкожено НЕ отдавать морде html в base64
            if is_smart_banner_ssr or html and html[0] in HTML_PREFIXES:
                rtb["html"] = crypt_decoded_banner(html, binurlprefix, config, key, base_path)
            else:
                html = urlsafe_b64decode(html.encode("utf-8"))
                html = crypt_decoded_banner(html, binurlprefix, config, key, base_path)
                rtb["html"] = urlsafe_b64encode(html)
        # https://st.yandex-team.ru/ANTIADB-2998
        elif "ssr" in rtb:
            encoding = rtb["ssr"].get("encoding", "raw")
            rtb["ssr"] = ssr_fields_repack(rtb["ssr"], binurlprefix, config, key, encoding)
    else:
        # crypt ssr fields https://st.yandex-team.ru/ANTIADB-2366, https://st.yandex-team.ru/ANTIADB-2566, https://st.yandex-team.ru/ANTIADB-2759
        ssr_dict = {}
        if "seatbid" in meta_body_dict:
            # widget SSR
            if "ssr" in meta_body_dict.get("settings", {}):
                ssr_dict = meta_body_dict["settings"]["ssr"]
            else:
                for block_id in meta_body_dict.get("settings", {}).keys():
                    if check_key_in_dict(meta_body_dict["settings"][block_id], "ssr"):
                        ssr_dict = meta_body_dict["settings"][block_id]["ssr"]
                        break
            encoding = "URI"
        else:
            # PCODE SSR
            ssr_dict = meta_body_dict.get("direct", {}).get("ssr", {})
            encoding = ssr_dict.get("encoding", "URI")
        if ssr_dict:
            if encoding not in ("URI", "base64"):
                raise Exception("Unknown encoding: {}".format(encoding))
            for field in ("html", "css"):
                if field not in ssr_dict:
                    continue
                content = ssr_dict[field]
                content = urlsafe_b64decode(content.encode("utf-8")) if encoding == "base64" else unquote(content.encode("utf-8"))
                content = crypt_decoded_banner(content, binurlprefix, config, key)
                ssr_dict[field] = urlsafe_b64encode(content) if encoding == "base64" else quote(content, safe="-_.!~*'()")
    return meta_body_dict


def smartbanner_json_data_crypt(meta_body, config, binurlprefix, key):
    """
    https://st.yandex-team.ru/ANTIADB-2581
    В ответе от RTB хоста для смарт баннера есть поле data, в котором лежит json с рекламой
    Нужно пошифровать только каунтовые ссылки, так как другие ссылки пошифровались на более ранних этапах

    :param meta_body: содержимое ответа от РТБ хоста в формате python dict
    :param config: конфиг партнера
    :param binurlprefix: текущий binurlprefix запроса
    :param key: текущий key для шифрования
    :return: входной словарь с перешифрованным содержимым поля html
    """
    data_params = meta_body["rtb"]["data"].get("AUCTION_DC_PARAMS", {}).get("data_params", {})
    for params_key in data_params.keys():
        if not isinstance(data_params[params_key], dict):
            continue
        if "count" in data_params[params_key]:
            url = data_params[params_key]["count"]
            data_params[params_key]["count"] = crypt_url(binurlprefix, url, key,
                                                         config.CRYPT_ENABLE_TRAILING_SLASH,
                                                         config.RAW_URL_MIN_LENGTH,
                                                         config.CRYPTED_URL_MIXING_TEMPLATE)
        if "click_url" in data_params[params_key]:
            for url_key, url in data_params[params_key]["click_url"].items():
                data_params[params_key]["click_url"][url_key] = crypt_url(binurlprefix, url, key,
                                                                          config.CRYPT_ENABLE_TRAILING_SLASH,
                                                                          config.RAW_URL_MIN_LENGTH,
                                                                          config.CRYPTED_URL_MIXING_TEMPLATE)
    return meta_body


def meta_video_b64_repack(meta_body, config, binurlprefix, key):
    """
    https://st.yandex-team.ru/ANTIADB-2995
    Ищем в ответе от RTB хоста поле vast, vastBase64, video (для последних двух содержимое base64encoded)
    Декодим, обрабатываем, шифруем содержимое и енкодим обратно

    :param meta_body: содержимое ответа от РТБ хоста в формате python dict
    :param config: конфиг партнера
    :param binurlprefix: текущий binurlprefix запроса
    :param key: текущий key для шифрования
    :return: входной словарь с перешифрованным содержимым поля с vast, а также rtb.ssr
    """
    meta_body['rtb'] = base64_video_repack(meta_body['rtb'], binurlprefix, config, key)
    return meta_body


def crypt_decoded_vast(vast, binurlprefix, config,  key):
    # шифруем контент
    if config.replace_body_with_tags_re is not None:
        vast = body_replace(body=vast,
                            key=key,
                            replace_dict=config.replace_body_with_tags_re,
                            crypt_in_lowercase=config.CRYPT_IN_LOWERCASE)

    # шифруем остальное
    vast = body_crypt(body=vast,
                      binurlprefix=binurlprefix,
                      key=key,
                      crypt_url_re=config.crypt_url_re_with_counts,
                      enable_trailing_slash=config.CRYPT_ENABLE_TRAILING_SLASH,
                      min_length=config.RAW_URL_MIN_LENGTH,
                      crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
                      config=config,
                      )
    if config.crypt_body_re is not None:
        vast = body_replace(body=vast,
                            key=key,
                            replace_dict={config.crypt_body_re: None},
                            crypt_in_lowercase=config.CRYPT_IN_LOWERCASE,
                            )
    return vast


def base64_video_repack(rtb, binurlprefix, config, key):
    vast = rtb.get("vast", "").strip()
    vast_base64 = rtb.get("vastBase64", "").strip()
    video = rtb.get("video", "").strip()
    if vast:
        rtb["vast"] = crypt_decoded_vast(vast, binurlprefix, config, key)
    elif vast_base64:
        vast_base64 = urlsafe_b64decode(vast_base64.encode("utf-8"))
        vast_base64 = crypt_decoded_vast(vast_base64, binurlprefix, config, key)
        rtb["vastBase64"] = urlsafe_b64encode(vast_base64)
    elif video:
        video = urlsafe_b64decode(video.encode("utf-8"))
        video = crypt_decoded_vast(video, binurlprefix, config, key)
        rtb["video"] = urlsafe_b64encode(video)

    ssr_dict = rtb.get("ssr", {})
    if ssr_dict and isinstance(ssr_dict, dict):
        encoding = ssr_dict.get("encoding", "raw")
        rtb["ssr"] = ssr_fields_repack(ssr_dict, binurlprefix, config, key, encoding)
    return rtb


def ssr_fields_repack(ssr_dict, binurlprefix, config, key, encoding):
    if encoding not in ("URI", "base64", "raw"):
        raise Exception("Unknown encoding: {}".format(encoding))
    for field in ("html", "css"):
        if field not in ssr_dict:
            continue
        content = ssr_dict[field]
        if encoding != "raw":
            content = urlsafe_b64decode(content.encode("utf-8")) if encoding == "base64" else unquote(content.encode("utf-8"))
        content = crypt_decoded_banner(content, binurlprefix, config, key)
        if encoding != "raw":
            ssr_dict[field] = urlsafe_b64encode(content) if encoding == "base64" else quote(content, safe="-_.!~*'()")
        else:
            ssr_dict[field] = content
    return ssr_dict


def get_script_key(key, seed, functime=time):
    """
    Функция генерации значения script_key для передачи в скрипт детекта
    В значении шифруем текущий момент времени для последующей валидации
    """
    rs.seed(int(seed.encode('hex'), 16) % VARIANTS_PER_INT)  # Random function depends on seed
    data = '{salt} {time}'.format(
        salt=rs.bytes(rs.random_integers(5, 10)).translate(BYTE_TO_LETTER_TRANSLATION_TABLE),
        time=int(functime()),
    )
    result = seed + encrypt_xor(data, key)
    # сплитим результат на 2 части случайным образом
    n = random.randint(1, len(result) - 1)
    return result[:n], result[n:]


def validate_script_key(script_key, partner_secret_key, ttl=900):
    """
    Валидация значения script_key, проверяем что с момента генерации прошло не более ttl секунд
    Возвращаем кортеж (результат валидации, сообщение об ошибке)
    """
    try:
        seed, data = script_key[:SEED_LENGTH], script_key[SEED_LENGTH:]
        key = get_key(partner_secret_key, seed)
        _, decrypted_time = decrypt_xor(str(data), key).split()
        if abs(time() - int(decrypted_time)) <= ttl or ENV_TYPE == 'load_testing':
            return True, ''
        return False, 'expired script key'
    except Exception as e:
        return False, 'fail to decrypt script key, exception: {}'.format(str(e))


def remove_all_placeholders_except_one(body, regex):
    """
    :param body: тело ответа с плейсхолдерами
    :param regex: регулярка для поиска плейсхолдеров c одной захватывающей группой,
    например, r'\b[a-z]=\"__scriptKey([01])Value__\"[;,]?'
    :return: тело ответа с одним местом вхождения для уникального плейсхолдера
    Для каждого элемента placeholders удаляем все, кроме одного случайного вхождения
    """
    def get_placeholders_remover(number_of_placeholders):
        # счетчики вхождений плейсхолдеров
        placeholders_curr_number = {group: 0 for group in number_of_placeholders.keys()}
        # для каждого плейсхолдера генерим случайное место вхождения, которое будет оставлено
        random_places_of_placeholders = {group: random.randint(0, cnt - 1) for group, cnt in number_of_placeholders.items()}

        def remove_all_but_one(match):
            curr_placeholder = match.group(1)
            if placeholders_curr_number[curr_placeholder] == random_places_of_placeholders[curr_placeholder]:
                placeholders_curr_number[curr_placeholder] += 1
                return match.group(0)
            placeholders_curr_number[curr_placeholder] += 1
            return ""
        return remove_all_but_one

    try:
        number_of_placeholders = Counter(regex.findall(body))
        if max(number_of_placeholders.values()) > 1:
            remover = get_placeholders_remover(number_of_placeholders)
            body = regex.sub(remover, body)
        return body
    except Exception:
        return body


def crypted_functions_replace(config, ctx):
    seed = ctx.seed
    key = ctx.key
    binurlprefix = ctx.binurlprefix
    if config.ENCRYPT_TO_THE_TWO_DOMAINS:
        cookieless_url_prefix = urlunparse((binurlprefix.scheme, config.second_domain, config.cookieless_path_prefix, "", "", ""))
    else:
        cookieless_url_prefix = binurlprefix.crypt_prefix()
    url_prefix = cookieless_url_prefix

    if binurlprefix.common_crypt_prefix.netloc != config.second_domain:
        url_prefix = binurlprefix.crypt_prefix()

    is_encoded_url_regex = r'/^(?:https?:)?\/\/[^/]+?(?:{all_url_prefixes})\w{{9}}\/{seed}./'.format(all_url_prefixes=config.CRYPT_PREFFIXES, seed=seed)
    client_cookieless_pattern = re_merge(config.PARTNER_TO_COOKIELESS_HOST_URLS_RE + config.YANDEX_STATIC_URL_RE).replace('/', '\\/').replace('\\\\', '\\')
    # PARTNER PCODE CONFIG OBJECT (https://st.yandex-team.ru/ANTIADB-479, https://st.yandex-team.ru/ANTIADB-1876)
    crypted_functions = config.crypted_functions.replace('{seed}', seed) \
        .replace('{seedForMyRandom}', seed.encode('hex')) \
        .replace('{encode_key}', urlsafe_b64encode(key)) \
        .replace('{url_prefix}', url_prefix) \
        .replace('{is_encoded_url_regex}', is_encoded_url_regex) \
        .replace('{cookieless_url_prefix}', cookieless_url_prefix) \
        .replace('{client_cookieless_regex}', client_cookieless_pattern) \
        .replace('{aab_origin}', binurlprefix.common_crypt_prefix.netloc)

    if ctx.replace_body_re:
        crypted_functions = body_replace(crypted_functions, key, replace_dict=ctx.replace_body_re,
                                         charset=ctx.charset, crypt_in_lowercase=config.CRYPT_IN_LOWERCASE)
    if config.crypt_body_re:
        crypted_functions = body_replace(crypted_functions, key, replace_dict={config.crypt_body_re: None},
                                         charset=ctx.charset, crypt_in_lowercase=config.CRYPT_IN_LOWERCASE)
    return crypted_functions
