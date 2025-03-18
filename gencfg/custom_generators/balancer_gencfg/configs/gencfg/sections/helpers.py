from collections import OrderedDict


def endpointsets(name, dcs):
    return [
        OrderedDict([
            ('cluster_name', dc),
            ('endpoint_set_id', name)
        ])
        for dc in dcs
    ]
