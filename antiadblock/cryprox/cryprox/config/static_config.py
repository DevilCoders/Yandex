"""
The part is not depending on partner config
"""
import re2

from antiadblock.cryprox.cryprox.common.tools.regexp import re_completeurl, re_expand, re_merge
from .ad_systems import AD_SYSTEM_CONFIG, YANDEX_AD_SYSTEMS
from . import bk as bk_config
from . import system as system_config
from . import adfox as adfox_config
from . import rambler as rambler_config

re2._MAXCACHE = 5000

YANDEX_AD_SYSTEMS_CONFIGS = [AD_SYSTEM_CONFIG.get(s) for s in YANDEX_AD_SYSTEMS]

# YANDEX
YANDEX_URL_RE = system_config.PROXY_URL_RE + sum([service.BANNER_SYSTEM_URL_RE + service.PROXY_URL_RE for service in YANDEX_AD_SYSTEMS_CONFIGS], [])
FORCE_YANDEX_URL_RE = sum([getattr(service, 'FORCE_YANDEX_URL_RE', []) for service in YANDEX_AD_SYSTEMS_CONFIGS], [])
RTB_AUCTION_URL_RE = sum([getattr(service, 'RTB_AUCTION_URL_RE', []) for service in YANDEX_AD_SYSTEMS_CONFIGS], [])
RTB_COUNT_URL_RE = sum([getattr(service, 'RTB_COUNT_URL_RE', []) for service in YANDEX_AD_SYSTEMS_CONFIGS], [])
MATCH_YANDEX_DOMAIN_WITH_TLD_RE = re2.compile(r'yandex\.({})'.format(system_config.YANDEX_CCTLDS_RE))

bk_url_re = re2.compile(re_completeurl(bk_config.BANNER_SYSTEM_URL_RE, True), re2.IGNORECASE)
bk_url_re_match = re2.compile(re_completeurl(bk_config.BANNER_SYSTEM_URL_RE, True, match_only=True), re2.IGNORECASE)
bk_count_and_abuse_url_re = re2.compile(re_expand(bk_config.RTB_COUNT_URL_RE + bk_config.BANNER_SYSTEM_ABUSE_URL_RE))
bk_rtb_count_url_re = re2.compile(re_expand(bk_config.RTB_COUNT_URL_RE))
meta_js_obj_extractor_re = re2.compile(bk_config.META_JS_OBJ_EXTRACTOR_RE, re2.IGNORECASE)
pjson_re = re2.compile(bk_config.PJSON_RE, re2.IGNORECASE)
hide_grab_re = re2.compile('^' + system_config.HIDE_GRAB_HEADER_RE + '$', re2.IGNORECASE)
hide_uri_path_re = re2.compile('^' + system_config.HIDE_URI_PATH_HEADER_RE + '$', re2.IGNORECASE)
crypt_html5_relative_url_re = re2.compile(re_merge(bk_config.CRYPT_HTML5_RELATIVE_URL_RE))
html5_banner_re = re2.compile(re_merge(bk_config.HTML5_BANNER_RE))
html5_base_path_re = re2.compile(re_merge(bk_config.HTML5_BASE_PATH_RE))
storage_mds_url_re = re2.compile(re_completeurl(bk_config.STORAGE_MDS_URL_RE, True), re2.IGNORECASE)
storage_mds_url_re_match = re2.compile(re_completeurl(bk_config.STORAGE_MDS_URL_RE, True, match_only=True), re2.IGNORECASE)
self_closing_tags_re = re2.compile(system_config.SELF_CLOSING_TAGS_RE)
pcode_js_url_re = re2.compile(re_completeurl(bk_config.PCODE_JS_REPLACE_URL_RE + system_config.PROXY_URL_RE, True), re2.IGNORECASE)
pcode_js_url_re_match = re2.compile(re_completeurl(bk_config.PCODE_JS_REPLACE_URL_RE + system_config.PROXY_URL_RE, True, match_only=True), re2.IGNORECASE)
pcode_loader_url_re_match = re2.compile(re_completeurl(bk_config.PCODE_LOADER_URL_RE, True, match_only=True), re2.IGNORECASE)
detect_lib_url_re = re2.compile('^' + re_completeurl([system_config.DETECT_LIB_HOST_RE]), re2.IGNORECASE)
detect_lib_url_re_match = re2.compile('^' + re_completeurl([system_config.DETECT_LIB_HOST_RE], match_only=True), re2.IGNORECASE)
yandex_metrika_url_re = re2.compile(re_completeurl(system_config.YANDEX_METRIKA_URL_RE), re2.IGNORECASE)
yandex_metrika_url_re_match = re2.compile(re_completeurl(system_config.YANDEX_METRIKA_URL_RE, match_only=True), re2.IGNORECASE)
strm_url_re_match = re2.compile(re_completeurl(system_config.STRM_URL_RE, match_only=True), re2.IGNORECASE)
dzen_player_url_re_match = re2.compile(re_completeurl(system_config.DZEN_PLAYER_URL_RE, match_only=True), re2.IGNORECASE)
adfox_url_re = re2.compile(re_completeurl(adfox_config.BANNER_SYSTEM_URL_RE, True), re2.IGNORECASE)
adfox_url_re_match = re2.compile(re_completeurl(adfox_config.BANNER_SYSTEM_URL_RE, True, match_only=True), re2.IGNORECASE)
rtb_auction_url_re = re2.compile(re_completeurl(RTB_AUCTION_URL_RE, True), re2.IGNORECASE)
rtb_auction_url_re_match = re2.compile(re_completeurl(RTB_AUCTION_URL_RE, True, match_only=True), re2.IGNORECASE)
bk_meta_auction_url_re = re2.compile(re_completeurl(bk_config.BK_META_AUCTION_URL_RE, True), re2.IGNORECASE)
bk_meta_auction_url_re_match = re2.compile(re_completeurl(bk_config.BK_META_AUCTION_URL_RE, True, match_only=True), re2.IGNORECASE)
bk_meta_bulk_auction_url_re = re2.compile(re_completeurl(bk_config.BK_META_BULK_AUCTION_URL_RE, True), re2.IGNORECASE)
bk_meta_bulk_auction_url_re_match = re2.compile(re_completeurl(bk_config.BK_META_BULK_AUCTION_URL_RE, True, match_only=True), re2.IGNORECASE)
bk_vmap_auction_url_re = re2.compile(re_completeurl(bk_config.BK_VMAP_AUCTION_URL_RE, True), re2.IGNORECASE)
bk_vmap_auction_url_re_match = re2.compile(re_completeurl(bk_config.BK_VMAP_AUCTION_URL_RE, True, match_only=True), re2.IGNORECASE)
yabs_count_url_re = re2.compile(re_completeurl(bk_config.YABS_COUNT_URL_RE, True), re2.IGNORECASE)
yabs_count_url_re_match = re2.compile(re_completeurl(bk_config.YABS_COUNT_URL_RE, True, match_only=True), re2.IGNORECASE)
rtb_count_url_re = re2.compile(re_completeurl(RTB_COUNT_URL_RE, True), re2.IGNORECASE)
rtb_count_url_re_match = re2.compile(re_completeurl(RTB_COUNT_URL_RE, True, match_only=True), re2.IGNORECASE)

yandex_url_re = re2.compile(re_completeurl(YANDEX_URL_RE, True), re2.IGNORECASE)
yandex_url_re_match = re2.compile(re_completeurl(YANDEX_URL_RE, True, match_only=True), re2.IGNORECASE)
force_yandex_url_re = re2.compile(re_completeurl(FORCE_YANDEX_URL_RE, True), re2.IGNORECASE)
force_yandex_url_re_match = re2.compile(re_completeurl(FORCE_YANDEX_URL_RE, True, match_only=True), re2.IGNORECASE)
rambler_url_re = re2.compile(re_completeurl(rambler_config.PROXY_URL_RE, True), re2.IGNORECASE)
rambler_url_re_match = re2.compile(re_completeurl(rambler_config.PROXY_URL_RE, True, match_only=True), re2.IGNORECASE)
rambler_crypt_relative_url_re = re2.compile(re_merge(rambler_config.CRYPT_RELATIVE_URL_RE))
rambler_auction_url_re = re2.compile(re_completeurl(rambler_config.RTB_AUCTION_URL_RE, True), re2.IGNORECASE)
rambler_auction_url_re_match = re2.compile(re_completeurl(rambler_config.RTB_AUCTION_URL_RE, True, match_only=True), re2.IGNORECASE)
rambler_js_obj_extractor_re = re2.compile(rambler_config.JS_OBJ_EXTRACTOR_RE, re2.IGNORECASE)

bk_counters_url_re = re2.compile(re_completeurl(bk_config.COUNTERS_URL_RE, True), re2.IGNORECASE)
bk_counters_url_re_match = re2.compile(re_completeurl(bk_config.COUNTERS_URL_RE, True, match_only=True), re2.IGNORECASE)

nanpu_url_re = re2.compile(re_completeurl(bk_config.NANPU_URL_RE, True), re2.IGNORECASE)
nanpu_url_re_match = re2.compile(re_completeurl(bk_config.NANPU_URL_RE, True, match_only=True), re2.IGNORECASE)
nanpu_startup_url_re = re2.compile(re_completeurl(bk_config.NANPU_STARTUP_URL_RE, True), re2.IGNORECASE)
nanpu_startup_url_re_match = re2.compile(re_completeurl(bk_config.NANPU_STARTUP_URL_RE, True, match_only=True), re2.IGNORECASE)
nanpu_auction_url_re = re2.compile(re_completeurl(bk_config.NANPU_AUCTION_URL_RE, True), re2.IGNORECASE)
nanpu_auction_url_re_match = re2.compile(re_completeurl(bk_config.NANPU_AUCTION_URL_RE, True, match_only=True), re2.IGNORECASE)
nanpu_bk_auction_url_re_match = re2.compile(re_completeurl(bk_config.NANPU_BK_AUCTION_URL_RE, True, match_only=True), re2.IGNORECASE)
nanpu_count_url_re = re2.compile(re_completeurl(bk_config.NANPU_COUNT_URL_RE, True), re2.IGNORECASE)
nanpu_count_url_re_match = re2.compile(re_completeurl(bk_config.NANPU_COUNT_URL_RE, True, match_only=True), re2.IGNORECASE)


deny_headers_forward = [header.lower() for header in system_config.DENY_HEADERS_FORWARD_RE]
deny_headers_backward = [header.lower() for header in system_config.DENY_HEADERS_BACKWARD_RE]
deny_headers_backward_bk = [header.lower() for header in bk_config.DENY_HEADERS_BACKWARD_RE]
deny_headers_backward_proxy = [header.lower() for header in system_config.DENY_HEADERS_BACKWARD_PROXY_RE]

turbo_redirect_script_re = re2.compile(re_completeurl(system_config.AUTOREDIRECT_SCRIPT_RE), re2.IGNORECASE)
turbo_redirect_script_re_match = re2.compile(re_completeurl(system_config.AUTOREDIRECT_SCRIPT_RE, match_only=True), re2.IGNORECASE)
