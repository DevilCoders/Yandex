#!/skynet/python/bin/python
"""Calculate list of group to move to vla"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))


import re
from collections import defaultdict

import gencfg
from core.db import CURDB


LOCATION_PATTERNS = [
    'MSK_UGRB_(.*)',
    'MSK_IVA_(.*)',
    'MSK_FOL_(.*)',
    'MSK_MYT_(.*)',
    'MSK_(.*)',
    'SAS_(.*)',
    'MAN_(.*)',
    'VLA_(.*)',
]


def cut_specific(groupname, enhanced=False):
    for pattern in LOCATION_PATTERNS:
        m = re.match(pattern, groupname)
        if m:
            groupname = m.group(1)

    return groupname

if __name__ == '__main__':
    groups_by_pattern = defaultdict(list)
    counts_by_pattern = defaultdict(int)

    for group in CURDB.groups.get_groups():
        if group.card.properties.nonsearch:
            continue
        if group.card.properties.created_from_portovm_group is not None:
            continue
        if group.card.name.find('XPROD_') >= 0:
            continue
        if group.card.name.find('YABSCS_') >= 0:
            continue
        if group.card.name.find('YABS_') >= 0:
            continue
        if group.card.name.find('VH_') >= 0:
            continue
        if group.card.name.find('TEST_') >= 0:
            continue
        if group.card.name.find('RAZLADKI_') >= 0:
            continue
        if group.card.name.find('DEVTOOLS_') >= 0:
            continue
        if group.card.name.find('PSI1_PORTOVM') >= 0:
            continue
        if group.card.name.find('WEB_BASE_PRIEMKA_PORTOVM') >= 0:
            continue
        if group.card.name.find('BS_RTB_') >= 0:
            continue
        if group.card.name.find('AUTOBUDGET') >= 0:
            continue
        if group.card.name.find('BANNER_') >= 0:
            continue
        if group.card.name.find('YASTAT_') >= 0:
            continue
        if group.card.name.find('SPAMOOBORONA_') >= 0:
            continue
        if group.card.name.find('JUGGLER_CLIENT_PRESTABLE') >= 0:
            continue
        if group.card.name.find('RSYA_') >= 0:
            continue
        if group.card.name.find('SEARCHPRIEMKA_') >= 0:
            continue
        if group.card.name.find('SPAMOOBORONA_') >= 0:
            continue
        if group.card.name.find('ADV_MACHINE_') >= 0:
            continue
        if group.card.name.find('_CAAS') >= 0:
            continue
        if group.card.name.find('OCR') >= 0:
            continue
        if group.card.name.find('DIRECT_') >= 0:
            continue
        if group.card.name.find('SAMOGON_') >= 0:
            continue
        if group.card.tags.metaprj in ('personal', 'robot', 'yt', 'market', 'banner'):
            continue
        if cut_specific(group.card.name) in (
                'YABS_FRONTEND',  'VIDEO_SPIDER', 'ADV_MACHINE_RSYA', 'RSYA_WIZARD', 'YASTAT_BALANCER', 'SDMS_POOL', 'WEB_TUR_YALITE', 'WEB_RUS_YALITE', 'ADV_MACHINE_EXPERIMENT', 'WEB_REFRESH_10DAY_INT', 'WEB_REFRESH_10DAY_INT_COMTRBACKUP', 'IMGS_SLOVO_MASTER', 'IMAGES_REFRESH_DEV_MMETA', 'IMAGES_REFRESH_DEV_MMETA_BAN', 'IMGS_QUICK_BASE_HAMSTER', 'IMGS_INT_HAMSTER', 'IMGS_BASE_HAMSTER', 'IMGS_IMPROXY_PRESTABLE', 'CBRD_IMPROXY', 'IMGS_IMPROXY', 'IMGS_IMPROXY_BAN', 'IMGS_RIM_BALANCER', 'IMGS_RIM_BAN', 'SPORT_PUSH_MONGO', 'SPORT_PUSH_FRONT', 'SAAS_PDS_SEARCHPROXY', 'SAAS_PDS_INDEXERPROXY', 'SAAS_PDS_INDEXERPROXY_ADDITIONAL', 'SAAS_CLOUD_BASE_PDS', 'MISC_REPORT_RENDERER', 'MISC_AH_SRC_SETUP', 'MISC_APP_HOST', 'PDB_MONGODB_DATA_PRODUCTION', 'OORT_GROUP_OORTQLOUDEXTUNSTABLE2_PROXY', 'OORT_GROUP_OORTQLOUDEXTUNSTABLE3_PROXY', 'OORT_GROUP_OORTQLOUDEXTUNSTABLE1_PROXY', 'OORT_GROUP_OORTQLOUDEXTUNSTABLE1_CACHED', 'OORT_GROUP_OORTQLOUDEXTUNSTABLE4_PROXY', 'OORT_GROUP_OORTQLOUDEXTUNSTABLE4_CACHED', 'OORT_GROUP_OORTQLOUDEXTUNSTABLE2_CACHED', 'OORT_GROUP_OORTQLOUDEXTUNSTABLE3_CACHED', 'ZOOLOOSER_PASSPORT_TEST', 'PRODUCER_PASSPORT_PROD', 'PS_SAND_INTEGRATION_TESTING', 'CONSUMER_PASSPORT_TEST', 'PRODUCER_PASSPORT_TEST', 'CONSUMER_PASSPORT_PROD', 'MSEARCH_LOG_BROKER', 'HEARTBEAT_A_MONGODB', 'DIVERSITY_TK_JUPITER_BASE_NEW', 'WEB_MMETA_TIER1', 'ST_TASK_GENCFG_697_INT', 'ST_TASK_GENCFG_697_INT_HAMSTER', 'ST_TASK_GENCFG_697_BASE_HAMSTER', 'ST_TASK_GENCFG_697_BASE', 'ST_TASK_GENCFG_697_BASE_NIDX', 'WEB_INT_PRIEMKA_PORTOVM', 'NODES_FOR_CLUSTERAPI', 'YASM_PREP_MONGO',
                # approved by qwizzy
                'JUGGLER_SENTRY_MAIN',
                # approved by qwizzy
                'JUGGLER_PRODUCTION',
                'JUGGLER_PRODUCTION_ACTIVE',
                'JUGGLER_PRODUCTION_AGGRS',
                'JUGGLER_PRODUCTION_API',
                'JUGGLER_PRODUCTION_APIV2',
                'JUGGLER_PRODUCTION_API_BALANCER',
                'JUGGLER_PRODUCTION_BANSHEE',
                'JUGGLER_PRODUCTION_BANSHEE_TEMPLATE_LOADER',
                'JUGGLER_PRODUCTION_CACHER',
                'JUGGLER_PRODUCTION_CONFIG_CACHER',
                'JUGGLER_PRODUCTION_FRONTEND',
                'JUGGLER_PRODUCTION_LEADER',
                'JUGGLER_PRODUCTION_MFETCHER',
                'JUGGLER_PRODUCTION_MONGO',
                'JUGGLER_PRODUCTION_MYSQL',
                'JUGGLER_PRODUCTION_REDIS_CONFIG',
                'JUGGLER_PRODUCTION_REDIS_RAW',
                'JUGGLER_PRODUCTION_REDIS_RAW_SLAVE',
                'JUGGLER_PRODUCTION_TASK_RUNNER',
                'PRODUCTION_VOPILKA',
                'PRESTABLE_VOPILKA',
                'VOPILKA_PRODUCTION_ZOOKEEPER',
                # approved by amich
                'META_JOB_API_A',
                # approved by kfour
                'L7_BALANCER_R1',
                # approved by aleksart
                'SITESUGGEST_SUGGEST',
                # approved by sereglond
                'CSPLOG_DAEMON',
                'DRLOG_DAEMON',
                # approved by qwizzy
                'JUGGLER_PRESTABLE',
                'JUGGLER_PRESTABLE_ACTIVE',
                'JUGGLER_PRESTABLE_AGGRS',
                'JUGGLER_PRESTABLE_API',
                'JUGGLER_PRESTABLE_APIV2',
                'JUGGLER_PRESTABLE_API_BALANCER',
                'JUGGLER_PRESTABLE_BANSHEE',
                'JUGGLER_PRESTABLE_BANSHEE_TEMPLATE_LOADER',
                'JUGGLER_PRESTABLE_CACHER',
                'JUGGLER_PRESTABLE_CONFIG_CACHER',
                'JUGGLER_PRESTABLE_FRONTEND',
                'JUGGLER_PRESTABLE_KNOCKER',
                'JUGGLER_PRESTABLE_LEADER',
                'JUGGLER_PRESTABLE_MFETCHER',
                'JUGGLER_PRESTABLE_MONGO',
                'JUGGLER_PRESTABLE_MYSQL',
                'JUGGLER_PRESTABLE_PONGER',
                'JUGGLER_PRESTABLE_REDIS_CONFIG',
                'JUGGLER_PRESTABLE_REDIS_RAW',
                'JUGGLER_PRESTABLE_REDIS_RAW_SLAVE',
                'JUGGLER_PRESTABLE_TASK_RUNNER',
                # approved by sereglond and dmitryno
                'WIZARD_WIZARD',
                'SERVICE_WIZARD_WIZARD',
                # approved by sereglond
                'WIZARD_MARKET_MONEY',
                # group with another name in Vla
                'BANFLT_BASE_NEW',
                'QUERYSEARCH_MID_SSD_BASE_NEW',
                'DIVERSITY_JUPITER_BASE',
                'ONLY_L7PROD_BALANCER',
                'QUERYSEARCH_MID_SSD_INT_NEW',
                'QUERYSEARCH_MID_SSD_BASE_UPDATERD_NEW',
                'CLICK_CLICKDAEMON_NEW',
                # approved by noiseless
                'YASM_PRODUCTION_SRV',
                'YASM_PRODUCTION_REDIS',
                'YASM_PRODUCTION_HSRV',
                'YASM_PRODUCTION_DUMPER',
                # MISC_NEW group is fake: all slaves moved to dynamic
                'MISC_NEW',
                # approved by kfour
                'L7_BALANCER_R1',
                # approved by amich
                'META_JOB_API',
                'META_JOB_API_A',
                # approved by yaskevich
                'PDB_MONGODB_RESULT_BACKEND_PRODUCTION',
                'PDB_MONGODB_QUEUE_PRODUCTION',
                # approved by alonger
                'SG_CLUSTER_FAST',
                # approved by feliksas
                'SOCIAL_SUBSCRIPTIONS_READIS',
                'SOCIAL_READIS',
                # approved by temnajab
                'SUGGEST_SHARDING_BASE',
                'SUGGEST_SHARDING_MERGE',
                # approved by marvelstat
                'PROD_BASE_S1',
                # approved by kfour
                'L7_BALANCER_FAST',
                'L7_ANY_BALANCER_R1',
                'L7_BALANCER_ISS_R1',
                # approved by osol
                'MAIL_LUCENE',
                'DISK_LUCENE',
                # approved by amich
                'JOB_SERVICE_MDC1',
                # approved by osidorkin
                'SAAS_CLOUD_BASE_HAMSTER',
                # approved by librarian
                'WEB_MOBILE_REPORT_ATOM_SEARCH',
                'APPHOST_PERS_RERANKD',
                'APPHOST_PERS_RERANKD_SECOND_DATA',
                'ATOM_AH_SRC_SETUP',
                'ATOM_APP_HOST',
                'WEB_ATOM_FRONT',
                'WEB_ATOM_FRONT_SECOND',
                # new groups
                'EXTMAPS_PROMO_ADMIN_PRODUCTION',
                'SAAS_MAPS_SEARCHPROXY_DYNAMIC',
            ):
            continue


        groups_by_pattern[cut_specific(group.card.name)].append(group.card.name)
        counts_by_pattern[cut_specific(group.card.name)] += len(group.get_kinda_busy_hosts())

    counts_by_pattern = counts_by_pattern.items()
    counts_by_pattern.sort(key=lambda (x, y): y)

    for k, _ in counts_by_pattern:
        v = groups_by_pattern[k]
        if len([x for x in v if x.startswith('VLA_')]):
            continue
        if len(v) > 1:
            owners = ','.join(CURDB.groups.get_group(v[0]).card.owners)
            other_dc_groups = []
            for elem in v:
                other_dc_group = CURDB.groups.get_group(elem)
                if other_dc_group.card.master is None:
                    other_dc_groups.append('{}(None)'.format(other_dc_group.card.name))
                else:
                    other_dc_groups.append('{}({})'.format(other_dc_group.card.name, other_dc_group.card.master.card.name))

            print 'New group VLA_{} ({} slaves)(owners {}): current groups {}'.format(k, len(other_dc_group.card.slaves), owners, ' '.join(other_dc_groups))
