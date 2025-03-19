import logging
from collections import defaultdict
from math import floor

try:
    import ujson as json
except ImportError:
    import json


class Yasm():
    """
    The yasm subagent data aggregator
    """

    # https://wiki.yandex-team.ru/balancer/cookbook/#tablicadostupnyxsignalov
    METRICS = {
        '1xx': 'outgoing_1xx',
        '2xx': 'outgoing_2xx',
        '3xx': 'outgoing_3xx',
        '4xx': 'outgoing_4xx',
        '5xx': 'outgoing_5xx',
        'fail': 'fail',
        'conn_fail': 'conn_fail',
        'backend_fail': 'backend_fail',
        'client_fail': 'client_fail',
    }

    PERCENTILES = (90, 95, 99, 100)

    def __init__(self, config):
        self.config = config
        self.log = config.get("logger", logging.getLogger())
        self.sections = config.get("sections", [])

    def aggregate_host(self, payload, prevtime, currtime, hostname=None):
        self.log.info("Aggregating host %s (payload length: %s)", hostname, len(payload))
        payload = json.loads(payload)

        # extract real data
        data_keys = []
        for key in payload['get']:
            if key.startswith('localhost_'):
                continue
            data_keys.append(key)
        data = payload['get'][data_keys[0]]['values']

        # extract real data
        result = {}
        for section in self.sections:
            result[section] = {}
            for metric in self.METRICS:
                try:
                    result[section][metric] = data['balancer_report-report-{}-{}_summ'.format(
                        section, self.METRICS[metric])]
                except KeyError:
                    self.log.error('%s has no metric %s, skipping', hostname, metric)
                    result[section][metric] = 0
            try:
                result[section]['hgram'] = dict(data['balancer_report-report-{}-2xx-u-u_hgram'.format(section)][1])
            except KeyError:
                self.log.error('%s has no histogram, skipping', hostname)
                result[section]['hgram'] = {}

        return result

    def _join_metrics(self, target_metrics, target_hgram, source):
        for metric in self.METRICS:
            target_metrics[metric] += source[metric]
        for k in source['hgram']:
            target_hgram[k] += source['hgram'][k]

    def _hgram_to_timings(self, hgram):
        data = []
        for k, v in hgram.items():
            for i in range(v):
                data.append(k)
        data.sort()

        count = len(data)
        result = {}
        for percentile in self.PERCENTILES:
            try:
                result['{}_prc'.format(percentile)] = data[int(floor(count * percentile / 100)) - 1]
            except IndexError:
                self.log.error('Could not calculate %s percentile (total time list size = %s, hgram key count = %s)',
                               percentile, count, len(hgram))

        return result

    def aggregate_group(self, payload):
        metrics = {}
        hgrams = {}
        for section in self.sections:
            metrics[section] = dict((x, 0) for x in self.METRICS)
            hgrams[section] = defaultdict(int)

        for item in payload:
            for section in self.sections:
                try:
                    self._join_metrics(metrics[section], hgrams[section], item[section])
                except KeyError as exc:
                    self.log.error('error: %s', exc)

        for section in self.sections:
            # noinspection PyTypeChecker
            metrics[section]['timings'] = self._hgram_to_timings(hgrams[section])
        return metrics


def main():
    import yaml
    hosts = (
        'man1-4336-man-music-balancer-14540.gencfg-c.yandex.net',
        'man1-4443-man-music-balancer-14540.gencfg-c.yandex.net',
    )
    yasm = Yasm({
        'sections': ['music'],
    })

    items = []
    for host in hosts:
        data = open('{}.json'.format(host)).read()
        agg_data = yasm.aggregate_host(data, None, None, host)
        items.append(agg_data)
        print(yaml.safe_dump(agg_data))

    print(yaml.safe_dump(yasm.aggregate_group(items)))


if __name__ == '__main__':
    main()
