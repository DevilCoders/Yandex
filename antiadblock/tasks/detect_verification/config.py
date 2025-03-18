# -*- coding: utf8 -*-
from json import dumps

import antiadblock.tasks.tools.common_configs as configs

UIDS_STATE_TABLE_PATH = '//home/antiadb/uids_adb_state/'
SOLOMON_API_URL = configs.SOLOMON_PUSH_API + 'project=Antiadblock&service=users_validation&cluster={cluster}'
SOLOMON_TS_FMT = configs.SOLOMON_TS_H_FMT

ADB_EXTS = [
    {'id': 'bieimkcgkepinadnphjpljpcfbdipofn', 'name': 'Adguard (yandex browser default version)'},
    # was found in top browser logs:
    {'id': 'oidhhegpmlfpoeialbgcdocjalghfpkp', 'name': 'Adblock Plus', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'lgblnfidahcdcjddiepkckcfdhpknnjh', 'name': 'Fair AdBlocker', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'njbiblblejhdhmoohgmcdiknodanpagp', 'name': 'Rule AdBlocker', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'bbmegnmpleoagolcnjnejdacakedpcgd', 'name': 'IObit Surfing Protection & Ads Removal', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'moefjfjeieehgdpklgbmbeihffhhaeek', 'name': 'ContentBlockHelper', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'pgbllmbdjgcalkoimdfcpknbjgnhjclg', 'name': 'Ads Killer Adblocker Plus', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'lalfpjdbhpmnhfofkckdpkljeilmogfl', 'name': 'Hola ad blocker', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'lelnemepcfnebljimmaaabbdmogjhnpm', 'name': 'AdBlocker Lite', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'oofnbdifeelbaidfgpikinijekkjcicg', 'name': 'uBlock Plus Adblocker', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'pkhklkokeogkelnbmfcldbplbhppmehl', 'name': 'MaiNews блокировка рекламы', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'mblkjekmapmageecfpaicafhiiddgana', 'name': 'AdvProfit', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'kacljcbejojnapnmiifgckbafkojcncf', 'name': 'Ad-Blocker', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'adkfgdipgpojicddmeecncgapbomhjjl', 'name': 'uBlocker - #1 Adblock Tool for Chrome', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'dgfopblddkpojhmniojidbinpkfmhfnf', 'name': 'AdBlock Google chrome Extension'},
    {'id': 'mmpljmegdkddjkklnheaaldbcblcpjpb', 'name': 'Stop Ads - Full Ad Blocking'},
    {'id': 'oaoakaajdpchhnjcnebaopippiibidgj', 'name': 'AdShield'},
    {'id': 'pnhflmgomffaphmnbcogleagmloijbkd', 'name': 'uBlock Adblocker Plus 2.3.3 CRX for Chrome or Chromium'},
    # taken from https://bitbucket.browser.yandex-team.ru/projects/STARDUST/repos/browser-server-config/browse/common/ad_blocker.json
    {'id': 'ocifcklkibdehekfnmflempfgjhbedch', 'name': 'Adblock Pro'},
    {'id': 'kccohkcpppjjkkjppopfnflnebibpida', 'name': 'uBlock Origin', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'cgdogbijachehheddakopmfjahhgmmma', 'name': 'AdBlocker for YouTube', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'cfhdojbkjhnklbpkdaibdccddilifddb', 'name': 'Adblock Plus - free ad blocker', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'alcmakjhknigccfidaelkafjmfifkhkc', 'name': 'WAB - Wise Ads Block', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'ohahllgiabjaoigichmmfljhkcfikeof', 'name': 'AdBlocker Ultimate', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'jjckigopagkhaikodedjnmbccfpnmiea', 'name': 'Ads Killer', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'fenfpfipldcdebkpiboonlpnoklcnepg', 'name': 'Ad-Blocker Pro', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'klhobddcbiabdfjmomildokiglpmdicc', 'name': 'Adblock Fast', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'dklmdhmkdbinnceekhecifmjhiiabolp', 'name': 'AdNauseam', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'pmpmnoinbkdojlnknogfeoagmhmhgakc', 'name': 'AdBlocker Ultimate', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'cjpalhdlnbpafiamejdnhcphjbkeiagm', 'name': 'uBlock Origin', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'mlomiejdfkolichcflejclcbmpeaniij', 'name': 'Ghostery - Privacy Ad Blocker', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'jacihiikpacjaggdldhcdfjpbibbfjmh', 'name': 'Adblocker Genesis Plus', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'bgnkhhnnamicmpeenaelnjfhikgbkllg', 'name': 'AdGuard AdBlocker', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'gighmmpiobklfepjocnamgkkbiglidom', 'name': 'AdBlock', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'bdnbdohkifapaghnepdmhobgkgfhkcga', 'name': 'AdStop', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'cbfogcnapljlnkmglcjilephioechgak', 'name': 'Adblocker.Global', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'fcnodchihpbcjiofidkkeaioejgpijkl', 'name': 'Stop Reclame', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'bbkekonodcdmedgffkkbgmnnekbainbg', 'name': 'Ghostery', 'store': 'https://addons.opera.com/en/search/?query='},
    {'id': 'epcnnfbjfcgphgdmggkamkmgojdagdnn', 'name': 'uBlock', 'store': 'https://chrome.google.com/webstore/search/'},
    # taken from top-20 from Crhome store
    {'id': 'dgpfeomibahlpbobpnjpcobpechebadh', 'name': 'AdBlock', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'lmiknjkanfacinilblfjegkpajpcpjce', 'name': 'AdBlocker', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'npmleadjnlojpinmkhnepddhlplealpg', 'name': 'Adblocker HARDLINE V2', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'cjpnaccogegoaigcmccebcknlgnealbi', 'name': 'Adblocker DEFENSE', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'jkihkechmikghlhilgdnjofibnbnpmcj', 'name': 'Adblocker CRM', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'gbgkoodppmcmfeaegpelbngiahdcccig', 'name': 'uBlocker', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'dgbldpiollgaehnlegmfhioconikkjjh', 'name': 'AdBlocker by Trustnav', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'bcidahcpjehlemnbmeaepadalhjofnmp', 'name': '7 Times Faster', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'gnpkpmplmkcebdadgkeiccpfgldggnoo', 'name': 'Adblock for WebSites', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'efajcfmdhbaeelgmjefpnmelhccfinnl', 'name': 'Adblock Inverted', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'emdofimlgdflefgpcohmmffijijgoooc', 'name': 'Funny Adblock', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'pkocadmokmpjeeaimigjpmfpdaighkga', 'name': 'The 1 Adblocker', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'nldifmjnaeokhdgcmlmfnhcnalapdjbg', 'name': 'Aon Adblock Mini', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'nneejgmhfoecfeoffakdnolopppbbkfi', 'name': 'Adblock Fast', 'store': 'https://chrome.google.com/webstore/search/'},
    {'id': 'jdiegbdfmhkofahlnojgddehhelfmadj', 'name': 'Adblocker Genius PRO', 'store': 'https://chrome.google.com/webstore/search/'},
]

ADB_EXTS_PLACEHOLDER = dumps([e['id'] for e in ADB_EXTS]).strip('[]')
