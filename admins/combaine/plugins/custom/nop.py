try:
    import ujson as json
except ImportError:
    import json


class Nop():
    def __init__(self, config):
        self.config = config

    @staticmethod
    def aggregate_host(payload, _prevtime, _currtime, hostname=None):
        host = json.loads(payload)
        host["source"] = hostname
        return host

    @staticmethod
    def aggregate_group(payload):
        return payload
