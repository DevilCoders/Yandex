#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from core.db import CURDB
from gaux.aux_decorators import gcdisable

LOCATION_IGNORE_GROUPS = ['MSK_WEB_BASE_R1', 'SAS_SAAS_CLOUD_BASE_HAMSTER', 'MSK_WEB_YLITE', 'SAS_SAAS_CLOUD_BASE',
                          'SAS_VMBENCHMARK_OPENSTACK_VM', 'MSK_QUERYDATASUPPORT', 'MSK_OXYGEN_BASE',
                          'MSK_IMGS_NMETA_PRIEMKA_RUS_LATEST', 'MSK_FUSION_BASE_3DAY_UKROP_EXPERIMENT',
                          'MSK_WEB_NMETA_PRIEMKA_RUS_LATEST', 'MSK_WEB_NMETA_PRIEMKA_RUS_UPPER',
                          'MSK_IMGS_NMETA_PRIEMKA_RUS_UPPER', 'MSK_FUSION_MMETA_10DAY_UKROP_EXPERIMENT',
                          'MSK_IMGS_IMPROXY_R1', 'MSK_WEB_BASE_PRIEMKA', 'MSK_SDMS_POOL', 'SAS_YASM_YASMAGENT_PRESTABLE', 'MSK_CAPI_LOAD_TEST']

@gcdisable
def main():
    constraints = [
        ("Non-msk hosts", lambda x: x.card.name.startswith('MSK_') and x.card.name not in LOCATION_IGNORE_GROUPS,
         lambda x: x.location != 'msk'),
        ("Non-ams hosts", lambda x: x.card.name.startswith('AMS_') and x.card.name not in LOCATION_IGNORE_GROUPS,
         lambda x: x.location != 'ams'),
        ("Non-sas hosts", lambda x: x.card.name.startswith('SAS_') and x.card.name not in LOCATION_IGNORE_GROUPS,
         lambda x: x.location != 'sas'),
        #        ("No amd in production", lambda x: x.name == 'MSK_WEB_BASE', lambda x: x.model.startswith('AMD')),  Should be fixed in stable-63
    ]

    failed = False
    for group in CURDB.groups.get_groups():
        # to not check slave groups and groups from ignore list
        if group.card.master is not None:
            continue
        if group.card.properties.nonsearch:
            continue

        for descr, group_filter, host_filter in constraints:
            if group_filter(group):
                failed_hosts = filter(lambda x: host_filter(x), group.getHosts())
                if len(failed_hosts):
                    print "Constraint <<%s>> failed for group %s, hosts: %s" % (descr, group.card.name, ','.join(map(lambda x: x.name, failed_hosts)))
                    failed = True

    if failed:
        sys.exit(1)

if __name__ == '__main__':
    main()
