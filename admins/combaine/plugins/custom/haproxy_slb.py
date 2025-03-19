import logging
from collections import Counter


class HaproxySlb():
    def __init__(self, config):
        self.config = config
        self.log = config.get("logger", logging.getLogger())

    def aggregate_host(self, payload, prevtime, currtime, hostname=None):

        lines = payload.decode('ascii', errors='ignore').strip().split("\n")
        # index 0 is headers. index > 0 are proxies
        # trim leading symbols in  '# pxname' and trailing empty field after trailing  comma
        # data example: input: "# foo,bar,", output: [ "foo", "bar" ]
        headers = lines[0][2:-1].split(',')
        proxies = [stat.split(',') for stat in lines[1:]]

        wanted_metrics = [
            "type",  # (0=frontend, 1=backend, 2=server, 3=socket/listener)
            "stot",  # session total
            "scur",  # current sessions
            "econ",  # number of requests that encountered an error trying to connect to a backend server
            "ereq",  # response errors
            "wretr",  # number of times a connection to a server was retried
            "wredis",  # number of times a request was redispatched to another
            "act",  # number of active servers (backend), server is active (server)
            "rate",  # number of sessions per second over last elapsed second
            "cli_abrt",  # number of data transfers aborted by the client
            "srv_abrt",  # number of data transfers aborted by the server
            "hrsp_1xx",
            "hrsp_2xx",
            "hrsp_3xx",
            "hrsp_4xx",
            "hrsp_5xx",
            # "status",
            "chkfail",  # number of failed checks
            "chkdown",  # number of UP->DOWN transitions
            "lbtot",  # number of times this server was selected for a request
            "bin",  # bytes in
            "bout",  # bytes out
        ]

        # "pxname",  # proxy name
        # "svname",  # server name
        ret = list()
        for proxy in proxies:
            zipped = dict(zip(headers, proxy))
            for metric in zipped:
                if metric in wanted_metrics:
                    key = ".".join([zipped["pxname"], zipped["svname"], metric])
                    val = int(zipped[metric]) if zipped[metric] != "" else 0
                    ret.append({key: val})

        return ret

    def aggregate_group(self, payload):
        counter = Counter()
        for load in payload:
            for d in load:
                counter.update(d)

        return dict(counter)


if __name__ == "__main__":
    import requests
    from pprint import pprint as pp

    hp = HaproxySlb({})

    hosts = [
        "music-stable-wsapi-proxy-myt-1.myt.yp-c.yandex.net",
    ]

    payload = list()
    for host in hosts:
        ret = requests.get("http://" + host + ":9002/;csv")
        payload.append(hp.aggregate_host(ret.text.encode(), None, None, host))
    pp(hp.aggregate_group(payload))
