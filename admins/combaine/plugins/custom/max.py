"""
The 'max' custom combaine plugin.
Recive plain text data as payload for aggregate_host.
Make dict where keys are metrics name in payload,
this dict contains only maximum from metrics values.
Aggregate group function merge hosts dicts in one dict,
and pick only maximum value for key.
"""

from __future__ import print_function

import logging


class Max():
    """
    Special aggregator of prehandled quantile data (fork Average)
    Data looks like:
    metric1 40 55 323
    metric2
    metirc3 10 20 30 44
    metirc4 8 9 10
    """

    def __init__(self, config):
        self.config = config
        self.log = config.get("logger", logging.getLogger())

    def aggregate_host(self, payload, prevtime, currtime, hostname=None):
        # pylint: disable=no-self-use,unused-argument
        """ Convert strings of payload into dict[string][]float and return """
        result = {}
        for raw_line in payload.splitlines():
            line = raw_line.decode('ascii', errors='ignore')
            name, _, metrics_as_strings = line.partition(" ")
            # put a default placeholder here if there's no such result yet
            if not metrics_as_strings:
                continue
            try:
                metrics_as_values = sum(float(i) for i in metrics_as_strings.split())
                if name in result:
                    result[name] += metrics_as_values
                else:
                    result[name] = metrics_as_values
            except ValueError as err:
                self.log.error("hostname=%s Unable to parse %s: %s", hostname, raw_line, err)
        return result

    def aggregate_group(self, payload):
        # pylint: disable=no-self-use
        """ Payload is list of dict[string][]float"""
        if len(payload) == 0:
            raise Exception("No data to aggregate")

        names_of_metrics = set()
        for one_dict in payload:
            names_of_metrics.update(one_dict.keys())

        result = {}
        for metric in names_of_metrics:
            metric_max = max(item.get(metric, 0) for item in payload)
            result[metric] = metric_max

        return result


if __name__ == '__main__':
    from pprint import pformat as pf  # pylint: disable=wrong-import-position

    HOST_PAYLOAD = [
        b"metric1 3 3 3 1\nmetric2\nmetirc3 21 9\nmetirc4 35 5",  # Host1
        b"metric1 4 3 3 1\nmetric2 20\nmetirc3 21 9\nmetirc4 35 5",  # Host2
        b"metric1 9 1\nmetric2 10 11\nmetirc3 30\nmetirc4 45 5",  # Host3
        b"metric1 10\nmetric2 15 5\nmetirc3\nmetirc4"  # Host4
    ]
    GROUP_PAYLOAD = []
    MAX = Max({})
    for idx, one_metrics_line in enumerate(HOST_PAYLOAD):
        res = MAX.aggregate_host(one_metrics_line, 0, 0)
        print("Agg host{0} res:".format(idx), res)
        GROUP_PAYLOAD.append(res)
    print("Result of hosts aggregations: {0}".format(pf(GROUP_PAYLOAD)))
    AGG_GROUP = pf(MAX.aggregate_group(GROUP_PAYLOAD))
    print("Result of group aggregations: {0}".format(AGG_GROUP))
