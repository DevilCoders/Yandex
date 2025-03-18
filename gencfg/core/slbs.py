"""Slbs from racktables (GENCFG-1781)"""


import os
import sys

from collections import OrderedDict

import json


class TSlbs(object):
    """Storage with slbs"""

    class TSlb(object):
        """"Slb info"""

        def __init__(self, jsoned):
            self.fqdn = jsoned['fqdn']
            self.ips = jsoned['ips']

        def to_json(self):
            return dict(fqdn=self.fqdn, ips=self.ips)

    def __init__(self, db):
        self.db = db
        self.modified = False
        self.data = OrderedDict()

        self.DATA_FILE = os.path.join(self.db.PATH, 'slbs.json')

        if self.db.version < '2.2.42':
            return

        jsoned = json.loads(open(self.DATA_FILE).read())

        for slb_info in json.loads(open(self.DATA_FILE).read()):
            assert slb_info['fqdn'] not in self.data
            self.data[slb_info['fqdn']] = TSlbs.TSlb(slb_info)

    def mark_as_modified(self):
        self.modified = True

    def has(self, fqdn):
        return fqdn in self.data

    def get(self, fqdn):
        return self.data[fqdn]

    def get_all(self):
        return self.data.values()

    def add(self, slb_info):
        assert slb_info['fqdn'] not in self.data
        self.data[slb_info['fqdn']] = TSlbs.TSlb(slb_info)
        self.mark_as_modified()

    def remove(self, fqdn):
        self.data.pop(fqdn)
        self.mark_as_modified()

    def update(self, smart=False):
        # if self.db.version < '2.2.42':
        #     return

        if smart and not self.modified:
            return

        result = [x.to_json() for x in self.data.itervalues()]

        with open(self.DATA_FILE, 'w') as f:
            f.write(json.dumps(result, indent=4))

    def fast_check(self, timeout):
        pass
