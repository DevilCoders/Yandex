import logging

DEFAULT_QUANTILE_VALUES = [75, 90, 93, 94, 95, 96, 97, 98, 99]


def _add_timings(container, name, timings_value):
    """
    Pack comming timings in to compact dict where
    keys are timings, and values are count of timings
    """
    try:
        tim_dict = container[name]
    except KeyError:
        container[name] = {}
        tim_dict = container[name]

    count = 1.0
    if name[0] == '@':  # is packed timings
        if '@' in timings_value:
            timings_value, count = timings_value.split("@")
        count = float(count)

    # use timings_value as string here
    # to avoid msgpack 'ValueError: float is not allowed for map key'
    try:
        tim_dict[timings_value] += count
    except KeyError:
        tim_dict[timings_value] = count


class Multimetrics():
    """
    General metrics count:
    ping.5xx 12
    api.2xx 3000
    traffix 234324234

    Special aggregator of prehandled quantile data
    Data looks like:
    uploader_timings_request_post_patch-url 0.001 0.001 0.002
    uploader_timings_request_post_upload-from-service
    uploader_timings_request_post_upload-url 0.001 0.002 0.001 0.002 0.001
    uploader_timings_request_put_patch-target 0.651 0.562 1.171

    or timings in packed form:
    metric name should startwith @
    @uploader_timings_request_post_upload-url 0.001@300 0.002@3 0.001@12
    """

    def __init__(self, config):
        self.quantile = list(config.get("values", DEFAULT_QUANTILE_VALUES))
        self.quantile.sort()
        # recalculate to rps? default yes
        self.rps = (config.get("rps", "yes") == "yes")
        # find timings by specified string. default '_timings'
        self.timings_is = config.get("timings_is", "_timings")
        # multiply on factor: default `1`
        factor = float(config.get("factor", 1))
        self.factor = lambda item: factor * item
        # get prc. By default - No. Format: { ext_services: error/info }
        # get prc of errors in info from metrics which contents 'ext_services'
        self.get_prc = config.get("get_prc", False)

        self.log = config.get("logger", logging.getLogger())

    def is_timings(self, name):
        "Check metric name against is timings"
        return name[0] == '@' or self.timings_is in name

    def aggregate_host(self, payload, prevtime, currtime, hostname=None):
        """ Convert strings of 'payload' into dict[string][]float and return """

        delta = float(currtime - prevtime)
        if delta <= 0:
            delta = 1.0

        result = {}
        for raw_line in payload.splitlines():
            line = raw_line.decode('ascii', errors='ignore')
            line = line.strip().replace('\t', ' ')
            if not line:
                continue

            name, _, metrics_as_strings = map(lambda x: x.strip(), line.partition(' '))
            if any(c in name for c in "<>;\\"):
                self.log.error("hostname=%s Name of metric contains forbidden symbols: '<>;\\'", hostname)
                continue

            try:
                if self.is_timings(name):
                    metrics_as_strings = metrics_as_strings.replace(',', ' ').replace('-', ' ').replace(':', ' ')
                    if not metrics_as_strings:
                        continue

                    if name not in result:
                        result[name] = {}

                    for tmn in metrics_as_strings.split():
                        _add_timings(result, name, tmn)
                else:
                    metrics_as_values = 0
                    if metrics_as_strings:
                        metrics_as_values = sum(map(float, metrics_as_strings.split()))

                    if self.rps:
                        metrics_as_values /= delta

                    try:
                        result[name] += metrics_as_values
                    except KeyError:
                        result[name] = metrics_as_values
            except Exception as err:  # pylint: disable=broad-except
                self.log.error("hostname=%s Unable to parse %s: %s", hostname, line, err)
        return result

    def aggregate_group(self, payload):
        """ Payload is list of dict[string][]float"""
        if not payload:
            raise Exception("No data to aggregate")

        names_of_metrics = set()
        for i in payload:
            names_of_metrics.update(i.keys())
        result = {}

        for metric in names_of_metrics:
            if self.is_timings(metric):
                agg_timings = {}
                for item in payload:
                    for tmk, val in item.get(metric, {}).items():
                        try:
                            agg_timings[tmk] += val
                        except KeyError:
                            agg_timings[tmk] = val

                if not agg_timings:
                    continue

                if metric[0] == '@':
                    metric = metric[1:]  # make timings name valid
                result[metric] = self.calculate_quantiles(agg_timings)
            else:
                metric_sum = sum(item.get(metric, 0) for item in payload)
                result[metric] = metric_sum

        if self.get_prc:
            self.calculate_percentage(result)

        return result

    def calculate_quantiles(self, timings):
        """
        Calculate quantiles from dict {'timings_value': 'values_count', ...}
        """

        values = {}
        total_count = 0

        for k, v in timings.items():
            try:
                f = float(k)
                total_count += v

                try:
                    values[f] += v
                except KeyError:
                    values[f] = v

            except ValueError:
                pass

        keys = sorted(values.keys())
        value = keys[0]
        quantiles = [value] * len(self.quantile)

        for idx, quant in enumerate(self.quantile):
            if quant < 100:
                index = int(total_count / 100 * quant)
                sumidx = 0
                for i in keys:
                    sumidx += values[i]
                    value = i
                    if index <= sumidx:
                        break
            else:
                value = keys[-1]

            quantiles[idx] = self.factor(value)
        return quantiles

    def calculate_percentage(self, result):
        """Get percentage one metric in other"""

        for pattern in self.get_prc:
            calc_result = {}
            # prc_result = base * 100 / all , where base, all - last key in metric
            base_key, all_key = self.get_prc[pattern].split('/')
            for metric in result:
                # Process only matching our pattern metrics
                if pattern not in metric:
                    continue
                # Construct metric parts
                metric_prefix = metric.rsplit('.', 1)[0]
                metric_base = '%s.%s' % (metric_prefix, base_key)
                metric_all = '%s.%s' % (metric_prefix, all_key)
                metric_result = '%s.err_prc' % (metric_prefix)
                # Do not process metric if we already have result
                if metric_result in calc_result:
                    continue

                try:
                    metric_base_value = result[metric_base]
                except KeyError:
                    # If there is no metric with base value, there 0% base in all
                    calc_result[metric_result] = 0
                    continue

                try:
                    metric_all_value = result[metric_all]
                except KeyError:
                    # If there is no metric with all value, there 100% base in all
                    calc_result[metric_result] = 100
                    continue

                if metric_all_value < metric_base_value:
                    calc_result[metric_result] = 100
                else:
                    calc_result[metric_result] = round(metric_base_value * 100.0 / metric_all_value, 2)
            result.update(calc_result)


def test(datafile):
    """Simple test with time measurement"""
    import time
    from pprint import pprint

    logging.getLogger().setLevel(logging.DEBUG)
    mms = Multimetrics({"timings_is": "_time", "values": [75, 90, 93, 94, 95, 96, 97, 98, 99, 100]})
    print("+++ {} +++".format(mms.__dict__))
    with open(datafile, 'rb') as fname:
        _payload = fname.read()
    start = time.time()
    res = mms.aggregate_host(_payload, 1, 3)
    print("+++ Aggregate host ({} s) +++".format(time.time() - start))
    pprint(mms.aggregate_group([res]), indent=1, width=120)

    _payload = [res, res, res, res, res, res, res, res, res, res, res, res, res]

    start = time.time()
    res = mms.aggregate_group(_payload)
    print("+++ Aggregate group ({} s) +++".format(time.time() - start))
    pprint(res, indent=1, width=120)


if __name__ == '__main__':
    import sys
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    test(sys.argv[1])
