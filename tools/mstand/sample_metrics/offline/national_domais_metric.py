# coding=utf-8
import yaqutils.url_helpers as uurl

from metrics_api.offline import SerpMetricParamsForAPI  # noqa

# NOTE:
# This metric is a private property of Alexander selivanov@.
# It's NOT a part of MSTAND project.
# It should be moved to quality/mstand_metrics/users/...

NATIONAL_MARKER = 'NATIONAL'

regionByDomain = {
    'AD': ['ad'],
    'AE': ['ae'],
    'AF': ['af'],
    'AG': ['ag'],
    'AI': ['ai'],
    'AM': ['am'],
    'AO': ['ao'],
    'AR': ['ar'],
    'AS': ['as'],
    'AT': ['at'],
    'AU': ['au'],
    'AZ': ['az'],
    'BA': ['ba'],
    'BD': ['bd'],
    'BE': ['be'],
    'BF': ['bf'],
    'BG': ['bg'],
    'BH': ['bh'],
    'BI': ['bi'],
    'BJ': ['bj'],
    'BN': ['bn'],
    'BO': ['bo'],
    'BR': ['br'],
    'BS': ['bs'],
    'BW': ['bw'],
    'BY': ['by'],
    'BZ': ['bz'],
    'CA': ['ca'],
    'CD': ['cd'],
    'CF': ['cf'],
    'CG': ['cg'],
    'CH': ['ch'],
    'CI': ['ci'],
    'CK': ['ck'],
    'CL': ['cl'],
    'CM': ['cm'],
    'CO': ['co'],
    'CR': ['cr'],
    'CU': ['cu'],
    'CY': ['cy'],
    'CZ': ['cz'],
    'DE': ['de'],
    'DJ': ['dj'],
    'DK': ['dk'],
    'DM': ['dm'],
    'DO': ['do'],
    'DZ': ['dz'],
    'EC': ['ec'],
    'EE': ['ee'],
    'EG': ['eg'],
    'ES': ['es'],
    'ET': ['et'],
    'FJ': ['fj'],
    'FM': ['fm'],
    'FR': ['fr'],
    'GA': ['ga'],
    'GB': ['uk', 'gb'],
    'GE': ['ge'],
    'GG': ['gg'],
    'GH': ['gh'],
    'GI': ['gi'],
    'GL': ['gl'],
    'GM': ['gm'],
    'GP': ['gp'],
    'GR': ['gr'],
    'GT': ['gt'],
    'GY': ['gy'],
    'HK': ['hk'],
    'HN': ['hn'],
    'HR': ['hr'],
    'HT': ['ht'],
    'HU': ['hu'],
    'ID': ['id'],
    'IE': ['ie'],
    'IL': ['il'],
    'IM': ['im'],
    'IN': ['in'],
    'IQ': ['iq'],
    'IS': ['is'],
    'IT': ['it'],
    'JE': ['je'],
    'JM': ['jm'],
    'JO': ['jo'],
    'JP': ['jp'],
    'KE': ['ke'],
    'KG': ['kg'],
    'KH': ['kh'],
    'KR': ['kr'],
    'KW': ['kw'],
    'KZ': ['kz'],
    'LA': ['la'],
    'LB': ['lb'],
    'LI': ['li'],
    'LK': ['lk'],
    'LS': ['ls'],
    'LT': ['lt'],
    'LU': ['lu'],
    'LV': ['lv'],
    'LY': ['ly'],
    'MA': ['ma'],
    'MD': ['md'],
    'ME': ['me'],
    'MG': ['mg'],
    'MK': ['mk'],
    'ML': ['ml'],
    'MN': ['mn'],
    'MS': ['ms'],
    'MT': ['mt'],
    'MU': ['mu'],
    'MV': ['mv'],
    'MW': ['mw'],
    'MX': ['mx'],
    'MY': ['my'],
    'MZ': ['mz'],
    'NA': ['na'],
    'NE': ['ne'],
    'NF': ['nf'],
    'NG': ['ng'],
    'NI': ['ni'],
    'NL': ['nl'],
    'NO': ['no'],
    'NP': ['np'],
    'NR': ['nr'],
    'NU': ['nu'],
    'NZ': ['nz'],
    'OM': ['om'],
    'PA': ['pa'],
    'PE': ['pe'],
    'PH': ['ph'],
    'PK': ['pk'],
    'PL': ['pl'],
    'PN': ['pn'],
    'PR': ['pr'],
    'PS': ['ps'],
    'PT': ['pt'],
    'PY': ['py'],
    'QA': ['qa'],
    'RO': ['ro'],
    'RS': ['rs'],
    'RU': ['ru', u'рф'],
    'RW': ['rw'],
    'SA': ['sa'],
    'SB': ['sb'],
    'SC': ['sc'],
    'SE': ['se'],
    'SG': ['sg'],
    'SH': ['sh'],
    'SI': ['si'],
    'SK': ['sk'],
    'SL': ['sl'],
    'SM': ['sm'],
    'SN': ['sn'],
    'SO': ['so'],
    'ST': ['st'],
    'SV': ['sv'],
    'TD': ['td'],
    'TG': ['tg'],
    'TH': ['th'],
    'TJ': ['tj'],
    'TK': ['tk'],
    'TL': ['tl'],
    'TM': ['tm'],
    'TN': ['tn'],
    'TO': ['to'],
    'TR': ['tr'],
    'TT': ['tt'],
    'TW': ['tw'],
    'TZ': ['tz'],
    'UA': ['ua'],
    'UG': ['ug'],
    'US': ['com', 'us'],
    'UY': ['uy'],
    'UZ': ['uz'],
    'VC': ['vc'],
    'VE': ['ve'],
    'VG': ['vg'],
    'VI': ['vi'],
    'VN': ['vn'],
    'VU': ['vu'],
    'WS': ['ws'],
    'ZA': ['za'],
    'ZM': ['zm'],
    'ZW': ['zw'],
}


class NationalDomain:
    def __init__(self, depth=5):
        self.depth = depth

        all_domains = []
        for tld_list in regionByDomain.values():
            all_domains.extend(tld_list)
        self.all_domains = all_domains

    def get_national(self, hostname, query_country):
        if hostname is not None and query_country is not None:
            splitted = hostname.split('.')
            tld = splitted[-1].lower()
            if tld in self.all_domains:
                return NATIONAL_MARKER
        return 'UNKNOWN'

    def value(self, metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        rel_by_pos = {}
        for res in metric_params.results[:self.depth]:
            hostname = uurl.urlparse(res.url).hostname
            nat_mark = self.get_national(hostname, metric_params.query_country)
            rel_by_pos[res.pos] = nat_mark

        result = 0.
        for pos in sorted(rel_by_pos.keys()):
            if rel_by_pos[pos] == NATIONAL_MARKER:
                result += 1

        if result == 0.:
            if metric_params.query_country:
                return 0.
            else:
                return -1.
        else:
            return result / len(rel_by_pos)
