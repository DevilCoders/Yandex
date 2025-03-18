import re2
import pytest

from antiadblock.cryprox.cryprox.common.tools.url import UrlClass, classify_url
from antiadblock.cryprox.cryprox.common.tools.regexp import re_completeurl
from antiadblock.cryprox.cryprox.common.visibility_protection import XHR_FIX_STYLE_SRC, SCRIPT_FIX_STYLE_SRC


@pytest.mark.parametrize('url, expected_classes', [
    ('http://yastatic.net/iconostasis/_/test.png', [UrlClass.YANDEX]),
    ('http://betastatic.yastatic.net/iconostasis/_/test.png', [UrlClass.YANDEX]),
    ('http://yandexadexchange.net/claim', [UrlClass.YANDEX]),

    ('https://an.yandex.ru/kakoi-to-path/file', [UrlClass.YANDEX, UrlClass.BK]),
    ('https://yandex.ru/an/kakoi-to-path/file', [UrlClass.YANDEX, UrlClass.BK]),
    ('https://an.yandex.ru/rtbcount/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.BK]),
    ('https://yandex.ru/an/rtbcount/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.BK]),
    ('https://an.yandex.ru/count/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.BK]),
    ('https://yandex.ru/an/count/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.BK]),
    ('https://yandex.com.am/an/count/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.BK]),
    ('https://an.yandex.ru/vmap/ololo', [UrlClass.YANDEX, UrlClass.BK, UrlClass.BK_VMAP_AUCTION]),
    ('https://yandex.ru/ads/vmap/ololo', [UrlClass.YANDEX, UrlClass.BK, UrlClass.BK_VMAP_AUCTION]),
    ('https://yandex.ua/ads/vmap/ololo', [UrlClass.YANDEX, UrlClass.BK, UrlClass.BK_VMAP_AUCTION]),

    ('https://an.yandex.ru/meta/ololo', [UrlClass.YANDEX, UrlClass.BK, UrlClass.RTB_AUCTION, UrlClass.BK_META_AUCTION]),
    ('https://yandex.ru/ads/meta/ololo', [UrlClass.YANDEX, UrlClass.BK, UrlClass.RTB_AUCTION, UrlClass.BK_META_AUCTION]),
    ('https://an.yandex.ru/adfox/123/getBulk/v2?trololo', [UrlClass.YANDEX, UrlClass.ADFOX, UrlClass.BK, UrlClass.RTB_AUCTION]),
    ('https://yandex.ru/ads/adfox/123/getBulk/v2?trololo', [UrlClass.YANDEX, UrlClass.ADFOX, UrlClass.BK, UrlClass.RTB_AUCTION]),

    ('https://an.yandex.ru/meta_bulk/ololo', [UrlClass.YANDEX, UrlClass.BK, UrlClass.BK_META_BULK_AUCTION]),

    ('https://bs.yandex.ru/kakoi-to-path/file', [UrlClass.YANDEX, UrlClass.BK]),
    ('https://bs.yandex.ru/rtbcount/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.BK]),
    ('https://bs.yandex.ru/count/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.BK]),
    ('https://bs.yandex.ru/meta/ololo', [UrlClass.YANDEX, UrlClass.BK, UrlClass.RTB_AUCTION, UrlClass.BK_META_AUCTION]),
    ('https://yabs.yandex.ru/rtbcount/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.BK]),
    ('https://yabs.yandex.ru/count/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.YABS_COUNT, UrlClass.BK]),
    ('https://yabs.yandex.ru/meta/ololo', [UrlClass.YANDEX, UrlClass.BK, UrlClass.RTB_AUCTION, UrlClass.BK_META_AUCTION]),
    ('https://yabs.yandex.by/rtbcount/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.BK]),
    ('https://yabs.yandex.by/count/1GNOprpX0', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.YABS_COUNT, UrlClass.BK]),
    ('https://yabs.yandex.by/meta/ololo', [UrlClass.YANDEX, UrlClass.BK, UrlClass.RTB_AUCTION, UrlClass.BK_META_AUCTION]),
    ('https://ads.adfox.ru/123/getCode?trololo', [UrlClass.YANDEX, UrlClass.ADFOX, UrlClass.RTB_AUCTION]),
    ('https://ads.adfox.ru/123/event?trololo', [UrlClass.YANDEX, UrlClass.ADFOX, UrlClass.RTB_COUNT]),
    ('https://ads.adfox.ru/123/clickURL?trololo', [UrlClass.YANDEX, UrlClass.ADFOX, UrlClass.RTB_COUNT]),
    ('http://ads.adfox.ru/getid', [UrlClass.YANDEX, UrlClass.RTB_COUNT, UrlClass.ADFOX]),
    ('http://aab-pub.s3.yandex.net/lib.browser.min.js', [UrlClass.YANDEX, UrlClass.PCODE, UrlClass.STATICMON]),
    ('http://banners.adfox.ru/getid', [UrlClass.YANDEX]),
    ('https://ssp.rambler.ru/capirs_async.js', [UrlClass.RAMBLER]),
    ('https://ssp.rambler.ru/context.jsp?wl=rambler', [UrlClass.RAMBLER, UrlClass.RAMBLER_AUCTION]),
    ('http://img02.ssp.rambler.ru/file.jsp?url=KIqF1uMX62nWUR4tslIfkQZL', [UrlClass.RAMBLER]),
    ('http://test.local/wut', [UrlClass.PARTNER]),
    ('https://this-is-a-fake-address.com/damn', [UrlClass.PARTNER]),
    ('http://yandex.ru/pogoda/moscow', [UrlClass.DENY]),
    ('http://auto.ru/cars', [UrlClass.DENY]),
    ('http://yandex.ru/ads/system/context_adb.js', [UrlClass.YANDEX, UrlClass.BK, UrlClass.PCODE, UrlClass.PCODE_LOADER]),
    ('http://an.yandex.ru/system/context_adb.js', [UrlClass.YANDEX, UrlClass.BK, UrlClass.PCODE, UrlClass.PCODE_LOADER]),
    ('http://yastatic.net/partner-code/context.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('http://yastat.net/partner-code/context.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('http://yastatic.net/safeframe-bundles/0.69/1-1-0/render_adb.html', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://mc.yandex.ru/pixel/2555327861230035827?rnd=%aw_random%', [UrlClass.COUNTERS]),
    ('https://amc.yandex.ru/show?cmn_id=28982&plt_id=81939&crv_id=203057&evt_t=render&ad_type=banner&rnd=%aw_random%', [UrlClass.COUNTERS]),
    ('https://www.tns-counter.ru/V13a****bbdo_ad/ru/UTF-8/tmsec=bbdo_cid1021927-posid1323236/%aw_random% ', [UrlClass.COUNTERS]),
    ('https://1919501142.verify.yandex.ru/verify?platformid=1&msid=yndx_5-45391007-7905604478&BID=7905604478&BTYPE=2&CID=45391007&DRND=1919501142&DTYPE=desktop&'
     'REF=https%3A%2F%2Fyandex.ru%2Fpogoda%2Fotradniy%3Ffrom%3Dserp_title&SESSION=6615691574752125145&page=49688', [UrlClass.COUNTERS]),
    ('https://yastatic.net/images-islands/0x8dfaf36a16/pages-desktop/common/_common.ru.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://yastatic.net/images-islands/_/QkXy9zXB7opQD9pG9fxDS7ddeTM.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://yastatic.net/s3/fiji-static/_/Holn4Wm5JOs2hRezvlNUOogk85U.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://serp-static-testing.s3.yandex.net/fiji/static/fiji/freeze/QkXy9zXB7opQD9pG9fxDS7ddeTM.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://yastatic.net/q/crowdtest/serp-static-nanny/static/sandbox-fiji-4c40649ffa4f9/static/fiji/freeze/Q00VpjB_W_c.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://yastatic.net/yandex-video-player-iframe-api-bundles/56712/player_bundle.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://static.dzeninfra.ru/yandex-video-player-iframe-api-bundles/56712/player_bundle.js', [UrlClass.DZEN_PLAYER]),
    ('https://yastatic.net/awaps-ad-sdk-js/1_0/adsdk.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://yastatic.net/awaps-ad-sdk-js/2_1/adsdk.min.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://yastatic.net/awaps-ad-sdk-js/1_0/adsdk.min.js?_=0.41948363226744956', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://yastatic.net/awaps-ad-sdk-js-bundles/1.0-2508/bundles-es2017/multiroll.bundle.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://yastatic.net/awaps-ad-sdk-js-bundles/1.0-1118/adsdk.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://yastatic.net/vas-bundles/621059/bundles-es2017/inpage.bundle.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https:' + XHR_FIX_STYLE_SRC, [UrlClass.XHR_PROTECTION]),
    ('https:' + SCRIPT_FIX_STYLE_SRC.format(ARGS=""), [UrlClass.SCRIPT_PROTECTION]),
    ('https:' + SCRIPT_FIX_STYLE_SRC.format(ARGS="?style_nonce=1234567890"), [UrlClass.SCRIPT_PROTECTION]),
    ('https://yastatic.net/pcode-native-bundles/224/widget_adb.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('https://yastatic.net/pcode-native-bundles/224/loader_adb.js', [UrlClass.YANDEX, UrlClass.PCODE]),
    ('http://turbo.local/detect?domain=domain.com&path=script/script.js&proto=https', [UrlClass.AUTOREDIRECT_SCRIPT, UrlClass.PCODE]),
    ('http://turbo.local/detect?domain=domain.com', [UrlClass.DENY]),
    ('http://mobile.yandexadexchange.net:80/url', [UrlClass.YANDEX, UrlClass.NANPU]),
    ('http://adsdk.yandex.ru:80/url', [UrlClass.YANDEX, UrlClass.NANPU]),
    ('http://mobile-ads-beta.yandex.ru/url', [UrlClass.YANDEX, UrlClass.NANPU]),
    ('http://mobile.yandexadexchange.net:80/v1/startup', [UrlClass.YANDEX, UrlClass.NANPU, UrlClass.NANPU_STARTUP]),
    ('http://mobile.yandexadexchange.net:80/v4/ad', [UrlClass.YANDEX, UrlClass.NANPU, UrlClass.NANPU_AUCTION]),
    ('http://adsdk.yandex.ru:80/v1/startup', [UrlClass.YANDEX, UrlClass.NANPU, UrlClass.NANPU_STARTUP]),
    ('http://adsdk.yandex.ru:80/v4/ad', [UrlClass.YANDEX, UrlClass.NANPU, UrlClass.NANPU_AUCTION]),
    ('http://adsdk.yandex.ru:80/proto/report/1622/KYB=/71/an.yandex.ru/count/WP0ejI?est-tag=36', [UrlClass.YANDEX, UrlClass.NANPU, UrlClass.NANPU_COUNT]),
    ('https://an.yandex.ru/meta/ololo?nanpu-version=2', [UrlClass.YANDEX, UrlClass.NANPU, UrlClass.NANPU_AUCTION, UrlClass.NANPU_BK_AUCTION]),
    ('https://yandex.ru/ads/meta/ololo?nanpu-version=2', [UrlClass.YANDEX, UrlClass.NANPU, UrlClass.NANPU_AUCTION, UrlClass.NANPU_BK_AUCTION]),
    ('https://an.yandex.ru/adfox/123/getBulk/v2?trololo&nanpu-version=2', [UrlClass.YANDEX, UrlClass.NANPU, UrlClass.NANPU_AUCTION, UrlClass.NANPU_BK_AUCTION]),
    ('https://yandex.ru/ads/adfox/123/getBulk/v2?nanpu-version=2&trololo', [UrlClass.YANDEX, UrlClass.NANPU, UrlClass.NANPU_AUCTION, UrlClass.NANPU_BK_AUCTION]),
    ('https://strm.yandex.net/int/enc/srvr.xml', [UrlClass.YANDEX, UrlClass.STRM]),
])
def test_classify_url(url, expected_classes, get_config):
    config = get_config('test_local')
    url_classes = classify_url(config, url)
    assert sorted(url_classes) == sorted(expected_classes)


@pytest.mark.parametrize('url, partner_url_re, expected_classes', [
    ('https://mc.yandex.ru/metrika/watch.js', [r'mc\.yandex\.ru/(?:metrika|watch|webwisor)/.*?'], [UrlClass.PARTNER, UrlClass.YANDEX_METRIKA]),
    ('https://mc.yandex.ru/watch/watch.js', [r'mc\.yandex\.ru/(?:metrika|watch|webwisor)/.*?'], [UrlClass.PARTNER, UrlClass.YANDEX_METRIKA]),
    ('https://mc.yandex.by/watch/watch.js', [r'mc\.yandex\.by/(?:metrika|watch|webwisor)/.*?'], [UrlClass.PARTNER, UrlClass.YANDEX_METRIKA]),
    ('https://mc.yandex.com.tr/webwisor/watch.js', [r'mc\.yandex\.com\.tr/(?:metrika|watch|webwisor)/.*?'], [UrlClass.PARTNER, UrlClass.YANDEX_METRIKA]),
    ('https://yandex.ru/restricted/?dummy=https://mc.yandex.ru/metrika/watch.js', [r'mc\.yandex\.ru/(?:metrika|watch|webwisor)/.*?'], [UrlClass.DENY]),
    ('https://mc.yandex.ru/metrika/watch.js', ['test.local/.*?'], [UrlClass.DENY]),
])
def test_classify_yandex_metrika_url(url, partner_url_re, expected_classes, get_config):
    config = get_config('test_local')
    config.partner_url_re = re2.compile(re_completeurl(partner_url_re, True), re2.IGNORECASE)
    config.partner_url_re_match = re2.compile(re_completeurl(partner_url_re, True, match_only=True), re2.IGNORECASE)
    url_classes = classify_url(config, url)
    assert sorted(url_classes) == sorted(expected_classes)


@pytest.mark.parametrize('url, partner_url_re, expected_classes', [
    ('https://yandex.ru/somepath', [r'yandex\.ru/.*?'], [UrlClass.PARTNER]),
    ('https://yandex.ru/ads/meta/1234456', [r'yandex\.ru/.*?'], [UrlClass.YANDEX, UrlClass.BK, UrlClass.RTB_AUCTION, UrlClass.BK_META_AUCTION]),
    ('https://yandex.ru/ads/meta/1234456', ['test.local/.*?'], [UrlClass.YANDEX, UrlClass.BK, UrlClass.RTB_AUCTION, UrlClass.BK_META_AUCTION]),
])
def test_classify_partner_and_ads_urls(url, partner_url_re, expected_classes, get_config):
    config = get_config('test_local')
    config.partner_url_re = re2.compile(re_completeurl(partner_url_re, True), re2.IGNORECASE)
    config.partner_url_re_match = re2.compile(re_completeurl(partner_url_re, True, match_only=True), re2.IGNORECASE)
    url_classes = classify_url(config, url)
    assert sorted(url_classes) == sorted(expected_classes)
