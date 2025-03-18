# coding=utf-8
import os
import logging
from json import dumps
from urlparse import urlparse
from datetime import datetime, timedelta

import re2

from . import static_config
from . import bk as bk_config
from . import adfox as adfox_config
from . import system as system_config
from . import service as service_config
from .system import EncryptionSteps, DEFAULT_URL_TEMPLATE, InjectInlineJSPosition
from .ad_systems import AdSystem, AD_SYSTEM_CONFIG, YANDEX_AD_SYSTEMS, AD_SYSTEM_TAG_MAP

import antiadblock.cryprox.cryprox.common.visibility_protection as v_protect
from antiadblock.cryprox.cryprox.common.cry import generate_seed, encrypt_xor
from antiadblock.cryprox.cryprox.common.resource_utils import url_to_filename
from antiadblock.libs.decrypt_url.lib import get_key, SEED_LENGTH, URL_LENGTH_PREFIX_LENGTH
from antiadblock.cryprox.cryprox.common.cryptobody import generate_hide_meta_args_header_name
from antiadblock.cryprox.cryprox.common.tools.experiments import is_experiment_date, get_active_experiment
from antiadblock.cryprox.cryprox.common.tools.misc import int_list_to_bitmask, is_bit_enabled, not_none
from antiadblock.cryprox.cryprox.common.tools.regexp import re_completeurl, re_expand, compile_replace_dict, re_expand_relative_urls, re_merge
from antiadblock.cryprox.cryprox.config.system import Experiments, UserDevice, AUTOREDIRECT_REPLACE_RE, AUTOREDIRECT_DETECT_LIB_PATH


def calculate_decrypted_url_length(crypted_url_length, mixing_template, name):
    """
    :param crypted_url_length: желаемая длина зашифрованного урла
    :param mixing_template: шаблон (набор символов) для размешивания шифрованного урла
    :return: длина незашифрованного урла который нужно передать на шифрование чтоб получить зашифрованный желаемой длины (crypted_url_length)
    """
    # Добавляем __AAB_ORIGINorigin__ в ссылки, это нужно учесть при расчете длины
    AAB_ORIGIN_LENGTH = len('__AAB_ORIGIN__') + len(name)
    # расчет поправочного коэфициента (зависит от шаблона размешивания)
    addend = min(sum([len(i[0]) * i[1] for i in mixing_template]),
                 (crypted_url_length - (URL_LENGTH_PREFIX_LENGTH + AAB_ORIGIN_LENGTH + 1 + SEED_LENGTH)) / float(system_config.CRYPTED_URL_MIXING_STEP + 1))
    # 1-slash, 3/4 - коэфф расширения ссылки base64
    result = (crypted_url_length - (URL_LENGTH_PREFIX_LENGTH + 1 + SEED_LENGTH + addend)) * 3.0 / 4
    return max(0, int(round(result)))


class Config(object):

    def __init__(self, name, config, version=None):
        self.name = name
        self.partner_config = config
        self.version = version
        self.device = getattr(self.partner_config, 'DEVICE_TYPE', None)

        self.AD_SYSTEMS = getattr(self.partner_config, 'AD_SYSTEMS', YANDEX_AD_SYSTEMS)

        enabled_ad_systems = not_none([AD_SYSTEM_CONFIG.get(s) for s in self.AD_SYSTEMS])
        self.enabled_yandex_ad_systems = not_none([AD_SYSTEM_CONFIG.get(s) for s in self.AD_SYSTEMS if s in YANDEX_AD_SYSTEMS])

        # tags for denied ad systems
        self.forbidden_tags = set(AD_SYSTEM_TAG_MAP.values()) - set(not_none([AD_SYSTEM_TAG_MAP.get(ad_s) for ad_s in self.AD_SYSTEMS]))

        # service
        self.PROXY_CLIENT_CONFIG = dict(hostname_mapping=getattr(self.partner_config, 'HOSTNAME_MAPPING', dict()))
        self.PROXY_CLIENT_REQUEST_CONFIG = service_config.PROXY_CLIENT_REQUEST_CONFIG

        # system
        ACCEL_REDIRECT_URL_RE = system_config.ACCEL_REDIRECT_URL_RE + sum([getattr(s, "ACCEL_REDIRECT_URL_RE", []) for s in self.enabled_yandex_ad_systems], [])
        self.ACCEL_REDIRECT_PREFIX = system_config.ACCEL_REDIRECT_PREFIX

        # ADFOX
        self.ADFOX_DEBUG = getattr(self.partner_config, 'ADFOX_DEBUG', adfox_config.ADFOX_DEBUG)

        # YANDEX
        self.YANDEX_NETS = system_config.YANDEX_NETS

        # TOTAL FOR PARTNER
        self.ENCRYPT_TO_THE_TWO_DOMAINS = getattr(self.partner_config, 'ENCRYPT_TO_THE_TWO_DOMAINS', False)
        self.PARTNER_COOKIELESS_DOMAIN = getattr(self.partner_config, 'PARTNER_COOKIELESS_DOMAIN', 'naydex.net')
        self.second_domain_is_yastatic = True if self.PARTNER_COOKIELESS_DOMAIN in service_config.YASTATIC_DOMAINS else False
        self.PARTNER_TO_COOKIELESS_HOST_URLS_RE = getattr(self.partner_config, 'PARTNER_TO_COOKIELESS_HOST_URLS_RE', [])  # партнерская статика под второй бескуковый домен
        self.partner_to_cookieless_host_urls_re = re2.compile(re_completeurl(self.PARTNER_TO_COOKIELESS_HOST_URLS_RE), re2.IGNORECASE) if self.PARTNER_TO_COOKIELESS_HOST_URLS_RE else None
        self.partner_to_cookieless_host_urls_re_match = \
            re2.compile(re_completeurl(self.PARTNER_TO_COOKIELESS_HOST_URLS_RE, match_only=True), re2.IGNORECASE) \
            if self.PARTNER_TO_COOKIELESS_HOST_URLS_RE else None
        # так как в нас токены не полетят - разделяем по доменам второго уровня
        if not self.second_domain_is_yastatic:
            self.second_domain = '{}.{}'.format(self.name.replace('.', '-').replace('_', '-'), self.PARTNER_COOKIELESS_DOMAIN)
            self.cookieless_path_prefix = service_config.COOKIELESS_PATH_PREFIX
        else:
            self.second_domain = self.PARTNER_COOKIELESS_DOMAIN
            self.cookieless_path_prefix = '/naydex/' + self.name.replace('.', '-').replace('_', '-') + service_config.COOKIELESS_PATH_PREFIX
        # https://st.yandex-team.ru/ANTIADB-2732
        AVATARS_NOT_TO_COOKIELESS = getattr(self.partner_config, 'AVATARS_NOT_TO_COOKIELESS', False)
        self.YANDEX_STATIC_URL_RE = getattr(system_config, "STATIC_URL_RE", []) + sum([getattr(ad_system, 'STATIC_URL_RE', []) for ad_system in enabled_ad_systems], [])
        if not AVATARS_NOT_TO_COOKIELESS:
            self.YANDEX_STATIC_URL_RE.extend(getattr(bk_config, "AVATARS_MDS_URL_RE", []))

        CRYPT_MIME_RE = re_merge(system_config.CRYPT_MIME_RE + sum([getattr(ad_s, 'CRYPT_MIME_RE', []) for ad_s in enabled_ad_systems], []))
        self.ENCRYPTION_STEPS = int_list_to_bitmask(getattr(self.partner_config, 'ENCRYPTION_STEPS', system_config.DEFAULT_ENCRYPTION_STEPS))
        self.DISABLED_BK_AD_TYPES = int_list_to_bitmask(getattr(self.partner_config, 'DISABLED_AD_TYPES', bk_config.DEFAULT_DISABLED_AD_TYPES))
        self.DISABLE_TGA_WITH_CREATIVES = getattr(self.partner_config, 'DISABLE_TGA_WITH_CREATIVES', False)
        self.CM_TYPES = getattr(self.partner_config, 'CM_TYPE', system_config.CM_DEFAULT_TYPE)
        self.CM_DISABLED = self.CM_TYPES == [system_config.CookieMatching.disabled] or not self.CM_TYPES
        self.AUTO_SELECT_DETECT = getattr(self.partner_config, 'AUTO_SELECT_DETECT', False)
        self.CM_REDIRECT_URL = getattr(self.partner_config, 'CM_REDIRECT_URL', system_config.CM_DEFAULT_REDIRECT_URL)
        self.CM_IMAGE_URL = getattr(self.partner_config, 'CM_IMAGE_URL', system_config.CM_DEFAULT_IMAGE_URL)  # Available domains: https://st.yandex-team.ru/ANTIADB-880
        self.PARTNER_DETECT_HTML = getattr(self.partner_config, 'DETECT_HTML', [])
        self.PARTNER_DETECT_IFRAME = getattr(self.partner_config, 'DETECT_IFRAME', [])
        self.DETECT_COOKIE_TYPE = getattr(self.partner_config, 'DETECT_COOKIE_TYPE', system_config.COOKIE_DOMAIN_DEFAULT_TYPE)  # https://st.yandex-team.ru/ANTIADB-495
        self.DETECT_COOKIE_DOMAINS = getattr(self.partner_config, 'DETECT_COOKIE_DOMAINS', list()) \
            if self.DETECT_COOKIE_TYPE == system_config.DetectCookieType.list else list()
        self.PARTNER_DETECT_LINKS = getattr(self.partner_config, 'DETECT_LINKS', [])
        self.DETECT_TRUSTED_LINKS = getattr(self.partner_config, 'DETECT_TRUSTED_LINKS', [])
        self.CRYPTED_HOST = getattr(self.partner_config, 'CRYPTED_HOST', None)
        self.PARTNER_ACCEL_REDIRECT_PREFIX = system_config.PARTNER_ACCEL_REDIRECT_PREFIX
        # accel redirect for partner nginx
        PARTNER_ACCEL_REDIRECT_URL_RE = getattr(self.partner_config, 'ACCEL_REDIRECT_URL_RE', [])
        # accel redirect for proxy nginx
        ACCEL_REDIRECT_URL_RE += getattr(self.partner_config, 'PROXY_ACCEL_REDIRECT_URL_RE', [])
        self.INTERNAL = getattr(self.partner_config, 'INTERNAL', False)
        self.CRYPT_ENABLE_TRAILING_SLASH = getattr(self.partner_config, 'CRYPT_ENABLE_TRAILING_SLASH', system_config.CRYPT_ENABLE_TRAILING_SLASH)  # https://st.yandex-team.ru/ANTIADB-246
        self.RTB_AUCTION_VIA_SCRIPT = getattr(self.partner_config, 'RTB_AUCTION_VIA_SCRIPT', False)
        self.EXTUID_TAG = getattr(self.partner_config, 'EXTUID_TAG', '')
        self.EXTUID_COOKIE_NAMES = system_config.EXTUID_COOKIE_NAMES + getattr(self.partner_config, 'EXTUID_COOKIE_NAMES', [])
        self.CURRENT_COOKIE = getattr(self.partner_config, 'CURRENT_COOKIE', 'bltsr')  # current "cookie of the day"
        self.DEPRECATED_COOKIES = getattr(self.partner_config, 'DEPRECATED_COOKIES', [])  # old "cookies of the day"
        self.CRYPTED_YAUID_COOKIE_NAME = getattr(self.partner_config, 'CRYPTED_YAUID_COOKIE_NAME', system_config.CRYPTED_YAUID_COOKIE_NAME)
        self.CRYPT_SECRET_KEY = str(self.partner_config.CRYPT_SECRET_KEY)  # Because hmac.new DOES NOT accept unicode
        self.COOKIE_CRYPT_KEY = getattr(self.partner_config, 'COOKIE_CRYPT_KEY', '1sBGV1c978bb0366')
        self.CRYPT_URL_PREFFIX = getattr(self.partner_config, 'CRYPT_URL_PREFFIX', '/')
        # https://st.yandex-team.ru/ANTIADB-1796 нужно добавить пустой префикс '/', чтобы была возможность расшифровывать урлы без префикса
        # так как урлы, зашифрованные под безкуковый домен, будут без префикса
        self.CRYPT_URL_OLD_PREFFIXES = getattr(self.partner_config, 'CRYPT_URL_OLD_PREFFIXES', []) + ['/']
        if self.ENCRYPT_TO_THE_TWO_DOMAINS and self.cookieless_path_prefix != '/':  # ANTIADB-2741
            self.CRYPT_URL_OLD_PREFFIXES += [self.cookieless_path_prefix]
        self.CRYPT_URL_RANDOM_PREFFIXES = getattr(self.partner_config, 'CRYPT_URL_RANDOM_PREFFIXES', [])
        self.CRYPT_PREFFIXES = re_merge([re2.escape(preffix) for preffix in ([self.CRYPT_URL_PREFFIX] + self.CRYPT_URL_OLD_PREFFIXES + self.CRYPT_URL_RANDOM_PREFFIXES)])
        self.PARTNER_TOKENS = self.partner_config.PARTNER_TOKENS
        # если в конфиге не прилетел PUBLISHER_SECRET_KEY, то подставляем первые 10 символов первого партнерского токена, см https://st.yandex-team.ru/PCODE-5678#1511515655000
        self.PUBLISHER_SECRET_KEY = getattr(self.partner_config, 'PUBLISHER_SECRET_KEY', self.PARTNER_TOKENS[0][:10])
        self.PARTNER_ACCEL_REDIRECT_ENABLED = getattr(self.partner_config, 'PARTNER_ACCEL_REDIRECT_ENABLED', False)

        self.CRYPTED_URL_MIXING_TEMPLATE = tuple((str(s), n) for s, n in getattr(self.partner_config, 'CRYPTED_URL_MIXING_TEMPLATE', DEFAULT_URL_TEMPLATE))
        self.CRYPTED_URL_MIN_LENGTH = getattr(self.partner_config, 'CRYPTED_URL_MIN_LENGTH', 0)
        self.RAW_URL_MIN_LENGTH = calculate_decrypted_url_length(self.CRYPTED_URL_MIN_LENGTH, self.CRYPTED_URL_MIXING_TEMPLATE, name)

        self.DISABLE_ADB_ENABLED = getattr(self.partner_config, 'DISABLE_ADB_ENABLED', False)
        # Замена макроса __ADB_FUNCTIONS__ во всех запросах https://st.yandex-team.ru/ANTIADB-2212
        self.REPLACE_ADB_FUNCTIONS = getattr(self.partner_config, 'REPLACE_ADB_FUNCTIONS', False)

        self.HIDE_GRAB_HEADER_RE = system_config.HIDE_GRAB_HEADER_RE
        self.HIDE_URI_PATH_HEADER_RE = system_config.HIDE_URI_PATH_HEADER_RE
        self.HIDE_META_ARGS_HEADER_MAX_SIZE = int(getattr(self.partner_config, 'HIDE_META_ARGS_HEADER_MAX_SIZE', system_config.HIDE_META_ARGS_HEADER_MAX_SIZE))
        self.HIDE_META_ARGS_ENABLED = getattr(self.partner_config, 'HIDE_META_ARGS_ENABLED', False)
        self.IMAGE_URLS_CRYPTING_RATIO = float(getattr(self.partner_config, 'IMAGE_URLS_CRYPTING_PROBABILITY', 100)) / 100
        SERVICE_SLB_URL_RE = getattr(self.partner_config, 'SERVICE_SLB_URL_RE', [])  # https://st.yandex-team.ru/ANTIADB-1038
        self.SEED_CHANGE_PERIOD = getattr(self.partner_config, 'SEED_CHANGE_PERIOD', 1)
        self.SEED_CHANGE_TIME_SHIFT_MINUTES = getattr(self.partner_config, 'SEED_CHANGE_TIME_SHIFT_MINUTES', 0)
        self.DIV_SHIELD_ENABLE = getattr(self.partner_config, 'DIV_SHIELD_ENABLE', False)  # https://st.yandex-team.ru/ANTIADB-1244
        self.REMOVE_ATTRIBUTE_ID = getattr(self.partner_config, 'REMOVE_ATTRIBUTE_ID', False)   # https://st.yandex-team.ru/ANTIADB-1285
        self.DISABLE_SHADOW_DOM = getattr(self.partner_config, 'DISABLE_SHADOW_DOM', False)  # https://st.yandex-team.ru/ANTIADB-1254
        self.HIDE_LINKS = getattr(self.partner_config, 'HIDE_LINKS', False)  # https://st.yandex-team.ru/ANTIADB-1536
        self.INVERTED_COOKIE_ENABLED = getattr(self.partner_config, 'INVERTED_COOKIE_ENABLED', False)  # https://st.yandex-team.ru/ANTIADB-1543
        self.DEBUG_LOGGING_ENABLE = getattr(self.partner_config, 'DEBUG_LOGGING_ENABLE', False)
        self.AIM_BANNER_ID_DEBUG_VALUE = getattr(self.partner_config, 'AIM_BANNER_ID_DEBUG_VALUE', None) if service_config.ENV_TYPE == 'staging' or service_config.LOCAL_RUN else None
        self.use_cache = getattr(self.partner_config, 'USE_CACHE', False)
        partner_no_cache_url_re = getattr(self.partner_config, 'NO_CACHE_URL_RE', None) or []
        self.no_cache_url_re = re2.compile(re_completeurl(system_config.NO_CACHE_URL_RE + partner_no_cache_url_re))
        self.no_cache_url_re_match = re2.compile(re_completeurl(system_config.NO_CACHE_URL_RE + partner_no_cache_url_re, match_only=True))
        self.url_keep_query_params = ["imp-id", "layout-config"]

        # https://st.yandex-team.ru/ANTIADB-1829
        new_detect_script_url = getattr(self.partner_config, 'NEW_DETECT_SCRIPT_URL', '')
        if new_detect_script_url:
            self.new_detect_script_url = urlparse(new_detect_script_url)
        else:
            self.new_detect_script_url = None
        self.new_detect_script_dc = getattr(self.partner_config, 'NEW_DETECT_SCRIPT_DC', [])

        active_experiment = get_active_experiment(getattr(self.partner_config, 'EXPERIMENTS', []))
        self.experiment_on = False
        self.experiment_type = Experiments.NONE if service_config.END2END_TESTING else active_experiment.get('EXPERIMENT_TYPE', Experiments.NONE)
        self.experiment_percent = active_experiment.get('EXPERIMENT_PERCENT', None)
        exp_start_dt = active_experiment.get('EXPERIMENT_START', None)
        exp_duration = active_experiment.get('EXPERIMENT_DURATION', None)
        self.experiment_range = None
        self.experiment_device = active_experiment.get('EXPERIMENT_DEVICE', None) or [UserDevice.DESKTOP, UserDevice.MOBILE]
        if self.experiment_type != Experiments.NONE and self.experiment_percent is not None and exp_start_dt is not None and exp_duration is not None:
            self.experiment_range = (exp_start_dt, exp_start_dt + timedelta(hours=exp_duration))
            self.experiment_on = True

        self.bypass_by_uids = getattr(self.partner_config, 'BYPASS_BY_UIDS', False)
        self.COUNT_TO_XHR = getattr(self.partner_config, 'COUNT_TO_XHR', False)

        IMAGE_TO_IFRAME_URL_RE = getattr(self.partner_config, 'IMAGE_TO_IFRAME_URL_RE', [])
        # TODO: remove IMAGE_TO_IFRAME_URL_LENGTH after rules from https://st.yandex-team.ru/ANTIADBSUP-1346 will disappear:
        image_to_iframe_url_len = getattr(self.partner_config, 'IMAGE_TO_IFRAME_URL_LENGTH', 0)
        if image_to_iframe_url_len == 0:
            self.IMAGE_TO_IFRAME_URL_LENGTH = self.RAW_URL_MIN_LENGTH
        else:
            self.IMAGE_TO_IFRAME_URL_LENGTH = calculate_decrypted_url_length(image_to_iframe_url_len, self.CRYPTED_URL_MIXING_TEMPLATE, name)
        self.IMAGE_TO_IFRAME_URL_IS_RELATIVE = getattr(self.partner_config, 'IMAGE_TO_IFRAME_URL_IS_RELATIVE', False)
        self.IMAGE_TO_IFRAME_CHANGING_RATIO = float(getattr(self.partner_config, 'IMAGE_TO_IFRAME_CHANGING_PROBABILITY', 100)) / 100

        CRYPT_URL_RE = system_config.CRYPT_URL_RE
        if self.encryption_step_enabled(EncryptionSteps.advertising_crypt):
            CRYPT_URL_RE = CRYPT_URL_RE + sum(map(lambda x: x.CRYPT_URL_RE, enabled_ad_systems), [])
        if self.encryption_step_enabled(EncryptionSteps.partner_content_url_crypt):
            CRYPT_URL_RE = CRYPT_URL_RE + self.partner_config.CRYPT_URL_RE

        CRYPT_URL_RE_WITH_COUNTS = CRYPT_URL_RE + bk_config.RTB_COUNT_URL_RE + bk_config.BANNER_SYSTEM_ABUSE_URL_RE
        CRYPT_URL_RE_WITH_NANPU = CRYPT_URL_RE_WITH_COUNTS + bk_config.NANPU_CRYPT_URL_RE
        if self.encryption_step_enabled(EncryptionSteps.advertising_crypt) and getattr(self.partner_config, 'CRYPT_BS_COUNT_URL', False) and AdSystem.BK in self.AD_SYSTEMS:
            CRYPT_URL_RE.extend(bk_config.RTB_COUNT_URL_RE)
        # https://st.yandex-team.ru/ANTIADB-1758
        BYPASS_URL_RE = getattr(self.partner_config, 'BYPASS_URL_RE', [])

        # this seed is expected to be used in one way crypting features
        indecryptable_seed = generate_seed(salt=version,
                                           change_period=self.SEED_CHANGE_PERIOD,
                                           time_shift_minutes=self.SEED_CHANGE_TIME_SHIFT_MINUTES)

        self.CRYPT_IN_LOWERCASE = getattr(self.partner_config, 'CRYPT_IN_LOWERCASE', False)
        CRYPT_BODY_RE = []
        REPLACE_BODY_WITH_TAGS_RE = {}
        FIX_JS_TAGS_RE = {}
        REPLACE_BODY_RE = {}
        if self.encryption_step_enabled(system_config.EncryptionSteps.advertising_crypt):
            for ad_system in [system_config] + enabled_ad_systems:
                REPLACE_BODY_RE.update(getattr(ad_system, 'REPLACE_BODY_RE', {}))
                CRYPT_BODY_RE.extend(getattr(ad_system, 'CRYPT_BODY_RE', []))
            if self.encryption_step_enabled(system_config.EncryptionSteps.tags_crypt):
                tags_crypt_dict = dict()
                key = get_key(self.CRYPT_SECRET_KEY, indecryptable_seed)
                for tag in system_config.COMMON_TAGS_REPLACE_LIST:
                    case_insensitive_tag = ''.join(['[' + ''.join(i) + ']' for i in zip(tag.lower(), tag.upper())])
                    tag_replace_regexp = re_merge(map(lambda s: s.format(case_insensitive_tag), system_config.TAG_REPLACE_REGEXP_TEMPLATE_LIST))
                    crypted_tag = encrypt_xor(tag, key)
                    # HTML tag names can only start with ASCII letters, and cannot contain '_'.
                    # Also we need '-' in tag because of Shadow DOM support. See https://st.yandex-team.ru/ANTIADB-1083
                    random_letter = chr(ord(key[0]) % 26 + ord('a'))
                    crypted_tag = random_letter + '-' + crypted_tag.replace('_', random_letter)
                    # https://st.yandex-team.ru/ANTIADB-1070 JS tagName returns tag in upper case.
                    crypted_tag = crypted_tag.upper()
                    tags_crypt_dict.update({tag_replace_regexp: crypted_tag})
                    FIX_JS_TAGS_RE.update({system_config.FIX_JS_TAGS_RE.format(case_insensitive_tag): r'["{}"]'.format(tag)})
                REPLACE_BODY_WITH_TAGS_RE.update(tags_crypt_dict)
                REPLACE_BODY_WITH_TAGS_RE.update({r'</head>': system_config.INSERT_STYLE_TEMPLATE.format(','.join(tags_crypt_dict.values()))})

        REPLACE_BODY_RE_PER_URL = getattr(self.partner_config, 'REPLACE_BODY_RE_PER_URL', dict())
        REPLACE_BODY_RE_EXCEPT_URL = getattr(self.partner_config, 'REPLACE_BODY_RE_EXCEPT_URL', dict())
        if self.encryption_step_enabled(EncryptionSteps.partner_content_class_crypt):
            partner_crypt_body_re = getattr(self.partner_config, 'CRYPT_BODY_RE', [])
            CRYPT_BODY_RE.extend([r for r in partner_crypt_body_re if r not in CRYPT_BODY_RE])
            REPLACE_BODY_RE.update(getattr(self.partner_config, 'REPLACE_BODY_RE', dict()))

        # Visibility protection
        # https://st.yandex-team.ru/ANTIADB-1692, https://st.yandex-team.ru/ANTIADB-1716
        self.xhr_protection_fix_style_src_re = v_protect.xhr_fix_style_src_re
        self.xhr_protection_fix_style_src_re_match = v_protect.xhr_fix_style_src_re_match
        self.script_protection_fix_style_src_re = v_protect.script_fix_style_src_re
        self.script_protection_fix_style_src_re_match = v_protect.script_fix_style_src_re_match
        self.v_protect_class_re = getattr(self.partner_config, 'VISIBILITY_PROTECTION_CLASS_RE', None)
        self.xhr_protection_on = getattr(self.partner_config, 'XHR_PROTECT', False) and self.v_protect_class_re is not None
        self.script_protection_on = getattr(self.partner_config, 'LOADED_JS_PROTECT', False) and self.v_protect_class_re is not None

        self.inject_inline_js = getattr(self.partner_config, 'INJECT_INLINE_JS', None)
        self.break_object_current_script = getattr(self.partner_config, 'BREAK_OBJECT_CURRENT_SCRIPT', False)
        if self.break_object_current_script:
            self.inject_inline_js = (self.inject_inline_js or '') + v_protect.OBJECT_CURRENT_SCRIPT_BREAKER
        self.INJECT_INLINE_JS_POSITION = getattr(self.partner_config, 'INJECT_INLINE_JS_POSITION', InjectInlineJSPosition.HEAD_END)

        self.BLOCK_TO_IFRAME_SELECTORS = getattr(self.partner_config, 'BLOCK_TO_IFRAME_SELECTORS', [])
        # https://st.yandex-team.ru/ANTIADB-2900
        self.CSP_PATCH = getattr(self.partner_config, 'CSP_PATCH', {})
        self.ADDITIONAL_PCODE_PARAMS = getattr(self.partner_config, 'ADDITIONAL_PCODE_PARAMS', {})

        self.NETWORK_FAILS_RETRY_THRESHOLD = int(getattr(self.partner_config, 'NETWORK_FAILS_RETRY_THRESHOLD', 0))
        if self.NETWORK_FAILS_RETRY_THRESHOLD:
            self.PROXY_CLIENT_REQUEST_CONFIG = self.PROXY_CLIENT_REQUEST_CONFIG.copy()
            self.PROXY_CLIENT_REQUEST_CONFIG['connect_timeout'] = (self.NETWORK_FAILS_RETRY_THRESHOLD / 1000.0) * 0.9

        REPLACE_BODY_WITH_TAGS_RE.update(REPLACE_BODY_RE)
        PARTNER_MANUAL_CRYPT_RELATIVE_URL_RE = getattr(self.partner_config, 'CRYPT_RELATIVE_URL_RE', []) if self.encryption_step_enabled(EncryptionSteps.partner_content_url_crypt) else []
        PARTNER_AUTO_CRYPT_RELATIVE_URL_RE = system_config.CRYPT_RELATIVE_URL_RE if (self.encryption_step_enabled(EncryptionSteps.crypt_relative_urls_automatically)
                                                                                     and self.encryption_step_enabled(EncryptionSteps.partner_content_url_crypt)) else []
        self.EXCLUDE_COOKIE_FORWARD = system_config.EXCLUDE_COOKIE_FORWARD + getattr(self.partner_config, 'EXCLUDE_COOKIE_FORWARD', [])
        PARTNER_URL_RE = self.partner_config.PROXY_URL_RE + self.partner_config.CRYPT_URL_RE

        # https://st.yandex-team.ru/ANTIADB-1255
        REPLACE_RESOURCE_WITH_XHR_RE = getattr(self.partner_config, 'REPLACE_SCRIPT_WITH_XHR_RE', [])
        if REPLACE_RESOURCE_WITH_XHR_RE:
            REPLACE_RESOURCE_WITH_XHR_RE = re_merge(system_config.REPLACE_RESOURCE_WITH_XHR_RE).format(SRC_RE=re_merge(REPLACE_RESOURCE_WITH_XHR_RE))
        # https://st.yandex-team.ru/ANTIADB-1394
        REPLACE_RESOURCE_WITH_XHR_SYNC_RE = getattr(self.partner_config, 'REPLACE_RESOURCE_WITH_XHR_SYNC_RE', [])
        if REPLACE_RESOURCE_WITH_XHR_SYNC_RE:
            REPLACE_RESOURCE_WITH_XHR_SYNC_RE = re_merge(system_config.REPLACE_RESOURCE_WITH_XHR_RE).format(SRC_RE=re_merge(REPLACE_RESOURCE_WITH_XHR_SYNC_RE))
        self.REMOVE_SCRIPTS_AFTER_RUN = getattr(self.partner_config, 'REMOVE_SCRIPTS_AFTER_RUN', False)  # https://st.yandex-team.ru/ANTIADB-1257
        self.ADD_NONCE = getattr(self.partner_config, 'ADD_NONCE', False)

        FOLLOW_REDIRECT_URL_RE = system_config.FOLLOW_REDIRECT_URL_RE + sum([ad_system.FOLLOW_REDIRECT_URL_RE for ad_system in enabled_ad_systems], []) + \
                                      getattr(self.partner_config, 'FOLLOW_REDIRECT_URL_RE', [])
        CLIENT_REDIRECT_URL_RE = system_config.CLIENT_REDIRECT_URL_RE + sum([ad_system.CLIENT_REDIRECT_URL_RE for ad_system in enabled_ad_systems], []) + \
                                      getattr(self.partner_config, 'CLIENT_REDIRECT_URL_RE', [])

        self.logging_level = logging.DEBUG if self.DEBUG_LOGGING_ENABLE else service_config.LOGGING_LEVEL
        self.crypt_mime_re = re2.compile(CRYPT_MIME_RE, re2.IGNORECASE)
        # accel redirect for proxy nginx
        self.accel_url_re = re2.compile(re_merge(ACCEL_REDIRECT_URL_RE), re2.IGNORECASE) if ACCEL_REDIRECT_URL_RE else None
        # accel redirect for partner nginx
        self.partner_accel_url_re = re2.compile(re_merge(PARTNER_ACCEL_REDIRECT_URL_RE), re2.IGNORECASE) if PARTNER_ACCEL_REDIRECT_URL_RE else None

        self.bk_url_re = static_config.bk_url_re
        self.bk_url_re_match = static_config.bk_url_re_match
        self.bk_auction_meta_url_re = static_config.bk_meta_auction_url_re
        self.bk_auction_meta_url_re_match = static_config.bk_meta_auction_url_re_match
        self.bk_auction_meta_bulk_url_re = static_config.bk_meta_bulk_auction_url_re
        self.bk_auction_meta_bulk_url_re_match = static_config.bk_meta_bulk_auction_url_re_match
        self.bk_vmap_auction_url_re = static_config.bk_vmap_auction_url_re
        self.bk_vmap_auction_url_re_match = static_config.bk_vmap_auction_url_re_match
        self.bk_rtb_count_url_re = static_config.bk_rtb_count_url_re
        self.meta_js_obj_extractor_re = static_config.meta_js_obj_extractor_re
        self.pjson_re = static_config.pjson_re

        self.hide_grab_re = static_config.hide_grab_re
        self.current_grab_header_name = generate_hide_meta_args_header_name(indecryptable_seed, self.HIDE_GRAB_HEADER_RE)  # should change once an hour
        self.hide_uri_path_re = static_config.hide_uri_path_re
        self.current_hide_uri_path_header_name = generate_hide_meta_args_header_name(indecryptable_seed, self.HIDE_URI_PATH_HEADER_RE)
        self.crypt_html5_relative_url_re = static_config.crypt_html5_relative_url_re
        self.html5_banner_re = static_config.html5_banner_re
        self.html5_base_path_re = static_config.html5_base_path_re
        self.storage_mds_url_re = static_config.storage_mds_url_re
        self.storage_mds_url_re_match = static_config.storage_mds_url_re_match
        self.self_closing_tags_re = static_config.self_closing_tags_re
        self.pcode_js_url_re = static_config.pcode_js_url_re
        self.pcode_js_url_re_match = static_config.pcode_js_url_re_match
        self.pcode_loader_url_re_match = static_config.pcode_loader_url_re_match
        self.yandex_metrika_url_re = static_config.yandex_metrika_url_re
        self.yandex_metrika_url_re_match = static_config.yandex_metrika_url_re_match
        self.strm_url_re_match = static_config.strm_url_re_match
        self.dzen_player_url_re_match = static_config.dzen_player_url_re_match
        self.detect_lib_url_re = static_config.detect_lib_url_re
        self.detect_lib_url_re_match = static_config.detect_lib_url_re_match
        self.adfox_url_re = static_config.adfox_url_re
        self.adfox_url_re_match = static_config.adfox_url_re_match
        self.rtb_auction_url_re = static_config.rtb_auction_url_re
        self.rtb_auction_url_re_match = static_config.rtb_auction_url_re_match
        self.rtb_count_url_re = static_config.rtb_count_url_re
        self.rtb_count_url_re_match = static_config.rtb_count_url_re_match
        self.yabs_count_url_re = static_config.yabs_count_url_re
        self.yabs_count_url_re_match = static_config.yabs_count_url_re_match
        self.partner_url_re = re2.compile(re_completeurl(PARTNER_URL_RE, True), re2.IGNORECASE)
        self.partner_url_re_match = re2.compile(re_completeurl(PARTNER_URL_RE, True, match_only=True), re2.IGNORECASE)

        self.bk_count_and_abuse_url_re = static_config.bk_count_and_abuse_url_re
        self.yandex_url_re = static_config.yandex_url_re
        self.yandex_url_re_match = static_config.yandex_url_re_match
        self.yandex_static_url_re = re2.compile(re_completeurl(self.YANDEX_STATIC_URL_RE, True)) if self.YANDEX_STATIC_URL_RE else None
        self.yandex_static_url_re_match = re2.compile(re_completeurl(self.YANDEX_STATIC_URL_RE, True, match_only=True)) if self.YANDEX_STATIC_URL_RE else None
        self.rambler_url_re = static_config.rambler_url_re
        self.rambler_url_re_match = static_config.rambler_url_re_match
        self.rambler_crypt_relative_url_re = static_config.rambler_crypt_relative_url_re
        self.rambler_auction_url_re = static_config.rambler_auction_url_re
        self.rambler_auction_url_re_match = static_config.rambler_auction_url_re_match
        self.rambler_js_obj_extractor_re = static_config.rambler_js_obj_extractor_re

        self.nanpu_url_re = static_config.nanpu_url_re
        self.nanpu_url_re_match = static_config.nanpu_url_re_match
        self.nanpu_startup_url_re = static_config.nanpu_startup_url_re
        self.nanpu_startup_url_re_match = static_config.nanpu_startup_url_re_match
        self.nanpu_auction_url_re = static_config.nanpu_auction_url_re
        self.nanpu_auction_url_re_match = static_config.nanpu_auction_url_re_match
        self.nanpu_bk_auction_url_re_match = static_config.nanpu_bk_auction_url_re_match
        self.nanpu_count_url_re = static_config.nanpu_count_url_re
        self.nanpu_count_url_re_match = static_config.nanpu_count_url_re_match

        self.service_slb_url_re = re2.compile(re_completeurl(SERVICE_SLB_URL_RE, True), re2.IGNORECASE) if SERVICE_SLB_URL_RE else None
        self.service_slb_url_re_match = re2.compile(re_completeurl(SERVICE_SLB_URL_RE, True, match_only=True), re2.IGNORECASE) if SERVICE_SLB_URL_RE else None

        self.bk_counters_url_re = static_config.bk_counters_url_re
        self.bk_counters_url_re_match = static_config.bk_counters_url_re_match

        self.deny_headers_forward = static_config.deny_headers_forward
        self.deny_headers_backward = static_config.deny_headers_backward
        self.deny_headers_backward_bk = static_config.deny_headers_backward_bk
        self.deny_headers_backward_proxy = static_config.deny_headers_backward_proxy

        self.crypt_url_re = re2.compile(re_expand(CRYPT_URL_RE))
        self.crypt_url_re_with_counts = re2.compile(re_expand(CRYPT_URL_RE_WITH_COUNTS))
        self.crypt_url_re_with_nanpu = re2.compile(re_expand(CRYPT_URL_RE_WITH_NANPU))
        self.bypass_url_re = re2.compile(re_completeurl(BYPASS_URL_RE)) if BYPASS_URL_RE else None
        self.bypass_url_re_match = re2.compile(re_completeurl(BYPASS_URL_RE, match_only=True)) if BYPASS_URL_RE else None
        self.crypt_body_re = re2.compile(re_merge(CRYPT_BODY_RE)) if CRYPT_BODY_RE else None
        self.replace_body_with_tags_re = compile_replace_dict(REPLACE_BODY_WITH_TAGS_RE) if REPLACE_BODY_WITH_TAGS_RE else None
        self.replace_body_re = compile_replace_dict(REPLACE_BODY_RE) if REPLACE_BODY_RE else None
        # https://st.yandex-team.ru/ANTIADB-2719
        self.replace_body_re_per_url = {}
        self.replace_body_re_except_url = {}
        for url_re, replace_dict in REPLACE_BODY_RE_PER_URL.items():
            self.replace_body_re_per_url[re2.compile(re_completeurl(url_re))] = compile_replace_dict(replace_dict)
        for url_re, replace_dict in REPLACE_BODY_RE_EXCEPT_URL.items():
            self.replace_body_re_except_url[re2.compile(re_completeurl(url_re))] = compile_replace_dict(replace_dict)

        self.fix_js_tags_re = compile_replace_dict(FIX_JS_TAGS_RE)
        if PARTNER_MANUAL_CRYPT_RELATIVE_URL_RE or PARTNER_AUTO_CRYPT_RELATIVE_URL_RE:
            self.partner_crypt_relative_url_re = re2.compile(re_merge([re_expand_relative_urls(PARTNER_MANUAL_CRYPT_RELATIVE_URL_RE)] + PARTNER_AUTO_CRYPT_RELATIVE_URL_RE))
        else:
            self.partner_crypt_relative_url_re = None
        self.follow_redirect_url_re = re2.compile(re_completeurl(FOLLOW_REDIRECT_URL_RE, True), re2.IGNORECASE) if FOLLOW_REDIRECT_URL_RE else None
        self.follow_redirect_url_re_match = re2.compile(re_completeurl(FOLLOW_REDIRECT_URL_RE, True, match_only=True), re2.IGNORECASE) if FOLLOW_REDIRECT_URL_RE else None
        self.client_redirect_url_re = re2.compile(re_completeurl(CLIENT_REDIRECT_URL_RE, True), re2.IGNORECASE) if CLIENT_REDIRECT_URL_RE else None
        self.client_redirect_url_re_match = re2.compile(re_completeurl(CLIENT_REDIRECT_URL_RE, True, match_only=True), re2.IGNORECASE) if CLIENT_REDIRECT_URL_RE else None
        self.image_to_iframe_url_re = re2.compile(re_merge(IMAGE_TO_IFRAME_URL_RE), re2.IGNORECASE) if IMAGE_TO_IFRAME_URL_RE else None

        self.replace_resource_with_xhr_re = tuple()
        if self.encryption_step_enabled(EncryptionSteps.partner_content_url_crypt):
            if REPLACE_RESOURCE_WITH_XHR_RE:
                self.replace_resource_with_xhr_re += (('async', re2.compile(REPLACE_RESOURCE_WITH_XHR_RE, re2.IGNORECASE)), )
            if REPLACE_RESOURCE_WITH_XHR_SYNC_RE:
                self.replace_resource_with_xhr_re += (('sync', re2.compile(REPLACE_RESOURCE_WITH_XHR_SYNC_RE, re2.IGNORECASE)), )

        # PARTNER PCODE CONFIG OBJECT (https://st.yandex-team.ru/ANTIADB-479)
        self.pcode_params = {'detect': {'links': self.PARTNER_DETECT_LINKS + system_config.DETECT_LINKS,
                                        'trusted': self.DETECT_TRUSTED_LINKS,
                                        'custom': system_config.DETECT_HTML + self.PARTNER_DETECT_HTML,
                                        'iframes': self.PARTNER_DETECT_IFRAME},
                             'pid': name,
                             'extuidCookies': self.EXTUID_COOKIE_NAMES,
                             'cookieMatching': {'publisherTag': self.EXTUID_TAG,
                                                'types': self.CM_TYPES,
                                                'redirectUrl': self.CM_REDIRECT_URL,
                                                'imageUrl': self.CM_IMAGE_URL,
                                                'publisherKey': self.PUBLISHER_SECRET_KEY,
                                                'cryptedUidUrl': system_config.CRYPTED_YAUID_URL,
                                                'cryptedUidCookie': system_config.CRYPTED_YAUID_COOKIE_NAME,
                                                'cryptedUidTTL': system_config.CRYPTED_YAUID_TTL,
                                                },
                             'cookieDomain': {'type': self.DETECT_COOKIE_TYPE,
                                              'list': self.DETECT_COOKIE_DOMAINS,
                                              },
                             'countToXhr': self.COUNT_TO_XHR,
                             'cookieTTL': system_config.DETECT_COOKIE_TTL,
                             'log': {'percent': 0,  # bamboozled to jstrace  https://st.yandex-team.ru/ANTIADB-904
                                     },
                             'rtbRequestViaScript': self.RTB_AUCTION_VIA_SCRIPT,
                             'treeProtection': {'enabled': self.DIV_SHIELD_ENABLE},
                             'hideMetaArgsUrlMaxSize': self.CRYPTED_URL_MIN_LENGTH,  # до какой длины сокращать урл на мету
                             'disableShadow': self.DISABLE_SHADOW_DOM,
                             'cookieName': self.CURRENT_COOKIE,
                             'deprecatedCookies': self.DEPRECATED_COOKIES,
                             'hideLinks': self.HIDE_LINKS,
                             'invertedCookieEnabled': self.INVERTED_COOKIE_ENABLED,
                             'removeAttributeId': self.REMOVE_ATTRIBUTE_ID,
                             'blockToIframeSelectors': self.BLOCK_TO_IFRAME_SELECTORS,
                             'additionalParams': self.ADDITIONAL_PCODE_PARAMS,
                             }
        # insert cookie names for mobile detect https://st.yandex-team.ru/ANTIADB-2690
        cookie_params = {
            'cookieName': self.CURRENT_COOKIE,
            'deprecatedCookies': self.DEPRECATED_COOKIES,
        }
        self.cookie_replace_dict = compile_replace_dict({r'[\'\"]__ADB_COOKIES__[\'\"]': bytes(dumps(cookie_params))})

        def get_update_header_values_from_partner_config(attr_name):
            result = {}
            for url_re, headers_update_dict in getattr(self.partner_config, attr_name, dict()).iteritems():
                url_for_headers_update_re = re2.compile(re_completeurl(url_re, True), re2.IGNORECASE)
                result[url_for_headers_update_re] = dict(headers_update_dict)
            return result

        self.UPDATE_RESPONSE_HEADERS_VALUES = get_update_header_values_from_partner_config('UPDATE_RESPONSE_HEADERS_VALUES')
        self.UPDATE_REQUEST_HEADER_VALUES = get_update_header_values_from_partner_config('UPDATE_REQUEST_HEADER_VALUES')

        self.CRYPT_LINK_HEADER = getattr(self.partner_config, 'CRYPT_LINK_HEADER', False)
        if self.CRYPT_LINK_HEADER:
            self.crypt_link_header_re = re2.compile(r'\<({})\>'.format(re_completeurl(CRYPT_URL_RE)))
            self.crypt_link_header_re_match = re2.compile(r'\<({})\>'.format(re_completeurl(CRYPT_URL_RE, match_only=True)))

        if self.experiment_on and self.experiment_type == Experiments.FORCECRY and is_experiment_date(self.experiment_range):
            experiment_expires = int((self.experiment_range[1] - datetime(1970, 1, 1)).total_seconds() * 1000)
            self.pcode_params['forcecry'] = dict(
                enabled=True,
                percent=self.experiment_percent,
                expires=experiment_expires
            )
        self.crypted_functions = re2.sub(r"\s*", "",  # remove whitespaces and newline
                                         ""","fn": {{"encodeCSS": {ENCODE_CSS_FUNC},
                                                     "encodeUrl": {ENCODE_URL_FUNC},
                                                     "decodeUrl": {DECODE_URL_FUNC},
                                                     "isEncodedUrl": {IS_ENCODED_URL_FUNC}
                                                     }}""") \
            .format(ENCODE_CSS_FUNC=system_config.ENCODE_CSS_FUNC,  # insert functions
                    ENCODE_URL_FUNC=system_config.ENCODE_URL_FUNC,
                    DECODE_URL_FUNC=system_config.DECODE_URL_FUNC,
                    IS_ENCODED_URL_FUNC=system_config.IS_ENCODED_URL_FUNC)\
            .replace('{trailing_slash}', str(self.CRYPT_ENABLE_TRAILING_SLASH).lower())\
            .replace('{min_length_decoded}', str(self.RAW_URL_MIN_LENGTH)) \
            .replace('{subdocument_url_is_relative}', str(self.IMAGE_TO_IFRAME_URL_IS_RELATIVE).lower()) \
            .replace('{crypted_subdocument_url_min_length}', str(self.IMAGE_TO_IFRAME_URL_LENGTH)) \
            .replace('{urlTemplate}', dumps(self.CRYPTED_URL_MIXING_TEMPLATE))
        # tests
        self.TEST_DATA = getattr(self.partner_config, 'TEST_DATA', dict(valid_domains=list(), invalid_domains=list()))

        # disable detect, см. https://st.yandex-team.ru/ANTIADB-1104, https://st.yandex-team.ru/ANTIADB-1175
        self.DISABLE_DETECT = getattr(self.partner_config, 'DISABLE_DETECT', False)

        # https://st.yandex-team.ru/ANTIADB-2513
        self.force_yandex_url_re = static_config.force_yandex_url_re
        self.force_yandex_url_re_match = static_config.force_yandex_url_re_match

        # https://st.yandex-team.ru/ANTIADB-1968
        self.webmaster_data = getattr(self.partner_config, 'WEBMASTER_DATA', {})
        self.turbo_redirect_script_re = static_config.turbo_redirect_script_re
        self.turbo_redirect_script_re_match = static_config.turbo_redirect_script_re_match
        self.autoredirect_replace_re = None
        if self.webmaster_data:
            # trying read redirect detect script from file cache
            path_to_script = os.path.join(service_config.PERSISTENT_VOLUME_PATH, url_to_filename(AUTOREDIRECT_DETECT_LIB_PATH))
            if os.path.exists(path_to_script):
                with open(path_to_script, mode='rb') as f:
                    self.autoredirect_replace_re = compile_replace_dict({AUTOREDIRECT_REPLACE_RE: f.read()})

    def to_dict(self):
        attrs = filter(lambda y: all(map(lambda x: x.isupper() or x == '_', y)), dir(self.partner_config))
        d = dict()
        for attr in attrs:
            d[attr] = getattr(self.partner_config, attr)
        if 'ENCRYPTION_STEPS' in d:
            d['ENCRYPTION_STEPS'] = map(int, d['ENCRYPTION_STEPS'])
        if 'DISABLED_BK_AD_TYPES' in d:
            d['DISABLED_BK_AD_TYPES'] = map(int, d['DISABLED_BK_AD_TYPES'])
        d['version'] = self.version
        return d

    def encryption_step_enabled(self, step):
        return is_bit_enabled(self.ENCRYPTION_STEPS, step)
