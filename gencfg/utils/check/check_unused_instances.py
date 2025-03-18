#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from gaux.aux_decorators import gcdisable
import logging

@gcdisable
def main():
    failed = 0
    for group in CURDB.groups.get_groups():
        if group.card.properties.get('uneven_instances', False):
            continue
        if group.card.tags.itype in ['fusion', 'base', 'int', 'thumb', 'newsd', 'realsearch', 'shardtool', 'intl2']:
            continue
        if group.card.name in ['ALL_TSNET_BASE', 'MSK_IMGMISC_MMETA_PRIEMKA', 'MSK_OXYDEV_EXPERIMENT',
                               'MSK_BANFLT_BASE_PRIEMKA', 'MSK_WEB_FUSION_3DAY_BASE', 'MSK_ADDRSNIPS_BASE',
                               'MSK_BANFLT_BASE', 'MSK_RTQUERYSEARCH_BASE', 'SAS_ADDRSNIPS_BASE', 'SAS_BANFLT_BASE',
                               'SAS_RTQUERYSEARCH_BASE', 'MSK_ADDRSNIPS_BASE_R1', 'MSK_BANFLT_BASE_R1',
                               'MSK_RTQUERYSEARCH_BASE_R1', 'MAN_BANFLT_BASE', 'SAS_CLICK_CLICKDAEMON_NEW',
                               'MAN_QUERYSEARCH_MID_SSD_BASE', 'MSK_QUERYSEARCH_MID_SSD_BASE',
                               'SAS_QUERYSEARCH_MID_SSD_BASE', 'MAN_WEB_BASE_PRIEMKA_PORTOVM_GUEST',
                               'SAS_WEB_BASE_PRIEMKA_PORTOVM_GUEST', 'MAN_OXYGEN_CALLISTO', 'MAN_OXYGEN_IO_BASE',
                               'SAS_WEB_BASE_PRIEMKA_PORTOVM', 'MAN_WEB_BASE_PRIEMKA_PORTOVM', 'MAN_QUERYSEARCH_BASE',
                               'MSK_MUSICMIC_BASE', 'SAS_ARCNEWS_BASE', 'MAN_ARCNEWS_BASE', 'MSK_ARCNEWS_BASE_R1',
                               'MAN_DIVERSITY2_BASE_PRIEMKA_PORTOVM', 'MSK_PERSONAL_HISTORY_SSD_BASE',
                               'SAS_PERSONAL_HISTORY_SSD_BASE', 'MAN_PERSONAL_HISTORY_SSD_BASE',
                               'MAN_YABS_FRONTEND_YABS', 'MSK_IMGS_BASE_NIDX', 'SAS_IMGS_BASE_NIDX', 'MAN_IMGS_BASE_NIDX', 'MSK_WEB_INTL2_PIP',
                               'MAN_WEB_INTL2_HAMSTER', 'MSK_WEB_INTL2_HAMSTER', 'SAS_WEB_INTL2_HAMSTER',
                               'MAN_DIVERSITY_JUPITER_BASE_PRIEMKA_PORTOVM', 'SAS_MAIL_LUCENE', 'ALL_QUERYSEARCH_USERDATA_PRIEMKA', 'MSK_WEB_INTL2_R1',
                               'VLA_IMGS_BASE_NIDX', 'VLA_IMGS_BASE_C1', 'VLA_IMGS_CBIR_BASE_PRIEMKA', 'VLA_ADDRS_12_BASE', 'MAN_ADDRS_18SHARDS_BASE',
                               'SAS_BERT_GPU_PROD', 'MAN_BERT_GPU_PROD', 'VLA_BERT_GPU_PROD']:
            continue

        # groups scheduled by search/tools/yp_alloc
        if 'INVERTED_INDEX' in group.card.name or 'EMBEDDING' in group.card.name or 'LUCENE' in group.card.name or 'REMOTE_STORAGE' in group.card.name:
            logging.info('%s skipped', group.card.name)
            continue

        if group.card.name in {'SAS_WEB_TIER1_INVERTED_INDEX', 'SAS_WEB_REMOTE_STORAGE_BASE_SLOTS_2', 'SAS_WEB_REMOTE_STORAGE_BASE_SLOTS', 'SAS_WEB_GEMINI_BASE', 'SAS_YP_COHABITATION_TIER1_BASE',
                               'VLA_VIDEO_PLATINUM_EMBEDDING', 'VLA_VIDEO_TIER0_EMBEDDING', 'VLA_IMGS_TIER0_EMBEDDING', 'VLA_WEB_TIER0_INVERTED_INDEX',
                               'SAS_VIDEO_PLATINUM_EMBEDDING', 'SAS_VIDEO_TIER0_EMBEDDING', 'SAS_IMGS_TIER0_EMBEDDING',  'SAS_VIDEO_PLATINUM_INVERTED_INDEX', 'SAS_VIDEO_TIER0_INVERTED_INDEX', 'SAS_IMGS_INVERTED_INDEX'}:
            logging.info('%s skipped', group.card.name)
            continue

        if group.card.name in {'VLA_WEB_TIER0_EMBEDDING_PIP', 'VLA_WEB_TIER0_BASE_PIP', 'VLA_WEB_TIER0_INVERTED_INDEX_PIP'}:
            logging.info('New WebTier0 acceptance (%s) skipped', group.card.name)
            continue

        if len(group.card.intlookups) == 0:
            continue
        if group.card.name.endswith('_NIDX'):
            continue
        unused_instances = set(group.get_instances()) - set(group.get_busy_instances())
        if len(unused_instances) > 0:
            print "!!! ERROR !!! Group %s: unused instances %s" % (group.card.name, ','.join(map(lambda x: '%s:%s' % (x.host.name, x.port), unused_instances)))
            failed = 1

    sys.exit(failed)

if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    main()
