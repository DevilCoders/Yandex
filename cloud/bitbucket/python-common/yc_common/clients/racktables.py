import logging
import requests
from requests import Timeout, ReadTimeout

LOG = logging.getLogger(__name__)


class RackTablesClient(object):

    def __init__(self, rack_tables_url):
        self.rack_tables_url = rack_tables_url

    def get_networks(self):
        try:
            rack_tables_result = requests.get(self.rack_tables_url, timeout=3)
            LOG.debug("rack tables result %s", rack_tables_result)
        except (RuntimeError, ConnectionError, Timeout, ReadTimeout) as e:
            LOG.warning('Failed to fetch data from racktables: %s', e)
            return 'Failed to fetch data from racktables', 500
        if rack_tables_result:
            if rack_tables_result.status_code == 200:
                networks = {net.strip(): 'yandex'
                            for net in rack_tables_result.text.split('\n')
                            if len(net.strip())}
                networks['0.0.0.0/0'] = 'internet'
                networks['::/0'] = 'internet'
                return networks
            else:
                LOG.error("request to racktables failed: %d %s", rack_tables_result.status_code,
                          rack_tables_result.text)
                return 'Failed to fetch data from racktables', 500
