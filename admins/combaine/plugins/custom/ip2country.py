import collections
import logging

import requests

try:
    import ujson as json
except ImportError:
    import json

logging.getLogger("requests").setLevel(logging.ERROR)
logging.getLogger("urllib3").setLevel(logging.ERROR)


class Ip2Country():
    def __init__(self, config):
        self.rps = config.get("rps", "yes") == "yes"
        self.log = config.get("logger", logging.getLogger())

    def aggregate_host(self, payload, prevtime, currtime, hostname=None):
        delta = float(currtime - prevtime)
        if delta < 1:
            delta = 1

        tmp = collections.defaultdict(list)
        unknown = collections.defaultdict(int)
        for raw_line in payload.splitlines():
            line = raw_line.decode('ascii', errors='ignore')
            code, _, ip_and_count = [x.strip() for x in line.partition('.')]
            if len(code) != 3 or ' ' in code or ' ' not in ip_and_count:
                self.log.error("Skip garbage line %s", raw_line)
                continue
            ip, _, count = [x.strip() for x in ip_and_count.partition(' ')]

            if ',' in ip:  # skip proxy addresses
                ip = ip.split(',')[0].strip()

            if '.' in ip:  # recheck ipv4
                if len(ip.split('.')) < 4:
                    self.log.error("Skip invalid ipv4 address with count %s", ip_and_count)
                    continue
            if ip.startswith('-'):
                count = int(count)
                if self.rps:
                    count /= delta
                unknown["Unknown." + code] += count
                continue

            tmp[code].append(ip + " " + count)
        result = self.resolve_ips(tmp, delta)
        result.update(unknown)
        return result

    def resolve_ips(self, data, delta):
        result = collections.defaultdict(int)
        for code in data:
            text = "\n".join(data[code])
            try:
                resp = requests.post("http://localhost:8080", text, {"Content-type": "text/plain"})
                if resp.status_code != requests.codes.ok:
                    self.log.error("ip2country response %s: %s", resp.status_code, resp.reason)
                    continue

                resp = resp.json() or {}
                for country, count in resp.items():
                    if not country:
                        # geobase response for local ips?
                        country = "Unknown"

                    if self.rps:
                        count /= delta
                    result[country + "." + code] += count

            except Exception as exc:
                self.log.error("ip2country request exception %s", exc)

        return result

    @staticmethod
    def aggregate_group(payload):
        response = collections.Counter()
        for p in payload:  # [{...}, {...}, ...]
            response.update(p)
        return response


def _test():
    """Simple test with time measurement"""
    import sys
    import time
    from textwrap import dedent

    payload = dedent("""404.- 12
    500.2a02:6b8:b011:4005:216:3eff:fe4b:27bd 6
    200.2a02:6b8:b011:4005:216:3eff:fe4b:27bd 6
    200.2a02:6b8:0:1a16:216:3eff:fefb:f5cd 4
    200.2a02:6b8:0:1a16:216:3eff:fe83:cc84 1
    200.2a02:6b8:b011:4005:216:3eff:fe41:ad04 3
    """).encode()
    do_assert = True
    if len(sys.argv) > 1:
        payload = open(sys.argv[1]).read()
        do_assert = False

    ip2c = Ip2Country({"rps": "yes"})
    print("+++ {} +++".format(ip2c.__dict__))
    starth = time.time()
    res = ip2c.aggregate_host(payload, 1, 3)  # 2 secons
    if do_assert:
        print("Result with rps in 2 seconds: {}".format(dict(res)))
        # Check in rps !
        assert (res == {'Unknown.404': 6, 'Russia.200': 2.5, 'Finland.200': 4.5, 'Finland.500': 3.0})
    print("+++ Aggregate host ({} s) +++".format(time.time() - starth))

    ip2c = Ip2Country({"rps": "no"})
    group = [ip2c.aggregate_host(payload, 1, 3)] * 45
    startg = time.time()
    res = ip2c.aggregate_group(group)
    if do_assert:
        print("Result without rps in 2 seconds: {}".format(dict(res)))
        # Check in rps !
        assert (res == {"Finland.500": 18, "Finland.200": 27, "Russia.200": 15, "Unknown.404": 36})
    print("+++ Aggregate group ({} s) +++".format(time.time() - startg))
    print("+++ Aggregate total ({} s) +++".format(time.time() - starth))
    print(json.dumps(res, indent=1))


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    _test()
