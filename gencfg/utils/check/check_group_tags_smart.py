#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import re

import gencfg

from core.db import CURDB
from gaux.aux_colortext import red_text
from gaux.aux_decorators import gcdisable

@gcdisable
def main():
    succ = True
    groups = CURDB.groups.get_groups()

    SINGLE_GROUP_MATCHERS = [
        (
            "All R1 groups have ctype test",
            lambda x: x.card.name.endswith('_R1') and x.card.name not in ['ALL_WALLE_DAEMON_R1', 'ALL_WALLE_SERVER_R1',
                                                                          'ALL_TERRY_R1'],
            lambda x: x.card.tags.ctype == 'test'
        ),
        (
            "All priemka groups has ctype priemka",
            lambda x: x.card.name.endswith('_PRIEMKA') and x.card.name not in ['ALL_TERRY_PRIEMKA', 'ALL_REPORTRENDERER_MOBILE_PRIEMKA', 'MSK_SUP_RATELIMITER_PRIEMKA',
                                                                               'MAN_SUP_MQ_PRIEMKA'],
            lambda x: x.card.tags.ctype in ['priemka', 'priemkametrix'],
        ),
        #        (
        #            "All _MMETA groups has itype mmeta",
        #            lambda x: x.card.name.find('_MMETA') >= 0 and x.card.name not in ['MAN_WEB_INT_MMETA_EXPERIMENT', 'MAN_WEB_BASE_MMETA_EXPERIMENT', 'SAS_WEB_MMETA_TIER1', 'MSK_WEB_MMETA_TIER1', 'MAN_WEB_MMETA_TIER1', 'SAS_IMGS_MMETA_PROXY_EXPERIMENT', 'SAS_MISC_NEW_ARCNEWS_STORY_MMETA', 'MAN_MISC_NEW_BANFLT_MMETA', 'SAS_MISC_NEW_AUDIOMATCH_MMETA', 'SAS_NEWS_REALSEARCH_MMETA', 'MAN_MISC_NEW_LYRICS_MMETA_NEW', 'MSK_UGRB_NEWS_REALSEARCH_MMETA', 'MAN_MISC_NEW_AUDIOMATCH_MMETA', 'MAN_NEWS_REALSEARCH_MMETA', 'SAS_PERSONAL_HISTORY_MMETA', 'MAN_MISC_NEW_ARCNEWS_MMETA', 'MSK_MISC_NEW_ARCNEWS_MMETA', 'SAS_MISC_NEW_YACA_MMETA', 'SAS_MISC_NEW_MUSICMIC_MMETA', 'ALL_QUERYSEARCH_MMETA_PRIEMKA', 'MAN_MISC_NEW_YACA_MMETA', 'MAN_PERSONAL_HISTORY_MMETA', 'MSK_MISC_NEW_AUDIOMATCH_MMETA', 'MSK_QUERYSEARCH_MMETA', 'MSK_MISC_NEW_BANFLT_MMETA', 'MAN_NEWSP_REALSEARCH_MMETA', 'SAS_MISC_NEW_ARCNEWS_MMETA', 'MAN_MISC_NEW_MUSICMIC_MMETA', 'SAS_NEWSP_REALSEARCH_MMETA', 'MSK_MISC_NEW_YACA_MMETA', 'SAS_QUERYSEARCH_MMETA', 'MSK_MISC_NEW_MUSICMIC_MMETA', 'MSK_UGRB_NEWSP_REALSEARCH_MMETA', 'SAS_MISC_NEW_LYRICS_MMETA_NEW', 'SAS_MISC_NEW_BANFLT_MMETA', 'MAN_MISC_NEW_ARCNEWS_STORY_MMETA', 'MAN_QUERYSEARCH_MMETA', 'MSK_PERSONAL_HISTORY_MMETA', 'MSK_MISC_NEW_ARCNEWS_STORY_MMETA', 'MSK_MISC_NEW_LYRICS_MMETA_NEW'] and (x.card.master is None or x.card.master.card.name != 'ALL_NANNY_DYNAMIC') and x.card.name.find('_NIDX') < 0,
        #            lambda x: x.card.tags.itype in ['mmeta', 'addrsmiddle', 'searchproxy', 'intl2', 'shardtool'],
        #        ),
        (
            "All _NMETA groups has itype upper",
            lambda x: x.card.name.find('_NMETA') >= 0 and x.card.name not in ['ALL_NMETA_DATAGENERATOR',
                                                                              'MSK_WEB_NMETA_TANK_DEGRADATION',
                                                                              'MSK_WEB_NMETA_TANK_EXPERIMENT',
                                                                              'MSK_WEB_NMETA_TANK_NOAPACHE',
                                                                              'MSK_WEB_NMETA_TARGET_BALANCER',
                                                                              'MSK_WEB_NMETA_TARGET_NOAPACHE',
                                                                              'MSK_WEB_NMETA_TANK_BALANCER',
                                                                              'MSK_WEB_NMETA_TARGET_EXPERIMENT',
                                                                              'MSK_WEB_NMETA_TARGET_DEGRADATION',
                                                                              'MSK_WEB_NMETA_TARGET_TDI_MERGER',
                                                                              'MSK_WEB_NMETA_TANK_TDI_MERGER'],
            lambda x: x.card.tags.itype in ['upper', 'addrsupper', 'newsupper', 'audioupper'],
        ),
        #(
        #    "All _DATAUPDATER groups has itype dataupdaterd",
        #    lambda x: x.card.name.find('_DATAUPDATER') >= 0,
        #    lambda x: x.card.tags.itype == 'dataupdaterd',
        #),
        #        (
        #            "All _NOAPACHE groups has itype noapache",
        #            lambda x: x.card.name.endswith('_NOAPACHE') and not x.card.name.endswith('_PRIEMKA_NOAPACHE'),
        #            lambda x: x.card.tags.itype == 'noapache',
        #        ),
        (
            "All _INT group has itype int",
            lambda x: (x.card.name.find('_INT_') >= 0 or x.card.name.endswith('_INT')) and x.card.name not in [
                'MSK_NOTIFICATIONS_INT_BALANCER', 'AMS_NOTIFICATIONS_INT_BALANCER', 'MAN_WEB_INT_PRIEMKA_PORTOVM',
                'MAN_WEB_INT_PRIEMKA_PORTOVM_GUEST', 'SAS_WEB_INT_PRIEMKA_PORTOVM',
                'SAS_WEB_INT_PRIEMKA_PORTOVM_GUEST'],
            lambda x: x.card.tags.itype in ['int', 'newsd', 'intsaas'],
        ),
        #(
        #    "All _THUMB group has itype thumb",
        #    lambda x: x.card.name.endswith('_THUMB'),
        #    lambda x: x.card.tags.itype == 'thumb',
        #),
        #(
        #    "All MSK_ groups contain only msk hosts",
        #    lambda x: x.card.name.startswith('MSK_') and x.card.master is None and x.card.name not in [
        #        'MSK_OXYGEN_BASE', 'MSK_WEB_YLITE', 'MSK_WEB_BASE_R1', 'MSK_FUSION_MMETA_10DAY_RTI_EXPERIMENT',
        #        'MSK_QUERYDATASUPPORT', 'MSK_FUSION_BASE_3DAY_UKROP_EXPERIMENT',
        #        'MSK_FUSION_BASE_10DAY_UKROP_EXPERIMENT', 'MSK_FUSION_MMETA_10DAY_UKROP_EXPERIMENT',
        #        'MSK_WEBSUGGEST_SUGGEST', 'MSK_IMGS_NMETA_PRIEMKA_RUS_LATEST', 'MSK_WEB_NMETA_PRIEMKA_RUS_LATEST',
        #        'MSK_WEB_NMETA_PRIEMKA_RUS_UPPER', 'MSK_IMGS_NMETA_PRIEMKA_RUS_UPPER', 'MSK_IMGS_IMPROXY_R1',
        #        'MSK_WEB_BASE_PRIEMKA', 'MSK_SDMS_POOL'],
        #    lambda x: len(x.getHosts()) == 0 or (list(set(map(lambda x: x.location, x.getHosts()))) == ['msk']),
        #),
        #(
        #    "All SAS_ groups contain only sas hosts",
        #    lambda x: x.card.name.startswith('SAS_') and x.card.master is None and x.card.name not in [
        #        'SAS_SAAS_CLOUD_BASE_HAMSTER', 'SAS_YASM_YASMAGENT_PRESTABLE'],
        #    lambda x: len(x.getHosts()) == 0 or (list(set(map(lambda x: x.location, x.getHosts()))) == ['sas']),
        #),
        #(
        #    "All MAN_ groups contain only man hosts",
        #    lambda x: x.card.name.startswith('MAN_') and x.card.master is None and x.card.name not in ['MAN_VIDEO_SPIDER'],
        #    lambda x: len(x.getHosts()) == 0 or (list(set(map(lambda x: x.location, x.getHosts()))) == ['man']),
        #),
        #        (
        #            "All _IMGS_ groups has at least one imgs project in project tags",
        #            lambda x: x.card.name.find('_IMGS_') > 0 and x.card.name != 'ALL_IMGS_C1_ALIAS',
        #            lambda x: len(filter(lambda y: y.startswith('imgs'), x.card.tags.prj)) > 0,
        #        ),
        #(
        #    "All FUSION BASE groups has itype fusion",
        #    lambda x: re.match(".*FUSION_.*BASE", x.card.name) is not None,
        #    lambda x: x.card.tags.itype == 'fusion',
        #),
        (
            "Group ALL_SEARCH does not have non-virtual hosts",
            lambda x: x.card.name == "ALL_SEARCH",
            lambda x: len(filter(lambda host: host.is_vm_guest(), x.getHosts())) == 0,
        ),
        (
            "All HAMSTER groups has ctype hamster",
            lambda x: re.match(".*_HAMSTER.*",
                               x.card.name) is not None and x.card.name != "MSK_WIZARD_TDI_WIZARD_HAMSTER" and (
                      x.card.master is None or x.card.master.card.name != 'ALL_NANNY_DYNAMIC'),
            lambda x: x.card.tags.ctype == "hamster",
        ),
        #(
        #    "All upper hamster in Sas on machines with ipv4",
        #    lambda x: x.card.tags.ctype == "hamster" and x.card.tags.itype == "upper" and x.card.name.startswith(
        #        "SAS_"),
        #    lambda x: len(filter(lambda x: x.ipv4addr == 'unknown', x.getHosts())) == 0,
        #),
        (
            "Groups with shardid tag have at least one intlookup",
            lambda x: x.card.searcherlookup_postactions.shardid_tag.enabled == True,
            lambda x: len(x.card.intlookups) > 0 or x.card.searcherlookup_postactions.custom_tier.enabled == True,
        ),
        (
            "Groups have at least one owner",
            lambda x: x.card.master is None or x.card.master.card.name != 'ALL_NANNY_DYNAMIC',
            lambda x: len(x.card.owners) > 0,
        ),
        (
            "Jupiter group have shardid/copy_on_ssd tags",
            lambda x: (x.card.tags.itype not in ('int', 'intl2')) and len(x.card.intlookups) > 0 and (CURDB.intlookups.get_intlookup(x.card.intlookups[0]).tiers is not None) and len(set(CURDB.intlookups.get_intlookup(x.card.intlookups[0]).tiers) & set(["WebTier0", "WebTier1", "PlatinumTier0", "Div0", "DivTr0", "RusMapsTier0", "TurMapsTier0"])) > 0,
            lambda x: (x.card.searcherlookup_postactions.shardid_tag.enabled == True) and (x.card.searcherlookup_postactions.copy_on_ssd_tag.enabled == True),
        ),
    ]

    for descr, flt, checker in SINGLE_GROUP_MATCHERS:
        filtered_groups = filter(lambda x: flt(x), groups)
        failcheck_groups = filter(lambda x: not checker(x), filtered_groups)
        if len(failcheck_groups) > 0:
            print red_text(
                "Failed check <%s> on groups: %s" % (descr, ','.join(map(lambda x: x.card.name, failcheck_groups))))
            succ = False

    DUAL_GROUP_MATCHERS = [
        (
            "Slave int groups have same tags as base groups",
            r"(.*)_BASE(.*)",
            r"\1_INT\2",
            lambda src, dst: dst.card.name == 'SAS_WEB_FUSION_INT_COMTRBACKUP' or (src.card.tags.ctype == dst.card.tags.ctype and src.card.tags.prj == dst.card.tags.prj),
        ),
        (
            "R1 groups have same prj and itype groups as non-R1",
            r"(.*)",
            r"\1_R1",
            lambda src, dst: (dst.card.name in ['SAS_L7_BALANCER_R1', 'MSK_L7_BALANCER_R1',
                                                'MSK_WEB_MOBILE_RERANKD_R1']) or (
                             src.card.tags.itype == dst.card.tags.itype and src.card.tags.prj == dst.card.tags.prj),
        ),
        (
            "R1 groups have same prj and itype groups as non-R1",
            r"(.*?)_(.*)",
            r"ALL_\2_R1",
            lambda src, dst: (dst.card.name in ['ALL_ADDRS_BUSINESS_R1']) or (src.card.tags.itype == dst.card.tags.itype and src.card.tags.prj == dst.card.tags.prj),
        ),
        (
            "Priemka groups have same prj and itype groups as non-priemka",
            r"(.*)",
            r"\1_PRIEMKA",
            lambda src, dst: (src.card.name in ['MAN_MUSICMIC_INT', 'MAN_MUSICMIC_BASE']) or (src.card.tags.itype == dst.card.tags.itype and src.card.tags.prj == dst.card.tags.prj),
        ),
        (
            "Priemka groups have same prj and itype groups as non-priemka",
            r"(.*?)_(.*)",
            r"ALL_\2_PRIEMKA",
            lambda src, dst:  (src.card.name in ['MAN_MUSICMIC_INT', 'MAN_MUSICMIC_BASE']) or (src.card.tags.itype == dst.card.tags.itype and src.card.tags.prj == dst.card.tags.prj),
        ),
        (
            "Msk and Sas have same prj and itype",
            r"MSK_(.*)",
            r"SAS_\1",
            lambda src, dst:
                (src.card.name in ['MSK_WEB_BASE_BACKUP', 'MSK_MISC_NMETA', 'MSK_SAAS_REFRESH_DEV_MMETA_VIDEO', 'MSK_BKHT_JUNK']) or
                (src.card.master is not None and src.card.master.card.name == 'MSK_MENACE_BALANCER') or
                (src.card.master is not None and src.card.master.card.name == 'MSK_MISC_NMETA') or
                (src.card.master is not None and src.card.master.card.name == 'MSK_WEB_NMETA_COMTRBACKUP') or
                (src.card.tags.itype == dst.card.tags.itype and set(src.card.tags.prj) == set(dst.card.tags.prj)),
        ),
        (
            "Msk and Man have same prj and itype",
            r"MSK_(.*)",
            r"MAN_\1",
            lambda src, dst:
                (src.card.name in ['MSK_PDB_STATIC_PRODUCTION', 'MSK_PDB_BACKEND_TEST', 'MSK_PSI1_PORTOVM', 'MSK_BKHT_JUNK', 'MSK_MUSICMIC_BASE', 'MSK_MISC_NEW_MUSICMIC_MMETA', 'MSK_MUSICMIC_INT']) or
                (src.card.tags.itype == dst.card.tags.itype and set(src.card.tags.prj) == set(dst.card.tags.prj)),
        ),
        #        (
        #            "Msk and Sas have same owners",
        #            r"MSK_(.*)",
        #            r"SAS_\1",
        #            lambda src, dst: set(src.card.owners) == set(dst.card.owners),
        #        ),
        #        (
        #            "Msk and Man have same owners",
        #            r"MSK_(.*)",
        #            r"MAN_\1",
        #            lambda src, dst: set(src.card.owners) == set(dst.card.owners),
        #        ),
        #        (
        #            "_NEW group has exactly same tags as old one",
        #            r"(.*)",
        #            r"\1_NEW",
        #            lambda src, dst: src.card.name in ['ALL_YASM_PRODUCTION_NGINX', 'MSK_VIDEO_QUICK_BASE', 'SAS_VIDEO_QUICK_BASE', 'MAN_VIDEO_QUICK_BASE', 'SAS_SAAS_CLOUD_LAAS_BASE', 'MSK_SAAS_CLOUD_LAAS_BASE'] or ((src.card.tags.itype == dst.card.tags.itype) and (src.card.tags.itype == dst.card.tags.itype) and
        #                             (set(src.card.tags.prj) == set(dst.card.tags.prj)) and (src.card.tags.metaprj == dst.card.tags.metaprj)),
        #        ),
        (
            "Groups ALL_ADDRS_LOAD_STAND1 and ALL_ADDRS_LOAD_STAND2 contain same machines",
            r"ALL_ADDRS_LOAD_STAND1",
            r"ALL_ADDRS_LOAD_STAND2",
            lambda src, dst: set(map(lambda x: x.model, src.getHosts())) == set(map(lambda x: x.model, dst.getHosts())),
        ),
        (
            "Groups ALL_ADDRS_LOAD_STAND1 and ALL_ADDRS_LOAD_STAND3 contain same machines",
            r"ALL_ADDRS_LOAD_STAND1",
            r"ALL_ADDRS_LOAD_STAND3",
            lambda src, dst: set(map(lambda x: x.model, src.getHosts())) == set(map(lambda x: x.model, dst.getHosts())),
        ),
    ]

    for descr, pattern, replacement, checker in DUAL_GROUP_MATCHERS:
        failed_groups = []
        for srcgroup in groups:
            if re.match(pattern, srcgroup.card.name) > 0:
                dstname = re.sub(pattern, replacement, srcgroup.card.name)
                if CURDB.groups.has_group(dstname):
                    dstgroup = CURDB.groups.get_group(dstname)
                    if not checker(srcgroup, dstgroup):
                        failed_groups.append((srcgroup.card.name, dstgroup.card.name))
        if len(failed_groups) > 0:
            print red_text("Failed check <%s> on groups: %s" % (descr, ','.join(map(lambda (x, y): "%s(%s)" % (x, y), failed_groups))))
            succ = False

    return int(succ)


if __name__ == '__main__':
    result = main()
    if result:
        sys.exit(0)
    else:
        sys.exit(1)
