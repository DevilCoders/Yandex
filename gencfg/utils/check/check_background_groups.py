#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from gaux.aux_colortext import red_text


IGNORE_BACKGROUND_GROUPS = ('ALL_ISS_AGENT', 'MAN_JUGGLER_CLIENT_STABLE', 'MAN_YASM_YASMAGENT_STABLE', 'SAS_JUGGLER_CLIENT_STABLE',
                            'SAS_YASM_YASMAGENT_PRESTABLE', 'SAS_YASM_YASMAGENT_STABLE', 'MSK_JUGGLER_CLIENT_STABLE',
                            'MSK_YASM_YASMAGENT_STABLE', 'VLA_JUGGLER_CLIENT_STABLE', 'VLA_YASM_YASMAGENT_STABLE',
                            'MAN_WEB_DEPLOY', 'SAS_WEB_DEPLOY', 'VLA_WEB_DEPLOY', 'VLA_IMGS_BETA_DEPLOY',
                            'SAS_RTC_SLA_TENTACLES_PROD', 'MAN_RTC_SLA_TENTACLES_PROD', 'VLA_RTC_SLA_TENTACLES_PROD', 'MSK_RTC_SLA_TENTACLES_PROD')


def main():
    host_background_groups = defaultdict(set)
    host_background_groups_disabled = defaultdict(set)

    # fill data
    for group in CURDB.groups.get_groups():
        if group.card.properties.fake_group and group.card.properties.background_group:
            continue
        if group.card.name in IGNORE_BACKGROUND_GROUPS:
            continue

        hosts = group.getHosts()
        if group.card.properties.background_group:
            for host in hosts:
                host_background_groups[host].add(group)
        elif not group.card.properties.allow_background_groups:
            for host in hosts:
                host_background_groups_disabled[host].add(group)

    # find intersection
    same_hosts = set(host_background_groups) & set(host_background_groups_disabled)
    if same_hosts:
        for host in same_hosts:
            s1 = ','.join(sorted(x.card.name for x in host_background_groups[host]))
            s2 = ','.join(sorted(x.card.name for x in host_background_groups_disabled[host]))
            print red_text('Host {} has background groups <{}> and groups where background groups disabled: <{}>'.format(host.name, s1, s2))
    else:
        return 0


if __name__ == '__main__':
    retval = main()
    sys.exit(retval)
