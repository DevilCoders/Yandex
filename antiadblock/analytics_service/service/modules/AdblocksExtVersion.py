# encoding=utf8
from _BaseModule import _BaseModule
from tools.github_releases import get_info_from_github

ADBLOCKS_URLS = {
    "Adblock":      'https://api.github.com/repos/betafish-inc/adblock-releases/tags',
    # "AdblockPlus":  'https://api.github.com/repos/adblockplus/adblockplusui/releases',  # почему-то не отдает релизы
    "Adguard":      'https://api.github.com/repos/AdguardTeam/AdguardBrowserExtension/releases',
    "Ghostery":     'https://api.github.com/repos/ghostery/ghostery-extension/releases',
    "Ublock":       'https://api.github.com/repos/uBlock-LLC/uBlock/releases',
    "UblockOrigin": 'https://api.github.com/repos/gorhill/uBlock/releases',
}

# Ключи проверки, чтоб знать какие результаты выгружать из YT и проверять что два разных метода не используют один ключ
CHECK_KEYS = map(lambda x: x + '_releases', ADBLOCKS_URLS.keys())


class AdblocksExtVersion(_BaseModule):
    def __init__(self, logger):
        super(AdblocksExtVersion, self).__init__(logger)

    def get_check_result(self):
        results = []
        for key, url in ADBLOCKS_URLS.items():
            self.logger.info('Get {} releases from {}'.format(key, url))

            results.append((key + '_releases', get_info_from_github(url, True)))
        return results

    def filter_results(self, current_results, previous_results):
        return [curr_result for curr_result in current_results if (curr_result[0], curr_result[1]['version']) not in [(result['check_name'], result['version']) for result in previous_results]]
