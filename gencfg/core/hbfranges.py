"""Local storage of hbf ranges for groups"""

import os
import json

import gencfg

from gaux.aux_mongo import get_next_hbf_project_id


class HbfRange(object):
    def __init__(self, name, from_, to, acl):
        self.name = name
        self.from_ = from_  # name `from` from racktables API, but it keyword
        self.to = to
        self.acl = acl

    def get_next_project_id(self):
        return get_next_hbf_project_id(self.name)

    @staticmethod
    def from_json(name, jsoned):
        return HbfRange(name, jsoned['from'], jsoned['to'], jsoned['acl'])

    def to_json(self):
        return {
            self.name: {
                'from': self.from_,
                'to': self.to,
                'acl': self.acl
            }
        }

    def __eq__(self, other):
        return self.name == other.name \
               and self.from_ == other.from_ \
               and self.to == other.to

    def __str__(self):
        return '{}[{}..{}]'.format(self.name, self.from_, self.to)


class HbfRanges(object):
    """Storage with mapping (name -> range (from, to))"""

    def __init__(self, db):
        self.db = db
        self.ranges = {}
        self.modified = False

        if self.db.version < '2.2.53':
            return

        self.DATA_FILE = os.path.join(self.db.PATH, 'hbfranges.json')
        with open(self.DATA_FILE, 'r') as rfile:
            for hbf_range_name, hbf_range in json.loads(rfile.read()).items():
                self.ranges[hbf_range_name] = HbfRange.from_json(hbf_range_name, hbf_range)

    def mark_as_modified(self):
        self.modified = True

    def to_json(self):
        result = {}
        for hbf_range in self.ranges.values():
            result.update(hbf_range.to_json())
        return result

    def update(self, smart=False):
        if self.db.version < '2.2.53':
            return

        if not self.modified:
            return

        result = self.to_json()
        with open(self.DATA_FILE, 'w') as wfile:
            json.dump(result, wfile, indent=4)

    def fast_check(self, timeout):
        pass

    def get_range_by_project_id(self, project_id):
        for hbf_range in self.ranges.values():
            if hbf_range.from_ <= project_id <= hbf_range.to:
                return hbf_range
        return None

    def has_range(self, hbf_range_name):
        return hbf_range_name in self.ranges

    def get_range_by_name(self, hbf_range_name):
        return self.ranges[hbf_range_name]

    def get_ranges(self):
        return self.ranges.values()

    def min_project_id(self):
        return min([x.from_ for x in self.ranges.values()])

    def max_project_id(self):
        return max([x.to for x in self.ranges.values()])

