import logging
from functools import reduce


class Average():
    """
    Special aggregator of prehandled quantile data
    Data looks like:
    metric1 40 55 323
    metric2
    metirc3 10 20 30 44
    metirc4 8 9 10
    """

    def __init__(self, config):
        self.total = config.get("total", "no")
        self.log = config.get("logger", logging.getLogger())

    def aggregate_host(self, payload, prevtime, currtime, hostname=None):
        """ Convert strings of payload into dict[string][]float and return """
        result = {}
        for raw_line in payload.splitlines():
            line = raw_line.decode('ascii', errors='ignore')
            name, _, metrics_as_strings = line.partition(" ")
            # put a default placeholder here if there's no such result yet
            if not metrics_as_strings:
                continue
            try:
                metrics_as_values = sum(map(float, metrics_as_strings.split()))
                if name in result:
                    result[name] += metrics_as_values
                else:
                    result[name] = metrics_as_values
            except ValueError as err:
                self.log.error("hostname=%s Unable to parse %s: %s", hostname, raw_line, err)
        return result

    def aggregate_group(self, payload):
        """ Payload is list of dict[string][]float"""
        result = {}
        if len(payload) == 0:
            return result
        names_of_metrics = set()
        for i in payload:
            names_of_metrics.update(i.keys())
        for metric in names_of_metrics:
            metric_len = reduce(lambda i, x: i + 1 if metric in x else i, payload, 0)
            if not metric_len:
                metric_len = 1
            metric_sum = sum(item.get(metric, 0) for item in payload)
            result[metric] = metric_sum / metric_len

        return result


if __name__ == '__main__':
    host_payload = [
        "metric1 3 3 3 1\nmetric2\nmetirc3 21 9\nmetirc4 35 5",
        "metric1 3 3 3 1\nmetric2 20\nmetirc3 21 9\nmetirc4 35 5",
        "metric1 9 1\nmetric2 10 10\nmetirc3 30\nmetirc4 35 5", "metric1 10\nmetric2 15 5\nmetirc3\nmetirc4"
    ]
    group_payload = []
    m = Average({})
    for i in host_payload:
        res = m.aggregate_host(i, 0, 0)
        print("Agg host res:", res)
        group_payload.append(res)
    print("Agg group payload:", group_payload)
    print("Agg group:", m.aggregate_group(group_payload))
