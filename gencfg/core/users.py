"""Staff users as separate entity in gencfg"""


import os
import json
from collections import defaultdict

from core.card.node import CardNode, Scheme, load_card_node, save_card_node
from core.card.types import Date


class User(CardNode):
    """Staff user as gencfg entity"""

    def __init__(self, parent, name, retired_at=None, staff_group=None):
        """
            :param dpt: Depratnemt object
        """
        super(User, self).__init__()

        self.parent = parent

        self.name = name
        self.staff_group = staff_group
        if isinstance(retired_at, basestring):
            self.retired_at = Date.create_from_text(retired_at)
        else:
            self.retired_at = retired_at


class Users(object):
    def __init__(self, db):
        self.db = db

        self.users = dict()
        self.group_users = defaultdict(set)

        if self.db.version <= '2.2.38':
            user_names = sorted(set(sum([x.card.owners for x in self.db.groups.get_groups()], [])))
            user_names = [x for x in user_names if not x.startswith('yandex_')]
            for user_name in user_names:
                self.users[user_name] = User(self, user_name)
        elif self.db.version <= '2.2.47':
            self.SCHEME_FILE = os.path.join(self.db.SCHEMES_DIR, 'users.yaml')
            self.DATA_FILE = os.path.join(self.db.PATH, 'users.yaml')

            contents = load_card_node(self.DATA_FILE, schemefile=self.SCHEME_FILE, cacher=self.db.cacher)

            for value in contents:
                user = User(self, value.name, staff_group=getattr(value, 'staff_group', None), retired_at=value.retired_at)
                if user.name in self.users:
                    raise Exception('User <%s> mentioned twice in <%s>' % (user.name, self.DATA_FILE))
                self.users[user.name] = user
        else:
            self.DATA_FILE = os.path.join(self.db.PATH, 'users.json')
            jsoned = json.loads(open(self.DATA_FILE).read())
            for elem in jsoned:
                user = User(self, elem['name'], retired_at=elem['retired_at'], staff_group=elem['staff_group'])
                self.users[user.name] = user
                self.group_users[elem['staff_group']].add(user)

        self.modified = False


    def has_user(self, name):
        return name in self.users

    def get_user(self, name):
        return self.users[name]

    def get_users(self):
        return self.users.values()

    def get_group_users(self, group_name):
        return self.group_users[group_name]

    def get_groups_with_user(self, user):
        if isinstance(user, str):
            user = self.users[user]

        return [x for x in self.db.groups.get_groups() if user.name in x.card.owners]

    def add_user(self, user_name, staff_group=None, retired_at=None):
        user = User(self, user_name, staff_group=staff_group, retired_at=retired_at)
        self.users[user.name] = user
        self.group_users[user.staff_group].add(user)

        self.mark_as_modified()

        return user

    def remove_user(self, user, check=True):
        if isinstance(user, str):
            user = self.users[user]

        if check:
            groups_with_user = self.get_groups_with_user(user)
            if len(groups_with_user) > 0:
                raise Exception("Can not remove user <%s>, which is used by <%s>" % (
                    user.name, ",".join(map(lambda x: x.card.name, groups_with_user))))

        self.users.pop(user.name)
        self.group_users[user.staff_group].pop(user)

        self.mark_as_modified()

        return user

    def update(self, smart=False):
        if self.db.version <= '2.2.38':
            return
        if smart and not self.modified:
            return

        if self.db.version < '2.2.47':
            save_card_node(sorted(self.users.values(), cmp=lambda x, y: cmp(x.name, y.name)), self.DATA_FILE,
                           self.SCHEME_FILE)
        else:
            result = []
            for user in sorted(self.users.values(), key=lambda x: x.name):
                if user.retired_at is None:
                    retired_at = None
                else:
                    retired_at = str(user.retired_at)

                result.append(dict(name=user.name, staff_group=user.staff_group, retired_at=retired_at))

            with open(self.DATA_FILE, 'w') as f:
                f.write(json.dumps(result, indent=4, sort_keys=True))

    def fast_check(self, timeout):
        # nothing to check
        pass

    def mark_as_modified(self):
        self.modified = True

    def available_staff_users(self):
        return sorted(self.users.keys())
