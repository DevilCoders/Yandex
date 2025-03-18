#!/skynet/python/bin/python
"""Create command for dynamic main to create group using template group"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB


if __name__ == '__main__':
    template_group = CURDB.groups.get_group(sys.argv[1])
    new_name = sys.argv[2]

    if len(sys.argv) > 3:
        descr = sys.argv[3]
    else:
        descr = template_group.card.description

    owners = ','.join(template_group.card.owners)

    location = new_name.partition('_')[0]
    if location == 'MSK':
        location = '{}_{}'.format(location, new_name.split('_')[1])

    itype = template_group.card.tags.itype
    ctype = template_group.card.tags.ctype
    prj = "'[{}]'".format(', '.join('"{}"'.format(x) for x in template_group.card.tags.prj))
    metaprj = template_group.card.tags.metaprj

    if len(sys.argv) > 4:
        min_replicas = int(sys.argv[4])
    else:
        min_replicas = len(template_group.get_kinda_busy_instances())
    max_replicas = min_replicas

    if len(sys.argv) > 5:
        min_power = int(sys.argv[5]) * min_replicas
    else:
        min_power = int(sum(x.power for x in template_group.get_kinda_busy_instances()))

    if len(sys.argv) > 6:
        memory = sys.argv[6]
    else:
        memory = template_group.card.reqs.instances.memory_guarantee.text

    print ('./optimizers/dynamic/main.py -m ALL_DYNAMIC -a add -g {group} -d "{descr}" -o {owners} --location {location} --itype {itype} '
           '--ctype {ctype} --prj {prj} --metaprj {metaprj} --min_replicas {min_replicas} --max_replicas {max_replicas} --min_power {min_power} '
           '--equal_instances_power True --memory "{memory}" -y').format(group=new_name, descr=descr, owners=owners, location=location, itype=itype,
                                                                      ctype=ctype, prj=prj, metaprj=metaprj, min_replicas=min_replicas,
                                                                      max_replicas=max_replicas, min_power=min_power, memory=memory)


