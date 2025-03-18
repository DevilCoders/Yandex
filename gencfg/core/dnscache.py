"""Local dns cache (for example, when we do not want resolve guest host names RX-213)"""

import os
import simplejson
import time


class TDnsCache(object):
    '''Dns cache (simple mapping (hostname) -> (ipv6addr)'''

    class TDnsCacheEntry(object):
        '''Single host dns cache info'''
        def __init__(self, ipv6addr, last_checked):
            self.ipv6addr = ipv6addr
            self.last_checked = last_checked

        def to_json(self):
            return dict(
                ipv6addr=self.ipv6addr,
                last_checked=self.last_checked,
            )

    def __init__(self, db):
        self.db = db
        self.modified = False
        self.DATA_FILE = os.path.join(self.db.PATH, 'dnscache.json')

        if self.db.version <= '2.2.39':
            self.data = dict()
        else:
            self.data = simplejson.loads(open(self.DATA_FILE).read())
            for k in self.data:
                self.data[k] = TDnsCache.TDnsCacheEntry(self.data[k]['ipv6addr'], self.data[k]['last_checked'])

    def has(self, hostname):
        return hostname in self.data

    def get(self, hostname, raise_failed=True):
        if hostname in self.data:
            return self.data[hostname].ipv6addr
        if not raise_failed:
            return None
        raise Exception('Hostname {} not found in dns cache'.format(hostname))

    def modify(self, hostname, addr):
        '''Update addr for host'''
        now = int(time.time())
        if hostname not in self.data:
            self.data[hostname] = TDnsCache.TDnsCacheEntry(addr, now)
        else:
            self.data[hostname].addr = addr
            self.data[hostname].last_checked = now

        self.modified = True

    def remove(self, hostname):
        self.data.remove(hostname)

        self.modified = True

    def update(self, smart=True):
        if self.db.version <= '2.2.39':
            return
        if smart and not self.modified:
            return

        with open(self.DATA_FILE, 'w') as f:
            json_data = {x: y.to_json() for x, y in self.data.iteritems()}
            f.write(simplejson.dumps(json_data, indent=4, sort_keys=True))

    def fast_check(self, timeout):
        # nothing to check
        pass
