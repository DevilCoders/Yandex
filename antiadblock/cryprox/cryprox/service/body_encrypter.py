# coding=utf-8
import json
from base64 import urlsafe_b64encode, urlsafe_b64decode, b64encode, b64decode
from functools import partial
from time import strftime, gmtime
from monotonic import monotonic
from copy import deepcopy
from urlparse import parse_qsl
from collections import Counter
from enum import IntEnum
from urllib import quote, unquote

from antiadblock.cryprox.cryprox.common.cry import generate_seed
import antiadblock.cryprox.cryprox.common.visibility_protection as v_protect
from antiadblock.cryprox.cryprox.common.cryptobody import body_replace, body_crypt, crypt_inline_js, \
    crypt_js_body, add_nonce_to_scripts, close_tags, meta_html_b64_repack, replace_resource_with_xhr, \
    get_script_key, remove_all_placeholders_except_one, crypted_functions_replace, smartbanner_json_data_crypt, meta_video_b64_repack
from antiadblock.cryprox.cryprox.common.tools import regexp as regexp_tools
from antiadblock.cryprox.cryprox.common.tools.jsonify_meta import jsonify_meta
from antiadblock.cryprox.cryprox.common.tools.misc import duration_usec, add_second_domain_to_csp, add_nonce_to_csp, add_patch_to_csp, check_key_in_dict
from antiadblock.cryprox.cryprox.common.tools.url import UrlClass
from antiadblock.cryprox.cryprox.common.tools.experiments import is_experiment_date
from antiadblock.cryprox.cryprox.config import system as sys_config
from antiadblock.cryprox.cryprox.config.system import EncryptionSteps, InjectInlineJSPosition
from antiadblock.cryprox.cryprox.config.service import END2END_TESTING
from antiadblock.cryprox.cryprox.service import action

__JAVASCRIPT_TEXT_HTML_CONTENT_TYPES__ = ['text/javascript', 'application/javascript', 'application/x-javascript',
                                          'text/html']


class AdFoxErrorsNotEmptyException(Exception):
    pass


class BodyType(IntEnum):
    """
    Какому типу соответсвует body_part
    """
    UNDEFINED = 1
    PLAIN = 2
    URL = 3

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return str(self.value)


class EncryptAction:
    def __init__(self):
        pass


class EncryptionContext:

    def __init__(self, **kwargs):
        self.inner_step_duration = Counter()
        for key, value in kwargs.items():
            setattr(self, key, value)

    # noinspection PyAttributeOutsideInit
    def set_content_type(self, content_type):
        self.http_handler.content_type = content_type
        # noinspection PyProtectedMember
        self.http_handler._headers['Content-Type'] = content_type
        self.content_type = content_type


class ParsedBody:
    def __init__(self, ctx, parts):
        self.ctx = ctx
        self.config = ctx.config
        self.parts = parts
        self._duration = Counter()

    def execute_step(self, func, name=None):
        name = name or func.__name__
        self.ctx.inner_step_duration.clear()
        try:
            for part in self.parts:
                _, part_duration = duration_usec(partial(func, part, self.ctx, self.config))
                self._duration[name] += part_duration
        finally:
            for action_name, action_duration in self.ctx.inner_step_duration.items():
                self.ctx.logger.info(None, action=action_name, duration=action_duration)
                self.ctx.metrics.increase(action_name, duration=action_duration)
        return self

    def build_content(self):
        return ''.join(part.text for part in self.parts)

    def duration_exists(self, name):
        return name in self._duration

    def duration(self, name):
        return self._duration[name]


def parse_body(body, ctx):
    return ParsedBody(ctx, [BodyPart(BodyType.UNDEFINED, body)])


class BodyPart:
    def __init__(self, part_type, text):
        self.part_type = part_type
        self.text = text


def logged_step(action_name=None):
    """
    Calculate func operation time, log its result and increment metric counter
    :param action_name: action name to log
    """

    def inner_decorator(func):
        def payload(*args, **kwargs):
            ctx = kwargs.get('ctx')
            if ctx is None:
                for arg in args:
                    if isinstance(arg, EncryptionContext):
                        ctx = arg
                        break
            start = monotonic()
            try:
                return func(*args, **kwargs)
            finally:
                end = monotonic()
                duration = int((end - start) * 1000000)
                ctx.inner_step_duration[action_name] += duration

        return payload

    return inner_decorator


def step_condition(condition):
    """
    Calculate step condition, if condition matches then executes step and returns its result
    :param condition: function to be executed for check
    """

    def inner_decorator(func):
        def payload(*args, **kwargs):
            res = condition(*args, **kwargs)
            if res:
                return func(*args, **kwargs)

        return payload

    return inner_decorator


def html_js_body_replace_step(body_part, ctx, config):
    is_js_txt_html = ctx.content_type in __JAVASCRIPT_TEXT_HTML_CONTENT_TYPES__
    if is_js_txt_html and config.encryption_step_enabled(EncryptionSteps.tags_crypt):
        # Change tags usage in JS code (https://st.yandex-team.ru/ANTIADB-1123). Also for inline scripts
        body_part.text = body_replace(body=body_part.text, key=ctx.key, replace_dict=config.fix_js_tags_re,
                                      charset=ctx.charset,
                                      crypt_in_lowercase=config.CRYPT_IN_LOWERCASE)
        # Close tags (https://st.yandex-team.ru/ANTIADB-1023)
        body_part.text = close_tags(body_part.text, config.self_closing_tags_re)


def partner_replace_step(body_part, ctx, config):
    if UrlClass.PARTNER in ctx.request.tags and ctx.content_type == 'text/html':
        if config.inject_inline_js is not None:
            insert_inline_script(
                config=config,
                body_part=body_part,
                charset=ctx.charset,
                key=ctx.key,
                nonce_js=ctx.nonce_js
            )
        if config.xhr_protection_on or config.script_protection_on:
            insert_visibility_protection(
                config=config,
                body=body_part,
                charset=ctx.charset,
                key=ctx.key,
                binurlprefix=ctx.binurlprefix,
                nonce_js=ctx.nonce_js,
                nonce_css=ctx.nonce_css,
            )
    enable_log = END2END_TESTING or ctx.http_handler.is_argus_handler
    if ctx.replace_body_re is not None:
        body_part.text = body_replace(
            body=body_part.text,
            key=ctx.key,
            replace_dict=ctx.replace_body_re,
            charset=ctx.charset,
            crypt_in_lowercase=config.CRYPT_IN_LOWERCASE,
            url=ctx.url,
            logger=ctx.logger if enable_log else None,
        )
    # https://st.yandex-team.ru/ANTIADB-2719
    if config.replace_body_re_per_url:
        for url_re, replace_dict in config.replace_body_re_per_url.items():
            if url_re.match(ctx.url):
                body_part.text = body_replace(
                    body=body_part.text,
                    key=ctx.key,
                    replace_dict=replace_dict,
                    charset=ctx.charset,
                    crypt_in_lowercase=config.CRYPT_IN_LOWERCASE,
                    url=ctx.url,
                    logger=ctx.logger if enable_log else None,
                )
                break
    if config.replace_body_re_except_url:
        for url_re, replace_dict in config.replace_body_re_except_url.items():
            if url_re.match(ctx.url) is None:
                body_part.text = body_replace(
                    body=body_part.text,
                    key=ctx.key,
                    replace_dict=replace_dict,
                    charset=ctx.charset,
                    crypt_in_lowercase=config.CRYPT_IN_LOWERCASE,
                    url=ctx.url,
                    logger=ctx.logger if enable_log else None,
                )


def __is_replace_resource_with_xhr_enabled(_, ctx, config):
    return UrlClass.PARTNER in ctx.request.tags and ctx.content_type == 'text/html' and config.replace_resource_with_xhr_re


@step_condition(condition=__is_replace_resource_with_xhr_enabled)
@logged_step(action_name=action.REPLACE_RESOURCE_WITH_XHR)
def replace_resource_with_xhr_step(body_part, ctx, config):
    # replace script with matched src to xhr https://st.yandex-team.ru/ANTIADB-1255
    orig_text = body_part.text
    try:
        for sync, regexp in config.replace_resource_with_xhr_re:
            body_part.text = replace_resource_with_xhr(body_part.text, regexp, ctx.nonce_js, ctx.nonce_css,
                                                       config.REMOVE_SCRIPTS_AFTER_RUN, sync == 'sync')
    except Exception:
        ctx.logger.error("Replace resource with xhr failed", action=action.REPLACE_RESOURCE_WITH_XHR,
                         url=ctx.request.url.geturl(), exc_info=True)
        body_part.text = orig_text


def crypt_rambler_relative_urls_step(body_part, ctx, config):
    if body_part.part_type not in (BodyType.URL, BodyType.UNDEFINED):
        return
    body_part.text = body_crypt(
        body=body_part.text,
        binurlprefix=ctx.binurlprefix,
        key=ctx.key,
        crypt_url_re=config.rambler_crypt_relative_url_re,
        enable_trailing_slash=False,
        file_url=ctx.url,
        min_length=config.RAW_URL_MIN_LENGTH,
        crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
        config=config,
    )


def crypt_partner_relative_urls_step(body_part, ctx, config):
    if body_part.part_type not in (BodyType.URL, BodyType.UNDEFINED):
        return
    body_part.text = body_crypt(
        body=body_part.text,
        binurlprefix=ctx.binurlprefix,
        key=ctx.key,
        crypt_url_re=config.partner_crypt_relative_url_re,
        enable_trailing_slash=config.CRYPT_ENABLE_TRAILING_SLASH,
        file_url=ctx.url,
        min_length=config.RAW_URL_MIN_LENGTH,
        crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
        image_to_iframe_url_re_match=config.image_to_iframe_url_re,
        image_to_iframe_url_length=config.IMAGE_TO_IFRAME_URL_LENGTH,
        image_to_iframe_url_is_relative=config.IMAGE_TO_IFRAME_URL_IS_RELATIVE,
        image_to_iframe_changing_ratio=config.IMAGE_TO_IFRAME_CHANGING_RATIO,
        config=config,
        partner_cookieless_url_re_match=config.partner_to_cookieless_host_urls_re_match,
        bypass_url_re_match=config.bypass_url_re_match,
    )


def crypt_yandex_canvas_ads_relative_urls_step(body_part, ctx, config):
    if body_part.part_type not in (BodyType.URL, BodyType.UNDEFINED):
        return
    file_url = None
    # если внутри файла есть basePath, отнистельно которого нужно строить урлы, то выпаршиваем его
    if config.html5_banner_re.search(body_part.text):
        base_path = config.html5_base_path_re.search(body_part.text)
        if base_path is not None:
            file_url = base_path.group(1).replace('\\/', '/')
    # иначе для исправления относительных урлов в js-ках mds вытаскиваем basePath из запроса
    elif config.storage_mds_url_re_match.match(ctx.url):
        file_url = ctx.url
    if file_url is not None:
        body_part.text = body_crypt(
            body=body_part.text,
            binurlprefix=ctx.binurlprefix,
            key=ctx.key,
            crypt_url_re=config.crypt_html5_relative_url_re,
            enable_trailing_slash=False,
            file_url=file_url,
            min_length=config.RAW_URL_MIN_LENGTH,
            crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
            config=config,
        )


def body_crypt_main_step(body_part, ctx, config):
    if body_part.part_type not in (BodyType.URL, BodyType.UNDEFINED):
        return
    body_part.text = body_crypt(
        body=body_part.text,
        binurlprefix=ctx.binurlprefix,
        key=ctx.key,
        crypt_url_re=config.crypt_url_re,
        enable_trailing_slash=config.CRYPT_ENABLE_TRAILING_SLASH,
        min_length=config.RAW_URL_MIN_LENGTH,
        crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
        image_urls_crypting_ratio=config.IMAGE_URLS_CRYPTING_RATIO if UrlClass.PARTNER in ctx.request.tags else 1,
        partner_url_re_match=config.partner_url_re_match,
        # https://st.yandex-team.ru/ANTIADB-1035
        image_to_iframe_url_re_match=config.image_to_iframe_url_re,
        image_to_iframe_url_length=config.IMAGE_TO_IFRAME_URL_LENGTH,
        image_to_iframe_url_is_relative=config.IMAGE_TO_IFRAME_URL_IS_RELATIVE,
        image_to_iframe_changing_ratio=config.IMAGE_TO_IFRAME_CHANGING_RATIO,
        config=config,
        partner_cookieless_url_re_match=config.partner_to_cookieless_host_urls_re_match,
        bypass_url_re_match=config.bypass_url_re_match,
    )


def process_rtb_auction_response_step(body_part, ctx, config):
    if not ctx.last_response_successfull:
        return
    is_bk_or_adfox = UrlClass.BK in ctx.request.tags or UrlClass.ADFOX in ctx.request.tags
    is_ya_rtb_auction = is_bk_or_adfox and UrlClass.RTB_AUCTION in ctx.request.tags
    if is_ya_rtb_auction or UrlClass.RAMBLER_AUCTION in ctx.request.tags:
        process_rtb_auction_response(ctx=ctx,
                                     body_part=body_part,
                                     request=ctx.request,
                                     config=config,
                                     binurlprefix=ctx.binurlprefix,
                                     key=ctx.key)
    elif UrlClass.BK_META_BULK_AUCTION in ctx.request.tags:
        process_bulk_meta_auction_response(ctx=ctx,
                                           body_part=body_part,
                                           config=config,
                                           binurlprefix=ctx.binurlprefix,
                                           key=ctx.key)


def process_nanpu_auction_response_step(body_part, ctx, config):
    if not ctx.last_response_successfull:
        return
    if UrlClass.NANPU_AUCTION in ctx.request.tags:
        nanpu_response_repack(ctx=ctx,
                              config=config,
                              body_part=body_part,
                              binurlprefix=ctx.binurlprefix,
                              key=ctx.key)


def process_body_replace_step(body_part, ctx, config):
    if config.crypt_body_re is not None:
        body_part.text = body_replace(body=body_part.text,
                                      key=ctx.key,
                                      replace_dict={config.crypt_body_re: None},
                                      charset=ctx.charset,
                                      crypt_in_lowercase=config.CRYPT_IN_LOWERCASE)


def __is_adb_function_replace_step_enabled(_, ctx, config):
    is_crypt_partner_enabled = config.encryption_step_enabled(EncryptionSteps.partner_content_class_crypt)
    return is_crypt_partner_enabled and config.REPLACE_ADB_FUNCTIONS and UrlClass.PCODE not in ctx.request.tags


@step_condition(condition=__is_adb_function_replace_step_enabled)
@logged_step(action_name=action.ADB_FUNCTION_REPLACE)
def adb_function_replace_step(body_part, ctx, config):
    # Замена макроса __ADB_FUNCTIONS__ во всех запросах https://st.yandex-team.ru/ANTIADB-2212
    crypted_functions = crypted_functions_replace(config, ctx)
    js_replace_dict = {
        r'[\'\"]__ADB_FUNCTIONS__[\'\"]': bytes('{' + crypted_functions[1:] + '}'),
    }
    replace_dict = regexp_tools.compile_replace_dict(js_replace_dict)
    body_part.text = body_replace(body_part.text, ctx.key, replace_dict)


def __is_adb_cookies_replace_step_enabled(_, ctx, config):
    is_mobile = ctx.request.user_agent_data.device == sys_config.UserDevice.MOBILE or ctx.request.user_agent_data.isDeviceUndefined
    return is_mobile and config.encryption_step_enabled(EncryptionSteps.pcode_replace) and UrlClass.PARTNER in ctx.request.tags and UrlClass.PCODE not in ctx.request.tags


@step_condition(condition=__is_adb_cookies_replace_step_enabled)
@logged_step(action_name=action.ADB_COOKIES_REPLACE)
def adb_cookies_replace_step(body_part, ctx, config):
    # Placeholders __ADB_COOKIES__ must be no more than once in a file (count=1)
    body_part.text = body_replace(body_part.text, ctx.key, config.cookie_replace_dict, count=1)


def autoredirect_script_step(body_part, ctx, config):
    request = ctx.request
    is_autoredirect = UrlClass.AUTOREDIRECT_SCRIPT in request.tags and config.autoredirect_replace_re is not None
    is_desktop = request.user_agent_data.device == sys_config.UserDevice.DESKTOP
    if is_autoredirect and is_desktop:
        body_part.text = body_replace(
            body=body_part.text,
            key=ctx.key,
            replace_dict=config.autoredirect_replace_re,
            charset=ctx.charset,
            crypt_in_lowercase=config.CRYPT_IN_LOWERCASE
        )


# Replace some placeholders in PCODE js scripts to provide correct detect and crypting methods
def js_replace_step(body_part, ctx, config):
    if config.encryption_step_enabled(EncryptionSteps.pcode_replace) and (UrlClass.PCODE in ctx.request.tags or UrlClass.DZEN_PLAYER in ctx.request.tags):
        js_replace(ctx, body_part, config, ctx.seed, ctx.key)


@step_condition(condition=lambda _, __, config: config.encryption_step_enabled(EncryptionSteps.loaded_js_crypt))
@logged_step(action_name=action.LOADED_JS_CRYPT)
def crypt_js_links_step(body_part, ctx, config):
    # Crypt all javascripts links https://st.yandex-team.ru/ANTIADB-1032
    try:
        body_part.text = crypt_js_body(body_part.text, ctx.seed, ctx.charset, config.REMOVE_SCRIPTS_AFTER_RUN)
    except Exception:
        ctx.logger.error("Crypt loaded js failed", action=action.LOADED_JS_CRYPT,
                         url=ctx.request.url.geturl(), exc_info=True)


def __is_crypt_inline_js(ctx, config):
    is_crypt_inline_js = config.encryption_step_enabled(sys_config.EncryptionSteps.inline_js_crypt)
    is_supported_browser = ctx.request.user_agent_data.BrowserName not in sys_config.BYPASS_CRYPT_INLINE_JS_BROWSERS
    return is_crypt_inline_js and is_supported_browser


@step_condition(condition=lambda _, ctx, config: __is_crypt_inline_js(ctx, config))
@logged_step(action_name=action.INLINE_JS_CRYPT)
def crypt_inline_js_step(body_part, ctx, config):
    # Crypt all inline javascripts https://st.yandex-team.ru/ANTIADB-1032
    # Bypass IE because of it doesn't support grave accent quotes (``) in JS
    # https://st.yandex-team.ru/ANTIADB-1707
    try:
        body_part.text = crypt_inline_js(body_part.text, ctx.seed, ctx.charset,
                                         ctx.content_type == 'application/json',
                                         config.REMOVE_SCRIPTS_AFTER_RUN)
    except Exception:
        ctx.logger.error("Crypt inline js failed", action=action.INLINE_JS_CRYPT,
                         url=ctx.request.url.geturl(), exc_info=True)


@step_condition(condition=lambda _, __, config: config.ADD_NONCE)
@logged_step(action_name=action.ADD_NONCE)
def add_nonce_step(body_part, ctx, config):
    # add nonce in all inline scripts and CSP headers (tags)
    _nonce = generate_seed(change_period=config.SEED_CHANGE_PERIOD,
                           time_shift_minutes=config.SEED_CHANGE_TIME_SHIFT_MINUTES,
                           salt=config.version)
    try:
        # add nonce in inline script
        body = add_nonce_to_scripts(body_part.text, _nonce)
        # add nonce to CSP headers and tags in body
        body = add_nonce_to_csp(ctx.headers, body, _nonce)
        body_part.text = body
    except Exception:
        ctx.logger.error("add nonce failed", action=action.ADD_NONCE, url=ctx.request.url.geturl(), exc_info=True)


def add_second_domain_to_csp_step(body_part, ctx, config):
    if config.ENCRYPT_TO_THE_TWO_DOMAINS and ctx.content_type in ['text/html'] and not config.second_domain_is_yastatic:
        # fix CSP headers and CSP tags for two domain crypt
        body_part.text = add_second_domain_to_csp(ctx.headers, body_part.text, config.second_domain)


def __is_csp_patch(ctx, config):
    return config.CSP_PATCH and ctx.content_type in ['text/html'] and UrlClass.PARTNER in ctx.request.tags


@step_condition(condition=lambda _, ctx, config: __is_csp_patch(ctx, config))
@logged_step(action_name=action.CSP_PATCH)
def add_patch_to_csp_step(body_part, ctx, config):
    # patch csp
    try:
        body_part.text = add_patch_to_csp(ctx.headers, body_part.text, config.CSP_PATCH)
    except Exception:
        ctx.logger.error("patch csp failed", action=action.CSP_PATCH, url=ctx.request.url.geturl(), exc_info=True)


@logged_step(action_name=action.NANPU_RESPONSE_PROCESS)
def nanpu_response_repack(ctx, config, body_part, binurlprefix, key):
    """
    Обрабатываем ответ от NANPU
    https://st.yandex-team.ru/ANTIADB-2631
    """
    try:
        response = b64decode(body_part.text.encode('utf-8'))
        if config.replace_body_with_tags_re is not None:
            response = body_replace(body=response,
                                    key=key,
                                    replace_dict=config.replace_body_with_tags_re,
                                    crypt_in_lowercase=config.CRYPT_IN_LOWERCASE)
        response = body_crypt(body=response,
                              binurlprefix=binurlprefix,
                              key=key,
                              crypt_url_re=config.crypt_url_re_with_nanpu,
                              enable_trailing_slash=config.CRYPT_ENABLE_TRAILING_SLASH,
                              min_length=config.RAW_URL_MIN_LENGTH,
                              crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
                              config=config,
                              )
        if config.crypt_body_re is not None:
            response = body_replace(body=response,
                                    key=key,
                                    replace_dict={config.crypt_body_re: None},
                                    crypt_in_lowercase=config.CRYPT_IN_LOWERCASE,
                                    )
        body_part.text = b64encode(response)
    except Exception:
        ctx.logger.error("NANPU response is not valid", action=action.NANPU_RESPONSE_PROCESS, exc_info=True,
                         meta=body_part.text[:30000])
        ctx.metrics.increase_counter(action.NANPU_RESPONSE_PROCESS, success=0)


@logged_step(action_name=action.BULK_META_AUCTION_RESPONSE_PROCESS)
def process_bulk_meta_auction_response(ctx, body_part, config, binurlprefix, key):
    try:
        meta_bulk_result = json.loads(body_part.text)
        creatives = []
        for creative in meta_bulk_result["responseData"]["ads"]:
            if "direct" in creative and "ssr" in creative["direct"]:
                creative = meta_html_b64_repack(meta_body=creative,
                                                config=config,
                                                binurlprefix=binurlprefix,
                                                key=key)
            creatives.append(creative)
        meta_bulk_result["responseData"]["ads"] = creatives
        body_part.text = json.dumps(meta_bulk_result)
        ctx.metrics.increase_counter(action.BULK_META_AUCTION_RESPONSE_PROCESS, success=1)
    except Exception:
        ctx.logger.error("BULK_META response is not valid", action=action.BULK_META_AUCTION_RESPONSE_PROCESS, exc_info=True,
                         meta=body_part.text[:30000])
        ctx.metrics.increase_counter(action.BULK_META_AUCTION_RESPONSE_PROCESS, success=0)


@logged_step(action_name=action.RTB_AUCTION_RESPONSE_PROCESS)
def process_rtb_auction_response(ctx, body_part, request, config, binurlprefix, key):
    request_qargs = parse_qsl(request.url.query)

    # Шифруем урлы счетчиков при наличии флага join-show-links (в обычном случае мы не шифруем счетчики сами)
    # https://st.yandex-team.ru/ANTIADB-1007
    if UrlClass.BK in request.tags and UrlClass.ADFOX not in request.tags and ('join-show-links', '1') in request_qargs:
        body_part.text = body_crypt(body=body_part.text,
                                    binurlprefix=binurlprefix,
                                    key=key,
                                    crypt_url_re=config.bk_count_and_abuse_url_re,
                                    enable_trailing_slash=config.CRYPT_ENABLE_TRAILING_SLASH,
                                    min_length=config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
                                    config=config,
                                    )
    # BASE64 REPACK STEP
    repacked_to_json = None
    try:
        if UrlClass.ADFOX in request.tags:
            repacked_to_json = adfox_base64repack(ctx, config, body_part.text, binurlprefix, key)
        elif UrlClass.BK in request.tags:
            repacked_to_json = bk_base64repack(ctx, config, body_part.text, binurlprefix, key)
        elif UrlClass.RAMBLER_AUCTION in request.tags:
            repacked_to_json = rambler_base64repack(ctx, config, body_part.text, binurlprefix, key)
    except AdFoxErrorsNotEmptyException:
        # commenting json when request via script is enabled (to avoid client errors) https://st.yandex-team.ru/ANTIADBSUP-1432
        if ctx.rtb_auction_via_script:
            body_part.text = '//' + body_part.text
            ctx.set_content_type('application/javascript')
        return
    except Exception:
        ctx.logger.error("Process rtb auction response is failed", action=action.RTB_AUCTION_RESPONSE_PROCESS,
                         exc_info=True)

    if repacked_to_json is not None:
        if UrlClass.RAMBLER_AUCTION in request.tags:
            body_part.text = "{callback}({body})".format(callback=ctx.meta_callback,
                                                         body=repacked_to_json.replace("'", "\\'"))
        elif 'server-side=1' not in request.url.query and (ctx.rtb_auction_via_script or ctx.rtb_jsonp_response):
            # Поддержать запрос за рекламой из тега script на бекенде cryprox
            # BK:    https://st.yandex-team.ru/ANTIADB-1137
            # ADFOX: https://st.yandex-team.ru/ANTIADB-1280
            # в случае запроса рекламы из скрипта, результат нужно завернуть в оригинальный callback (если мы его заменили)
            body = repacked_to_json.replace("\\", "\\\\")
            if getattr(ctx, 'nativeDesign', False):
                body = body.replace('\\"', '\\\\"')
            body_part.text = "{callback}('{body}')".format(callback=ctx.meta_callback,
                                                           body=body.replace("'", "\\'"))
            ctx.set_content_type('application/javascript')
        elif 'server-side=1' in request.url.query and ctx.meta_callback != 'json':
            # https://st.yandex-team.ru/ANTIADB-1888 yandex_mail server side requests
            body_part.text = "{callback}('{body}')".format(callback=ctx.meta_callback or '',
                                                           body=repacked_to_json.replace("'", "\\'"))
            ctx.set_content_type('application/javascript')
        else:
            # Мы привели контент к чистому json, так что надо сменить тип контента на соответствующий
            body_part.text = repacked_to_json
            ctx.set_content_type('application/json')
    # Подготовка к шифрованию содержимого ответа от RTB-хоста с помощью crypt_js_body (https://st.yandex-team.ru/ANTIADB-1032)
    # При шифровании скриптов содержимое эвалится (eval). Заэвалить json невозможно, потому необходимо обернуть его в ()
    if config.encryption_step_enabled(EncryptionSteps.loaded_js_crypt) and ctx.content_type == 'application/json':
        body_part.text = '({})'.format(body_part.text)
        ctx.set_content_type('application/javascript')


@logged_step(action_name=action.BASE64_REPACK)
def rambler_base64repack(ctx, config, body_text, binurlprefix, key):
    """
    Обрабатываем ответ от SSP Rambler
    https://st.yandex-team.ru/ANTIADB-2195
    """
    try:
        rambler_response = jsonify_meta(body_text, config.rambler_js_obj_extractor_re, ctx.metrics,
                                        action.RAMBLER_JSON_PARSE)
        banners = rambler_response.get('banners')
        if banners:
            for creative in banners['graph']:
                # в ответе пререндеренная на сервере реклама
                if 'directs2s' in creative['words']:
                    # "data:text/html;base64,PGh0bWw+PGJvZHk+..."
                    begin, data = creative['source'].split(',', 1)
                    # может быть не кратно 4
                    data += '=' * (4 - len(data) % 4)
                    creative_code = b64decode(data.encode('utf-8'))
                    creative_code = meta_html_b64_repack(meta_body=creative_code,
                                                         config=config,
                                                         binurlprefix=binurlprefix,
                                                         key=key)
                    creative['source'] = "{},{}".format(begin, b64encode(creative_code))
        ctx.metrics.increase_counter(action.RAMBLER_JSON_PARSE, success=1)
        return json.dumps(rambler_response)
    except Exception:
        ctx.logger.error("RAMBLER response is not valid", action=action.RAMBLER_JSON_PARSE, exc_info=True,
                         meta=body_text[:30000])
        ctx.metrics.increase_counter(action.RAMBLER_JSON_PARSE, success=0)


@logged_step(action_name=action.BASE64_REPACK)
def adfox_base64repack(ctx, config, body_text, binurlprefix, key):
    """
    Обрабатываем ответ от РТБ хоста AdFox, ищем в нем base64encoded коды креативов и шифруем их содержимое
    https://st.yandex-team.ru/ANTIADB-1139
    """
    try:
        adfox_response = json.loads(body_text)
        if len(adfox_response.get('errors', [])) > 0 and 'getBulk' in ctx.request.url.path:
            # Do not process Adfox responses with non-empty errors field
            raise AdFoxErrorsNotEmptyException
        creatives = []
        for creative in adfox_response.get('data', []):
            creative_rtb_code = None
            if creative['type'] == 'banner.direct' and creative['attributes'].get('dataBase64') is not None:
                data_key = 'dataBase64'
                creative_rtb_code = urlsafe_b64decode(creative['attributes'][data_key].encode('utf-8'))
                try:
                    parsed_creative_rtb_code = json.loads(creative_rtb_code)
                    if parsed_creative_rtb_code.get('rtb', {}).get('html') is not None:
                        creative_rtb_code = parsed_creative_rtb_code
                except ValueError:
                    ctx.logger.debug('AdFox response is not a valid json.')
            # https://st.yandex-team.ru/ANTIADB-2587
            elif creative['type'] == 'banner.direct' and \
                    ('html' in creative['attributes'].get('data', {}).get('rtb', {}) or
                     'ssr' in creative['attributes'].get('data', {}).get('rtb', {}) or
                     'ssr' in creative['attributes'].get('data', {}).get('direct', {}) or
                     'video' in creative['attributes'].get('data', {}).get('rtb', {})):
                data_key = 'data'
                creative_rtb_code = creative['attributes'][data_key]
            elif creative['type'] == 'banner.html':
                if creative['attributes'].get('htmlBase64') is not None:
                    # https://st.yandex-team.ru/ANTIADB-1793#5d414abd34894f001eb4b28d
                    data_key = 'htmlBase64'
                    creative_rtb_code = urlsafe_b64decode(creative['attributes'][data_key].encode('utf-8'))
                elif creative['attributes'].get('htmlEncoded') is not None:
                    # https://st.yandex-team.ru/ANTIADB-2620
                    data_key = 'htmlEncoded'
                    creative_rtb_code = unquote(creative['attributes'][data_key].encode('utf-8'))
            if creative_rtb_code is not None:
                if 'video' not in creative['attributes'].get('data', {}).get('rtb', {}):
                    creative_rtb_code_repacked = meta_html_b64_repack(meta_body=creative_rtb_code,
                                                                      config=config,
                                                                      binurlprefix=binurlprefix,
                                                                      key=key)
                else:
                    creative_rtb_code_repacked = meta_video_b64_repack(meta_body=creative_rtb_code,
                                                                       config=config,
                                                                       binurlprefix=binurlprefix,
                                                                       key=key)
                # noinspection PyUnboundLocalVariable
                if data_key == 'data':
                    creative['attributes'][data_key] = creative_rtb_code_repacked
                else:
                    if isinstance(creative_rtb_code_repacked, dict):
                        creative_rtb_code_repacked = json.dumps(creative_rtb_code_repacked)
                    if data_key == 'htmlEncoded':
                        creative['attributes'][data_key] = quote(creative_rtb_code_repacked, safe="-_.!~*'()")
                    else:
                        creative['attributes'][data_key] = urlsafe_b64encode(creative_rtb_code_repacked)
            creatives.append(creative)
        adfox_response['data'] = creatives
        return json.dumps(adfox_response)
    except AdFoxErrorsNotEmptyException:
        raise
    except Exception:
        ctx.logger.error('AdFox response is not valid', action=action.ADFOX_JSON_PARSE, exc_info=True,
                         meta=body_text[:30000])
        ctx.metrics.increase_counter(action.ADFOX_JSON_PARSE, success=0)


@logged_step(action_name=action.BASE64_REPACK)
def bk_base64repack(ctx, config, body_text, binurlprefix, key):
    """
    Обрабатываем ответ от РТБ хоста БК, ищем в нем base64encoded коды креативов и шифруем их содержимое
    https://st.yandex-team.ru/ANTIADB-899
    """
    def replace_widget_data(data):
        if data.get('name', '') in ('nativeDesign', 'grid'):
            for field in ('css', 'template'):
                if field in data:
                    setattr(ctx, 'nativeDesign', True)
                    break

    # JSONIFY https://st.yandex-team.ru/ANTIADB-982
    # TODO: убрать jsonify после https://st.yandex-team.ru/BSSERVER-6254, https://st.yandex-team.ru/PCODE-9775, https://st.yandex-team.ru/ANTIADB-1145
    try:
        if ('callback', 'json') in parse_qsl(ctx.request.url.query) and not config.pjson_re.match(body_text):
            meta_result = json.loads(body_text)
            ctx.metrics.increase_counter(action.BK_JSON_PARSE, validate='Valid JSON')
        else:
            meta_result = jsonify_meta(body_text, config.meta_js_obj_extractor_re, ctx.metrics,
                                       action.BK_JSON_PARSE)
        ctx.metrics.increase_counter(action.BK_JSON_PARSE, success=1)
        # META_HTML_B64_REPACK https://st.yandex-team.ru/ANTIADB-899
        # ssr https://st.yandex-team.ru/ANTIADB-2366, https://st.yandex-team.ru/ANTIADB-2566, https://st.yandex-team.ru/ANTIADB-2759
        is_widget_ssr = 'seatbid' in meta_result and \
                        ('ssr' in meta_result.get('settings', {}) or any([check_key_in_dict(meta_result['settings'][block_id], 'ssr') for block_id in meta_result.get('settings', {}).keys()]))
        is_vast = 'vast' in meta_result.get('rtb', {}) or 'vastBase64' in meta_result.get('rtb', {}) or 'video' in meta_result.get('rtb', {})
        is_media = ('html' in meta_result.get('rtb', {}) or 'ssr' in meta_result.get('rtb', {})) and not is_vast
        is_tga_ssr = 'ssr' in meta_result.get('direct', {})
        if is_media or is_tga_ssr or is_widget_ssr:
            meta_result = meta_html_b64_repack(meta_body=meta_result, config=config, binurlprefix=binurlprefix, key=key)
        # https://st.yandex-team.ru/ANTIADB-2581
        elif 'data' in meta_result.get('rtb', {}):
            meta_result = smartbanner_json_data_crypt(meta_body=meta_result, config=config, binurlprefix=binurlprefix, key=key)
        # https://st.yandex-team.ru/ANTIADB-2995
        elif is_vast:
            meta_result = meta_video_b64_repack(meta_body=meta_result, config=config, binurlprefix=binurlprefix, key=key)
        # https://st.yandex-team.ru/ANTIADB-2404
        if ctx.rtb_auction_via_script and 'seatbid' in meta_result:
            if 'name' in meta_result.get('settings', {}):
                replace_widget_data(meta_result['settings'])
            else:
                for block_id in meta_result.get('settings', {}):
                    replace_widget_data(meta_result['settings'][block_id])
        return json.dumps(meta_result)
    except Exception:
        ctx.logger.error("BK response is not valid", action=action.BK_JSON_PARSE, exc_info=True,
                         meta=body_text[:30000])
        ctx.metrics.increase_counter(action.BK_JSON_PARSE, success=0)


@logged_step(action_name=action.JS_REPLACE)
def js_replace(ctx, response_body, config, seed, key):
    # Update pcode config object with current encode params
    pcode_config_object = deepcopy(config.pcode_params)
    pcode_config_object['encode'] = {'key': urlsafe_b64encode(key)}
    if pcode_config_object.get('forcecry') is not None and pcode_config_object['forcecry'].get('enabled', False):
        # to be sure experiment will stop even if configs not updating:
        is_experiment_enabled = is_experiment_date(config.experiment_range)
        is_experiment_device = ctx.request.user_agent_data.device in config.experiment_device
        pcode_config_object['forcecry']['enabled'] = is_experiment_enabled and is_experiment_device
    if config.HIDE_META_ARGS_ENABLED:
        pcode_config_object['hideUriPathHeader'] = config.current_hide_uri_path_header_name
        pcode_config_object['hideGrabHeader'] = config.current_grab_header_name
        all_headers_size = sum([len(name) + len(value) for name, value in ctx.request.headers.items()])
        pcode_config_object['hideMetaArgsHeaderMaxSize'] = max(config.HIDE_META_ARGS_HEADER_MAX_SIZE - all_headers_size,
                                                               0)  # it's adoptive heuristic

    if ctx.is_argus_handler:
        pcode_config_object['pcodeDebug'] = True

    if config.DISABLE_DETECT:
        pcode_config_object['dbltsr'] = strftime("%Y-%m-%dT%H:%M:%S+0000", gmtime())

    script_key_0, script_key_1 = get_script_key(key, seed)
    # в скрипте детекта нужно оставить по одному плейсхолдеру для частей script_key
    response_body.text = remove_all_placeholders_except_one(response_body.text, sys_config.SCRIPT_KEY_PLACEHOLDERS_RE)

    crypted_functions = crypted_functions_replace(config, ctx)
    js_replace_dict = {
        r'[\'\"]__ADB_CONFIG__[\'\"]': bytes(json.dumps(pcode_config_object)[:-1] + crypted_functions + '}'),
        r'[\'\"]__ADB_FUNCTIONS__[\'\"]': bytes('{' + crypted_functions[1:] + '}'),
        r'__scriptKey0Value__': bytes(script_key_0),
        r'__scriptKey1Value__': bytes(script_key_1),
    }
    replace_dict = regexp_tools.compile_replace_dict(js_replace_dict)
    # Placeholders __ADB_CONFIG__, __ADB_FUNCTIONS__ must be no more than once in a file (count=1)
    response_body.text = body_replace(response_body.text, key, replace_dict, count=1)


def insert_inline_script(config, body_part, charset, key, nonce_js):
    inject_inline_script = v_protect.INLINE_JS_TEMPLATE.format(
        INLINE_JS=config.inject_inline_js,
        NONCE="nonce=\"{}\" ".format(nonce_js) if nonce_js else ""
    )
    if config.INJECT_INLINE_JS_POSITION == InjectInlineJSPosition.HEAD_BEGINNING:
        match_re = v_protect.BEGINNING_OF_HEAD_RE
    else:
        match_re = v_protect.END_OF_HEAD_RE
    inject_inline_replace_body_re = regexp_tools.compile_replace_dict({match_re: inject_inline_script})
    body_part.text = body_replace(body_part.text, key, inject_inline_replace_body_re, count=1, charset=charset,
                                  crypt_in_lowercase=config.CRYPT_IN_LOWERCASE)


def insert_visibility_protection(config, body, charset, key, binurlprefix, nonce_js, nonce_css):
    # https://st.yandex-team.ru/ANTIADB-1692, https://st.yandex-team.ru/ANTIADB-1716
    if config.script_protection_on:
        # вставка инлайнового скрипта c загружаемым скриптом
        # если распарсили nonce для style в csp, то приклеим его значение как квериарг ссылки
        # чтобы можно было его вставить в инлайновый стиль при выполнении скрипта
        template = v_protect.LOADED_SCRIPT_TEMPLATE
        fixing_url = v_protect.SCRIPT_FIX_STYLE_SRC.format(
            ARGS="?{}={}".format(v_protect.NONCE_QUERY_ARG_NAME, nonce_css) if nonce_css else ""
        )
    else:
        # вставка инлайнового скрипта сразу делающего xhr запрос за стилями
        template = v_protect.XHR_TEMPLATE
        fixing_url = v_protect.XHR_FIX_STYLE_SRC
    fixing_script = template.format(
        SRC=fixing_url,
        NONCE="nonce=\"{}\" ".format(nonce_js) if nonce_js else "",
        NONCE_2="a.setAttribute(\"nonce\", \"{}\");".format(nonce_css) if nonce_css else "",
        SCRIPT_BREAKER_PROTECTION=v_protect.SCRIPT_BREAKER_PROTECTION if config.break_object_current_script else ''
    )
    fixing_script = body_replace(fixing_script, key, {v_protect.crypt_property_to_check_re: None})
    replace_dict = dict()
    # добавляем класс со скрыващим style в элементы верстки, используемые для защиты рекламы
    replace_dict[v_protect.CLASS_WRAPPER_RE.format(class_re=config.v_protect_class_re)] = ' ' + v_protect.PLACEHOLDER
    # вставляем инлайновый стиль для скрытия элемента, не забываем про nonce для style
    replace_dict[v_protect.END_OF_HEAD_RE] = v_protect.HARMFUL_STYLE.format(
        NONCE_2="nonce=\"{}\" ".format(nonce_css) if nonce_css else ""
    )
    # скрипт, при выполении которого отменяется скрытие нерекламных элементов верстки
    replace_dict[v_protect.END_OF_BODY_RE] = fixing_script
    replace_body_re = regexp_tools.compile_replace_dict(replace_dict)
    # todo do we need to replace url?
    body.text = body_replace(body.text, key, replace_dict=replace_body_re, charset=charset,
                             crypt_in_lowercase=config.CRYPT_IN_LOWERCASE)
    if body.part_type == BodyType.URL or body.part_type == BodyType.UNDEFINED:
        body.text = body_crypt(
            body.text, binurlprefix, key,
            crypt_url_re=v_protect.crypt_url_re,
            enable_trailing_slash=config.CRYPT_ENABLE_TRAILING_SLASH,
            min_length=config.RAW_URL_MIN_LENGTH,
            crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
            config=config,
            partner_cookieless_url_re_match=config.partner_to_cookieless_host_urls_re_match,
        )
    body.text = body_replace(body.text, key, replace_dict={v_protect.crypt_body_re: None}, charset=charset,
                             crypt_in_lowercase=config.CRYPT_IN_LOWERCASE)
