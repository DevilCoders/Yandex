import logging


class Traffic():
    """
    Special aggregator of traffic loggiver handle
    """

    def __init__(self, config):
        self.debug = config.get("debug", False)
        self.log = config.get("logger", logging.getLogger())

    def aggregate_host(self, payload, prevtime, currtime, hostname=None):
        """
        Convert strings of payload into dict[string]int and return
        Sample payload
        clip00d.video.yandex.net:eth1:rx:0:tx:0
        clip00d.video.yandex.net:eth0:rx:852350:tx:6374659
        """
        result = {'rx': 0, 'tx': 0}
        for raw_line in payload.splitlines():
            iface = raw_line.decode('ascii', errors='ignore').split(":")
            if len(iface) == 6:
                try:
                    result['rx'] += int(iface[3]) * 8  # bits per seconds
                    result['tx'] += int(iface[5]) * 8  # bits per seconds
                except Exception as err:
                    self.log.error("hostname=%s Unable to parse %s: %s", hostname, raw_line, err)
        return result

    def aggregate_group(self, payload):
        """ Payload is list of dict[string]int"""
        if len(payload) == 0:
            raise Exception("No data to aggregate")

        result = {'rx': 0, 'tx': 0}
        for host in payload:
            try:
                result['rx'] += host['rx']
                result['tx'] += host['tx']
            except KeyError:
                continue
        return result


if __name__ == '__main__':
    from pprint import pprint
    from random import randrange

    m = Traffic({})
    sample_line = """clip00d.video.yandex.net:eth1:rx:{0[0]}:tx:{0[1]}
                     clip00d.video.yandex.net:eth0:rx:{0[2]}:tx:{0[3]}"""

    group_payload = []

    def get_sample_line():
        return sample_line.format([randrange(0, 1000000) for i in xrange(4)])

    pprint(m.aggregate_host(get_sample_line(), None, None))
    for i in xrange(0, 1000):
        group_payload.append(m.aggregate_host(get_sample_line(), None, None))

    pprint(m.aggregate_group(group_payload))
