from collections import defaultdict, Counter
import json


def load_pods(filename):
    with open(filename) as f:
        data = json.load(f)
        pods = [obj for obj in data if obj['meta']['type'] == 'pod']
        return pods


def load_instances(f):
    for pod in load_pods(f):
        yield pod['meta']['pod_set_id'], pod['spec']['node_id']


def load_groups(f):
    groups = defaultdict(lambda: defaultdict(Counter))
    for group, host in load_instances(f):
        groups[group][host] += 1
    return groups


def load_groups(f):
    groups = defaultdict(Counter)
    for group, host in load_instances(f):
        groups[group][host] += 1
    return groups


if __name__ == '__main__':
    groups = load_groups('/home/okats/arcadia/gencfg/sandbox/result1.json')
    groups['SAS_WEB_TIER1_JUPITER_BASE'].most_common(5)
