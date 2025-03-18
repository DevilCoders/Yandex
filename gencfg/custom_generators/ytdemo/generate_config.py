#!/skynet/python/bin/python
# vim: set fileencoding=utf-8:

import errno
import os
import sys
import json

path = os.path.abspath(__file__)
for i in range(3):  # three directories down
    path = os.path.dirname(path)
sys.path.append(path)

import gencfg
import config
from core.db import CURDB


GROUPS = [
    "MSK_YT_DEMO_MASTER",
    "MSK_YT_DEMO_NODE",
    "MSK_YT_DEMO_PROXY",
    "MSK_YT_DEMO_SCHEDULER",

]

def cast_instances(instances):
    result = []
    for instance in instances:
        result.append({
            "fqdn": instance.host.name,
            "port": instance.port,
            "power": instance.power,
            "host": json.loads(instance.host.save_to_json_string()),
        })

    result.sort(key=lambda x: (x['fqdn'], x['port']))
    return result



if __name__ == '__main__':
    print "- Gathering host names"
    data = {}
    for group in GROUPS:
        instances = CURDB.groups.get_group(group).get_instances()
        data[group] = cast_instances(instances)

    result = {}
    result['data'] = data
    result['tag'] = CURDB.get_repo().get_current_tag()
    import json
    text = json.dumps(data, sort_keys=True, indent=4)

    with open(sys.argv[1], 'w') as f:
        f.write(text)

