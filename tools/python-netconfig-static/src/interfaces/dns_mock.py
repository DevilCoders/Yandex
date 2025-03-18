
from collections import defaultdict


def prepare(data):
    def fqdn():
        return {4: [], 6: []}
    result = defaultdict(fqdn)
    for k, v in data.iteritems():
        result[k] = v
        if "4" in v:
            v[4] = v["4"]
            del v["4"]
        if "6" in v:
            v[6] = v["6"]
            del v["6"]
    return result
