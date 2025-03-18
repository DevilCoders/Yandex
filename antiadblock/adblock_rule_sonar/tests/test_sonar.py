# encoding=utf8
from hamcrest import assert_that, equal_to, contains_inanyorder, has_item, all_of, is_not
from pytest_localserver import plugin
import pytest

from antiadblock.adblock_rule_sonar.sonar.lib.utils.parser import ParserNewRules, RuleHash, \
    get_domain_from_selector_value
from antiadblock.adblock_rule_sonar.sonar.lib.config import CONFIG, YANDEX_MORDA_REGEX, \
    YANDEX_MORDA_REGEX_WITHOUT_WWW, YANDEX_TLDS
from antiadblock.adblock_rule_sonar.sonar.lib.utils.sonar_yt import SonarYT, RuleInfo

httpserver = plugin.httpserver

TEST_BLOCK_LIST = """
auto.ru##.sales-list-ad
24auto.ru##div[class^="brrr_"]
2mm.ru,auto.ru,bigpicture.ru,ctc.ru,drive.ru,drive2.ru,echo.msk.ru,f1news.ru,finanz.ru,gastronom.ru,irr.ru,kakprosto.ru,km.ru,meduza.io,paperpaper.ru,probirka.org,rollingstone.ru,sobesednik.ru,sportbox.ru,supersadovnik.ru,svpressa.ru,tjournal.ru,vokrug.tv##div[id^="AdFox_banner"]
||auto.ru/get.php?args=
||smolensk-auto.ru/bb/view_mesto_js
auto.ru###some-div-id
@@/prepareCode?$xmlhttprequest,domain=auto.ru|coub.com
@@||avatars-fast.yandex.net/$image,domain=auto.ru
@@/getCodeTest?$script,domain=1tv.ru|2mm.ru|auto.ru|avito.ru|bigpicture.ru|chetv.ru|echo.msk.ru|finanz.ru|gastronom.ru|gubernia.com|kakprosto.ru|lenta.ru|mk.ru|paperpaper.ru|piter.tv|rambler.ru|ren.tv|rollingstone.ru|rusplt.ru|sobesednik.ru|sovsport.md|sovsport.ru|sports.ru|svpressa.ru|vokrugsveta.ru
@@/prepareCode?$script,domain=1tv.ru|2mm.ru|aif.ru|auto.ru|avito.ru|bigpicture.ru|chetv.ru|echo.msk.ru|finanz.ru|friday.ru|gastronom.ru|gubernia.com|km.ru|lenta.ru|mirtesen.ru|paperpaper.ru|piter.tv|rambler.ru|ren.tv|rollingstone.ru|sovsport.md|sovsport.ru|sports.ru|supersadovnik.ru|svpressa.ru|tass.ru|vokrugsveta.ru
/viewc.js$third-party,script,domain=agentnews.ru|alibabaru.com|alkodoma.ru|all-russ-fish.ru|auto-kmu.ru|avt-rost.ru|blogspot.ru|comics-portal.com|computera.info|csharpprogramming.ru|dendyportal.ru|diacr.ru|ecosibiri.ru|eset-key.ru|feniyx.ru|fishing-mr.ru|freezeet.ru|gameruns.ru|gamescontent.ru|gamesontorrent.ru|getmedic.ru|gorodenok.com|granitauto.ru|hemingway-lib.ru|hen-tai.ru|hentaichan.me|hipermir.ru|india-tv.net|kapec.tv|kasrentyg.ru|knighki.com|komischek.ru|kurshe.ru|la-greelyaj.ru|mainfun.ru|mercedesbenz124.ru|militaryexp.com|mp3party.net|nashidni.org|nofollow.ru|noviyserial.ru|ojimail.ru|onepdf.ru|ostfilm.org|ostfilm.tv|ostfilmtv.org|pazavto2012.ru|phortepiano.ru|photo-boutique.ru|popcat.ru|pressa.today|profiwins.com.ua|romanbook.net|rostovautosoundfest.ru|roundrobin.sytes.net|scrin.org|stud-time.ru|studon.ru|thevidosss.ru|timeallnews.ru|torrent-lynx.ru|torrent3d.ru|torrents-game.net|turkey-tv.net|tvuz.ru|vesnikasert.ru|viewapplegroup.ru|vozbuzhdaet.com|vsekratko.ru|windows-usb.ru|wsturbo.net|yoffy.ru|легион.net
/viewcdamn.js$second-party,script,domain=agentnews.ru|alibabaru.com|alkodoma.ru|all-russ-fish.ru|auto-kmu.ru|avt-rost.ru|blogspot.ru|comics-portal.com|computera.info|csharpprogramming.ru|dendyportal.ru|diacr.ru|ecosibiri.ru|eset-key.ru|feniyx.ru|fishing-mr.ru|freezeet.ru|gameruns.ru|gamescontent.ru|gamesontorrent.ru|getmedic.ru|gorodenok.com|auto.ru|hemingway-lib.ru|hen-tai.ru|hentaichan.me|hipermir.ru|india-tv.net|kapec.tv|kasrentyg.ru|knighki.com|komischek.ru|kurshe.ru|la-greelyaj.ru|mainfun.ru|mercedesbenz124.ru|militaryexp.com|mp3party.net|nashidni.org|nofollow.ru|noviyserial.ru|ojimail.ru|onepdf.ru|ostfilm.org|ostfilm.tv|ostfilmtv.org|pazavto2012.ru|phortepiano.ru|photo-boutique.ru|popcat.ru|pressa.today|profiwins.com.ua|romanbook.net|rostovautosoundfest.ru|roundrobin.sytes.net|scrin.org|stud-time.ru|studon.ru|thevidosss.ru|timeallnews.ru|torrent-lynx.ru|torrent3d.ru|torrents-game.net|turkey-tv.net|tvuz.ru|vesnikasert.ru|viewapplegroup.ru|vozbuzhdaet.com|vsekratko.ru|windows-usb.ru|wsturbo.net|yoffy.ru|легион.net
! auto.ru - anti-adblock ads
auto.ru$$script[tag-content="Ya.Context.AdvManager.render"][max-length="8000"]
! some rules for fb.ru
@@||xpicw.top/*.js$domain=czx.to
@@||xpicw.top/ajax/*$domain=czx.to
sdamgia.ru,baby.ru,vz.ru,avtovzglyad.ru,otzovik.com,e1.ru,yandex.by,yandex.com,yandex.com.tr,yandex.kz,yandex.ru,yandex.ua,dni.ru,kakprosto.ru$$script[wildcard="*AdvManager.render*createElement*s.src*"][max-length="3500"]
||an.yandex.ru/count/$popup
||auto.ru/assets/flat-ui/min/js/MG.Init.*.min.js$replace=/\\,l="_gac"/\\,l="__gac"/
||forums.overclockers.ru$replace=/(script.onerror = function\\(\\)\\{)/\\$1console.log('чииик');return;/,important
||static-mon.yandex.net/static/main.js
||http-check.yandex.ru
||anysite.ru$generichide
##.ad
~zen.yandex.ru,auto.ru,yandex.by,yandex.com,yandex.com.tr,yandex.fr,yandex.kz,yandex.ru,yandex.ua#%#AG_removeCookie('/blcrm|bltsr|^from\\$|^los\\$|HgGedof|mcBaGDt/');
auto.ru,yandex.ru,yandex.ua#%#//scriptlet("remove-cookie", "/JPIqApiY|BgeeyNoBJuyII|YvUVZgzQ|^adequate\\$|^telecommunications\\$|RMHqSzrMeR|yw_cmpr_prm|^computer|pcssspb|substantial$|HgGedof|mcBaGDt/")
||yandex.$cookie=/JPIqApiY|yw_cmpr_prm|computer|pcssspb|substantial|HgGedof|mcBaGDt/
auto.ru##+js(cookie-remover.js, /^blcrm|^bltsr|^cmtchd|^crookie|^kpunk/)
||naydex.net/.*$script
"""  # noqa: E501
EXPECTED_RULES = dict(
    partner_rules=[
        'auto.ru-hide-css-.sales-list-ad',
        'auto.ru-hide-css-div[id^="AdFox_banner"]',
        'auto.ru-hide-css-#some-div-id',
        'auto.ru-allow-url-pattern-/prepareCode?',
        'auto.ru-allow-url-pattern-||avatars-fast.yandex.net/',
        'auto.ru-allow-url-pattern-/getCodeTest?',
        'auto.ru-allow-url-pattern-/prepareCode?',
        'auto.ru-block-url-pattern-/viewcdamn.js',
        'auto.ru-hide-snippet-script[tag-content="Ya.Context.AdvManager.render"][max-length="8000"]',
        'e1.ru-hide-snippet-script[wildcard="*AdvManager.render*createElement*s.src*"][max-length="3500"]',
        '''auto.ru-hide-adguard-script-AG_removeCookie('/blcrm|bltsr|^from\\$|^los\\$|HgGedof|mcBaGDt/');''',
        'auto.ru-hide-adguard-script-//scriptlet("remove-cookie", '
        '"/JPIqApiY|BgeeyNoBJuyII|YvUVZgzQ|^adequate\\$|^telecommunications\\$|RMHqSzrMeR|yw_cmpr_prm|^computer|pcssspb|substantial$|HgGedof|mcBaGDt/")',
        'auto.ru-hide-snippet-+js(cookie-remover.js, /^blcrm|^bltsr|^cmtchd|^crookie|^kpunk/)',
    ],
    general_rules=[
        '-hide-css-.ad',
        '-block-url-pattern-||auto.ru/get.php?args=',
        '-block-url-pattern-||smolensk-auto.ru/bb/view_mesto_js',
        '-block-url-pattern-||an.yandex.ru/count/',
        '-block-url-pattern-||auto.ru/assets/flat-ui/min/js/MG.Init.*.min.js',
        '-block-url-pattern-||forums.overclockers.ru',
        '-block-url-pattern-||static-mon.yandex.net/static/main.js',
        '-block-url-pattern-||http-check.yandex.ru',
        '-block-url-pattern-||anysite.ru',
        '-block-url-pattern-||yandex.',
        '-block-url-pattern-||naydex.net/.*',
    ],
    yandex_related_rules=[
        '-block-url-pattern-||auto.ru/get.php?args=',
        '-block-url-pattern-||an.yandex.ru/count/',
        '-block-url-pattern-||auto.ru/assets/flat-ui/min/js/MG.Init.*.min.js',
        '-block-url-pattern-||static-mon.yandex.net/static/main.js',
        '-block-url-pattern-||http-check.yandex.ru',
        '-block-url-pattern-||yandex.',
        '-block-url-pattern-||naydex.net/.*',
    ],
    cookie_remove_rule=[
        '-block-url-pattern-||yandex.',
        '''auto.ru-hide-adguard-script-AG_removeCookie('/blcrm|bltsr|^from\\$|^los\\$|HgGedof|mcBaGDt/');''',
        'auto.ru-hide-snippet-+js(cookie-remover.js, /^blcrm|^bltsr|^cmtchd|^crookie|^kpunk/)',
        'auto.ru-hide-adguard-script-//scriptlet("remove-cookie", '
        '"/JPIqApiY|BgeeyNoBJuyII|YvUVZgzQ|^adequate\\$|^telecommunications\\$|RMHqSzrMeR|yw_cmpr_prm|^computer|pcssspb|substantial$|HgGedof|mcBaGDt/")',
    ]
)


def test_search_new_rules(httpserver):
    httpserver.serve_content(TEST_BLOCK_LIST)
    test_config = {
        'cluster': 'some_cluster',
        'token': 'some_token',
        'search_regexps': {
            'auto.ru': ['auto\\.ru'],
            'fb.ru': [],
            'e1.ru': ['e1\\.ru'],
        },
        'known_rules': {
        },
        'yandex_related_rules': CONFIG['yandex_related_rules'],
        'cookies_of_the_day': {
            'extended': 'crookie\ncomputer\nbltsr'
        },
    }
    syt = SonarYT(configuration=test_config, yt_client=Yt_test_client())
    rules_parser_from_lists = ParserNewRules(test_config, syt.known_rules,
                                             test_config['cookies_of_the_day']['extended'].splitlines(),
                                             {'Adguard': [httpserver.url]})
    new_rules = rules_parser_from_lists.parse_new_rules()

    assert_that(map(str, new_rules['partner_rules']), contains_inanyorder(*EXPECTED_RULES['partner_rules']))
    assert_that(map(str, new_rules['general_rules']), contains_inanyorder(*EXPECTED_RULES['general_rules']))
    assert_that(map(str, filter(lambda r: r.is_yandex_related, new_rules['general_rules'])),
                contains_inanyorder(*EXPECTED_RULES['yandex_related_rules']))
    assert_that(map(str, filter(lambda r: r.is_cookie_remove_rule,
                                new_rules['partner_rules'] + new_rules['general_rules'])),
                contains_inanyorder(*EXPECTED_RULES['cookie_remove_rule']))


def test_detect_obsolete_rules(httpserver):
    httpserver.serve_content(TEST_BLOCK_LIST)
    faked_block_list_url = httpserver.url
    test_config = {
        'cluster': 'some_cluster',
        'token': 'some_token',
        'search_regexps': {
            'auto.ru': ['auto\\.ru'],
            'fb.ru': [],
            'e1.ru': ['e1\\.ru'],
        },
        'yandex_related_rules': [],
        'cookies_of_the_day': {'extended': 'some_cookie_of_the_day'}
    }
    known_rules = {
        RuleHash('auto.ru-hide-css-.sales-list-ad', list_url=faked_block_list_url, options=''): False,
        # partner relevant rule
        RuleHash('auto.ru-hide-css-div[class^="some old obsolete rule"]', list_url=faked_block_list_url,
                 options=''): False,  # partner not relevant rule
        RuleHash('-block-url-pattern-||auto.ru/get.php?args=', list_url=faked_block_list_url, options=''): False,
        # general relevant rule
        RuleHash('-block-url-pattern-||obsolete.ru/get.php?args=', list_url=faked_block_list_url,
                 options=''): False,  # general not relevant rule
    }

    syt = SonarYT(configuration=test_config, yt_client=Yt_test_client())
    for rule_hash in known_rules:
        syt.known_rules[rule_hash].is_known_rule = known_rules[rule_hash]

    rules_parser_from_lists = ParserNewRules(test_config, syt.known_rules,
                                             test_config['cookies_of_the_day']['extended'],
                                             {'Ublock': [faked_block_list_url]})
    rules_parser_from_lists.parse_new_rules()

    for rule_hash in syt.known_rules:
        rule_found_in_list = rule_hash.short_rule in EXPECTED_RULES['partner_rules'] or rule_hash.short_rule in EXPECTED_RULES['general_rules']
        assert_that(syt.known_rules[rule_hash].is_known_rule, equal_to(rule_found_in_list))


class Yt_test_client:
    def __init__(self):
        self.config = dict(proxy=dict(url=''), token='')
        self.yt_read_table = [
            # delete yandex non related general
            {
                'short_rule': '-allow-url-pattern-/wp-content/plugins/anti-block/js/advertisement.js',
                'list_url': 'https://filters.adtidy.org/extension/firefox/filters/2.txt',
                'options': 'third-party: False',
                'raw_rule': '@@/wp-content/plugins/anti-block/js/advertisement.js$~third-party,domain=~gaytube.com'
                            '|~pornhub.com|~redtube.com|~redtube.com.br|~tube8.com|~tube8.es|~tube8.fr|~xtube.com|~'
                            'youjizz.com|~youporn.com|~youporngay.com',
                # for tests
                'is_parter_or_ya_related': False,
            },
            # delete yandex related general
            {
                'short_rule': '-allow-url-pattern-://yandex.ru/|',
                'list_url': 'https://easylist-downloads.adblockplus.org/ruadlist+easylist.txt',
                'options': 'generichide: True',
                'raw_rule': '@@://yandex.ru/|$generichide',
                # for tests
                'is_parter_or_ya_related': True,
            },
            # delete partner
            {
                'short_rule': 'autoru-allow-url-pattern-||avatars-fast.yandex.net/',
                'list_url': 'https://easylist-downloads.adblockplus.org/advblock+cssfixes.txt',
                'options': 'image: True',
                'raw_rule': '@@||avatars-fast.yandex.net/$image,domain=auto.ru',
                # for tests
                'is_parter_or_ya_related': True,
            },
            # not delete
            {
                'short_rule': '-allow-url-pattern-doubleclick.net/ads/preferences/',
                'list_url': 'https://filters.adtidy.org/extension/chromium/filters/2.txt',
                'options': '',
                'raw_rule': '@@doubleclick.net/ads/preferences/\r',
                # for tests
                'is_parter_or_ya_related': False,
            },
        ]

    def read_table(self, *args, **kwargs):
        return self.yt_read_table

    def TablePath(self, *args, **kwargs):
        pass

    def JsonFormat(self, *args, **kwargs):
        pass

    def insert_rows(self, *args, **kwargs):
        pass


SYT_CONFIG = {
    'cluster': 'some_cluster',
    'token': 'some_token',
}


def test_sonar_yt_get_obsolete_rules():
    yt_client = Yt_test_client()
    syt = SonarYT(SYT_CONFIG, yt_client)
    rule_hash_not_del = RuleHash(short_rule=yt_client.yt_read_table[-1]['short_rule'],
                                 list_url=yt_client.yt_read_table[-1]['list_url'],
                                 options=yt_client.yt_read_table[-1]['options'])
    syt.known_rules[rule_hash_not_del] = RuleInfo(is_known_rule=True, raw_rule='raw_rule')

    obsolete_rules = syt.get_obsolete_rules(CONFIG)
    # last is not delete rule
    for rule in yt_client.yt_read_table[:-1]:
        rule_hash = RuleHash(short_rule=rule['short_rule'],
                             list_url=rule['list_url'],
                             options=rule['options'])
        assert_that(obsolete_rules['general_rules'] + obsolete_rules['yandex_related_rules'], has_item(rule_hash))
    # check last rule
    assert_that(all_of(obsolete_rules['general_rules'], obsolete_rules['yandex_related_rules']), is_not(has_item(rule_hash_not_del)))


def test_sonar_yt_get_yandex_related_rules():
    yt_client = Yt_test_client()
    syt = SonarYT(SYT_CONFIG, yt_client)
    rule_hash_not_del = RuleHash(short_rule='-allow-url-pattern-doubleclick.net/ads/preferences/',
                                 list_url='https://filters.adtidy.org/extension/chromium/filters/2.txt',
                                 options='')
    syt.known_rules[rule_hash_not_del] = RuleInfo(is_known_rule=True, raw_rule='raw_rule')
    obsolete_rules = syt.get_obsolete_rules(CONFIG)
    for rule in yt_client.yt_read_table[:-1]:
        rule_hash = RuleHash(short_rule=rule['short_rule'],
                             list_url=rule['list_url'],
                             options=rule['options'])
        if rule['is_parter_or_ya_related']:
            assert_that(obsolete_rules['yandex_related_rules'], has_item(rule_hash))
            assert_that(obsolete_rules['general_rules'], is_not(has_item(rule_hash)))
        else:
            assert_that(obsolete_rules['yandex_related_rules'], is_not(has_item(rule_hash)))
            assert_that(obsolete_rules['general_rules'], has_item(rule_hash))


@pytest.mark.parametrize(
    'block_list, domains_block', (
        ('~yandex.ru,afisha.yandex.ru##.zen-lib__container div[class="direkt"]', ['yandex_afisha']),
        ('yandex.ru,~afisha.yandex.ru##.zen-lib__container div[class="direkt"]', ['yandex_morda', 'yandex_mail']),
        ('yandex.ru,yandex.by,~afisha.yandex.ru##.zen-lib__container div[class="direkt"]', ['yandex_morda', 'yandex_mail', 'yandex_afisha']),
        ('yandex.ru,~yandex.by##.zen-lib__container div[class="direkt"]', ['yandex_morda', 'yandex_mail', 'yandex_afisha']),
        ('shmyandex.ru,~yandex.ru##.zen-lib__container div[class="direkt"]', []),
        ('||kinopoisk.ru^$cookie=/determine|some_cookie_of_the_day\\$/', ['kinopoisk.ru']),
        ('||kinopoisk.*^$cookie=/determine|some_cookie_of_the_day\\$/', ['kinopoisk.ru']),
        ('lena-miro.ru,levik.blog,livejournal.*,olegmakarenko.ru,varlamov.*#?#+js(set-constant.js, Object.prototype.Begun, undefined)', ['livejournal']),
        ('lena-miro.ru,varlamov.*#?#+js(set-constant.js, Object.prototype.Begun, undefined)', ['livejournal']),
    ))
def test_sonar_included_and_excluded_domains_in_rules(httpserver, block_list, domains_block):
    httpserver.serve_content(block_list)
    test_config = {
        'cluster': 'some_cluster',
        'token': 'some_token',
        'search_regexps': {
            'yandex_morda': [YANDEX_MORDA_REGEX],
            'yandex_mail': [r'mail\.yandex\.{tld}'.format(tld=YANDEX_TLDS),
                            YANDEX_MORDA_REGEX_WITHOUT_WWW],
            'yandex_afisha': [r'afisha\.yandex\.{tld}'.format(tld=YANDEX_TLDS),
                              YANDEX_MORDA_REGEX_WITHOUT_WWW],
            'kinopoisk.ru': [r'(?:\w+\.)*kinopoisk\.(?:ru|\*|)'],
            'livejournal': [r'(?:[\w-]+\.)?livejournal\.(?:com|net|\*)', r'(?:[\w-]+\.)?varlamov\.(?:ru|me|\*)'],
        },
        'known_rules': {
        },
        'yandex_related_rules': CONFIG['yandex_related_rules'],
        'cookies_of_the_day': {'extended': 'some_cookie_of_the_day'}
    }
    syt = SonarYT(configuration=test_config, yt_client=Yt_test_client())
    rules_parser_from_lists = ParserNewRules(test_config, syt.known_rules,
                                             test_config['cookies_of_the_day']['extended'].splitlines(),
                                             {'Adguard': [httpserver.url]})
    new_rules = rules_parser_from_lists.parse_new_rules()

    partner_rules = [str(rule) for rule in new_rules['partner_rules']]
    list_of_domains = []
    for rule in partner_rules:
        for partner in test_config['search_regexps'].keys():
            if partner in rule:
                list_of_domains.append(partner)

    assert_that(list_of_domains, contains_inanyorder(*domains_block))


@pytest.mark.parametrize(
    'selector_value, domain', (
        ("||kinopoisk.ru^$cookie=/determine|introductory\\$/", "kinopoisk.ru"),
        ("||auto.ru^", "auto.ru"),
        ("||yandex.team.ru^", "yandex.team.ru"),
        ("||amazonaws.com/bucket-files-widget$third-party", "amazonaws.com"),
        ("||живи-по-кайфу.рф/^$popup", None),
        ("||ro/rtr/$script", None)
    ))
def test_get_domain_from_selector_value(selector_value, domain):
    assert_that(get_domain_from_selector_value(selector_value), equal_to(domain))
