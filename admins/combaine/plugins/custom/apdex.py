import logging


def is_apdex(name):
    return "_timings" in name


class _ApdexItem():
    def __init__(self, satisfied, tolerating):
        # limits
        self.satisfied = satisfied
        self.tolerating = tolerating

        # values
        self.satcount = 0
        self.tolcount = 0
        self.restcount = 0

    def push(self, values):
        for value in values:
            if value < self.satisfied:
                self.satcount += 1
            elif value < self.tolerating:
                self.tolcount += 1
            else:
                self.restcount += 1

    def value(self):
        return (self.satcount, self.tolcount, self.restcount)


def calc_apdex(sat, tol, rest):
    return (sat + tol / 2.0) / (sat + tol + rest)


class Apdex():
    def __init__(self, config):
        self.satisfied = config["satisfied"]
        self.tolerating = config["tolerating"]
        if self.tolerating < self.satisfied:
            raise ValueError("tolerating must be less than satisfied")

        # recalculate to msec for example
        factor = config.get("factor", 1)
        self.factor = lambda item: factor * float(item)

        self.log = config.get("logger", logging.getLogger())

    def aggregate_host(self, payload, prevtime, currtime, hostname=None):
        factor = self.factor
        sat, tol = self.satisfied, self.tolerating
        result = {}
        for raw_line in payload.splitlines():
            line = raw_line.decode('ascii', errors='ignore')
            if not is_apdex(line):
                continue
            name, _, metrics_as_strings = line.partition(" ")

            # no value for this metric has stored yet and no value now
            if name not in result:
                result[name] = _ApdexItem(sat, tol)

            if not metrics_as_strings:
                continue

            try:
                metrics_as_values = map(factor, metrics_as_strings.split())
                result[name].push(metrics_as_values)
            except ValueError as err:
                self.log.error("hostname=%s Unable to parse %s: %s", hostname, raw_line, err)
        return tuple((k, v.value()) for (k, v) in result.iteritems())

    def aggregate_group(self, payload):
        """payload: []("metric_name", (<sat>, <tol>, <res>))"""
        res = {}
        for payload_from_one in payload:
            for metric_name, values in payload_from_one:
                if metric_name in res:
                    fast = res[metric_name]
                    res[metric_name] = tuple(i + j for i, j in zip(fast, values))
                else:
                    res[metric_name] = values

        apdex_res = {}
        for k, v in res.iteritems():
            apdex_res[k] = calc_apdex(*v)
        return apdex_res


if __name__ == '__main__':
    logging.basicConfig()
    payload = "blabla_timings " + ' '.join(map(str, range(0, 100)))
    payload += "\nblabla_timings " + ' '.join(map(str, range(0, 100)))
    payload += "\nb_timings " + ' '.join(map(str, range(0, 300)))
    config = {"satisfied": 30, "tolerating": 90}
    apdex = Apdex(config)
    t = apdex.aggregate_host(payload, 0, 100)
    res = t[0]
    assert "blabla_timings" in res, res
    assert res[1] == (60, 120, 20), res[1]

    final = apdex.aggregate_group([t, t, t])
    assert "blabla_timings" in final, final
    assert final["blabla_timings"] == 0.6, final["blabla_timings"]
    assert final["b_timings"] == 0.2, final["b_timings"]
