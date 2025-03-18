"""Staff groups as cached in gencfg (RX-474)"""

import copy
import json
import os
from collections import defaultdict


class TStaffGroups(object):
    """Storage of staff groups"""

    class TStaffGroup(object):
        """Staff group"""

        def __init__(self, parent, jsoned):
            self.parent = parent
            self.from_json(jsoned)

        @property
        def children(self):
            """Child groups"""
            return self._children()

        def _children(self):
            return copy.copy(self.parent.group_children[self.name])

        @property
        def recurse_children(self):
            """Child groups (all subtree)"""
            return self._recurse_children()

        def _recurse_children(self):
            result = []
            for child in self.children:
                result.append(child)
                result.extend(child._recurse_children())
            return result

        @property
        def users(self):
            """Group users"""
            return self._users()

        def _users(self):
            result = [x for x in self.parent.db.users.get_group_users(self.name) if x.retired_at is None]
            result.sort(key=lambda x: x.name)
            return result

        @property
        def recurse_users(self):
            """Group users (with all subgroups)"""
            result = []
            for staff_group in self.recurse_children + [self]:
                result.extend(staff_group.users)

            result = list(set(result))
            result.sort(key=lambda x: x.name)

            return result

        def from_json(self, jsoned):
            self.name = jsoned['name']
            self.parent_group = jsoned['parent_group']

        def to_json(self):
            return dict(
                name=self.name,
                parent_group=self.parent_group,
            )

    def __init__(self, db):
        self.db = db
        self.modified = False
        self.DATA_FILE = os.path.join(self.db.PATH, 'staffgroups.json')

        self.groups = {}

        # fill data
        if self.db.version >= '2.2.48':
            jsoned = json.loads(open(self.DATA_FILE).read())
            for elem in jsoned:
                self.groups[elem['name']] = TStaffGroups.TStaffGroup(self, elem)

        # calc caches
        self.calc_cache()

    def calc_cache(self):
        self.group_children = defaultdict(list)
        for group in self.groups.itervalues():
            if group.parent_group is not None:
                self.group_children[group.parent_group].append(group)

    def has(self, name):
        return name in self.groups

    def get(self, name):
        return self.groups[name]

    def get_all(self):
        return self.groups.values()

    def add(self, name, parent_group=None):
        new_group = TStaffGroups.TStaffGroup(self, dict(name=name, parent_group=parent_group))
        self.groups[new_group.name] = new_group

        if new_group.parent_group is not None:
            self.group_children[new_group.parent_group].append(new_group)

        return new_group

    def remove(self, name):
        raise Exception('Not implemented')

    def purge(self):
        self.groups = {}
        self.calc_cache()

    def update(self, smart=True):
        result = [self.groups[x].to_json() for x in sorted(self.groups.keys())]

        with open(self.DATA_FILE, 'w') as f:
            f.write(json.dumps(result, indent=4))

    def fast_check(self, timeout):
        pass

    def available_staff_groups(self):
        return sorted(self.groups.keys())

