"""Abc groups as cached in gencfg (RX-447)"""

import copy
import json
import os


class TAbcGroups(object):
    """Storage of abc groups"""

    class TAbcRole(object):
        """Single abc role"""

        def __init__(self, parent, jsoned):
            self.parent = parent
            self.from_json(jsoned)

        def from_json(self, jsoned):
            self.id = jsoned['id']
            self.name = jsoned['name']

        def to_json(self):
            return {
                'id': self.id,
                'name': self.name,
            }

    class TAbcGroup(object):
        """Single abc group"""

        def __init__(self, parent, jsoned):
            """Initialize from json (as stored in database"""
            self.parent = parent
            self.from_json(jsoned)

        def from_json(self, jsoned):
            self.id = jsoned['id']
            self.name = jsoned['name']
            self.parent_group_id = jsoned['parent_group_id']
            self.roles = {x['id']: x['users'] for x in jsoned['roles']}

        def to_json(self):
            return {
                'id': self.id,
                'name': self.name,
                'parent_group_id': self.parent_group_id,
                'roles': [dict(id=x, users=self.roles[x]) for x in sorted(self.roles)],
            }

    def __init__(self, db):
        self.db = db
        self.modified = False
        self.DATA_FILE = os.path.join(self.db.PATH, 'abcgroups.json')

        if self.db.version < '2.2.56':
            # data structures
            self.abc_groups = []
            self.abc_roles = []

            # fill data
            if self.db.version >= '2.2.49':
                jsoned = json.loads(open(self.DATA_FILE).read())
                for elem in jsoned['groups']:
                    self.abc_groups.append(TAbcGroups.TAbcGroup(self, elem))
                for elem in jsoned['roles']:
                    self.abc_roles.append(TAbcGroups.TAbcRole(self, elem))
        else:
            with open(self.DATA_FILE) as rfile:
                data = json.load(rfile)
            self.abc_services = data['services']
            self.abc_scopes = data['scopes']

        # fill caches
        self.calc_cache()

    def get_service_scope_slug(self, formated_string):
        """
        Parser fromated strings to service slug and scope slug
        1) abc:service_slug:scope_slug
        2) abc:service_slug:role_name=scope_slug
        3) abc:service_slug:role_id=scope_id
        4) abc:service_slug:role=scope_slug_or_id
        5) abc:service_slug_scope_slug
        6) abc:service_slug
        """
        formated_string = formated_string.lower()
        if not formated_string.startswith('abc:'):
            return formated_string, None
        formated_string = formated_string[4:]

        if ':' in formated_string:  # 1, 2, 3, 4 cases
            service_slug, scope_slug = formated_string.split(':')  # 1 case
            if scope_slug.startswith('role') and '=' in scope_slug:  # 2, 3, 4 cases
                scope = scope_slug.split('=')[-1]
                scope_slug = None

                for abc_scope_slug, abc_scope_id in self.abc_scopes.items():
                    if scope == abc_scope_slug or scope == str(abc_scope_id):
                        scope_slug = abc_scope_slug
                        break
        elif '_' in formated_string:  # 5, 6 cases
            service_slug_parts = formated_string.split('_')

            service_slug = None
            scope_slug = None
            for i in xrange(len(service_slug_parts)):
                service_slug = '_'.join(service_slug_parts[:i+1])
                scope_slug = '_'.join(service_slug_parts[i+1:]) or None

                if self.has_service(service_slug) and self.has_scope(scope_slug):
                    break

        else:  # 6 case
            service_slug, scope_slug = formated_string, None  # 6 case

        if not self.has_service(service_slug):
            raise ValueError('Unknown abc service {}'.format(service_slug))

        if service_slug != formated_string and not self.has_scope(scope_slug):
            raise ValueError('Unknown abc scope {}'.format(scope_slug))

        return service_slug, scope_slug

    def get_service(self, service_slug):
        return self.abc_services[service_slug]

    def get_scope(self, scope_slug):
        return {'id': self.abc_scopes[scope_slug], 'slug': scope_slug}

    def get_service_members(self, service_slug, scope_slug=None):
        members = []
        if scope_slug is None:
            all_service_members = []
            for scope_members in self.abc_services[service_slug]['scopes'].values():
                all_service_members += scope_members
            return all_service_members
        elif self.has_scope(scope_slug):
            return self.abc_services[service_slug]['scopes'].get(scope_slug, [])
        raise ValueError('Unknown abc scope {}'.format(scope_slug))

    def add_service(self, service_slug, service_id, service_parent_id):
        if self.has_service(service_slug):
            raise ValueError('Abc service {} already exists'.format(service_slug))

        self.abc_services[service_slug] = {
            'slug': service_slug,
            'id': int(service_id),
            'parent_service_id': int(service_parent_id) if service_parent_id else None,
            'scopes': {}
        }

    def add_scope(self, scope_slug, scope_id):
        if self.has_scope(scope_slug):
            raise ValueError('Abc scope {} already exists'.format(scope_slug))
        if self.has_scope(scope_slug, scope_id=scope_id):
            raise ValueError('Abc scope {} already exists'.format(scope_id))

        self.abc_scopes[scope_slug] = scope_id

    def has_service(self, service_slug):
        return service_slug in self.abc_services

    def has_scope(self, scope_slug, scope_id=None):
        if scope_id is None:
            return scope_slug in self.abc_scopes
        return scope_slug in self.abc_scopes or scope_id in self.abc_scopes.values()

    def has_service_slug(self, formated_string):
        try:
            service_slug, scope_slug = self.get_service_scope_slug(formated_string)
            if scope_slug:
                return self.has_service(service_slug) and self.has_scope(scope_slug)
            return self.has_service(service_slug)
        except ValueError as e:
            return False

    def add_scope_to_service(self, service_slug, scope_slug, members=None):
        if not self.has_service(service_slug):
            raise ValueError('Unknown abc service {}'.format(service_slug))
        if not self.has_scope(scope_slug):
            raise ValueError('Unknown abc scope {}'.format(scope_slug))

        self.abc_services[service_slug]['scopes'][scope_slug] = members or []

    def add_member_to_service_scope(self, service_slug, scope_slug, member):
        if not self.has_service(service_slug):
            raise ValueError('Unknown abc service {}'.format(service_slug))
        if not self.has_scope(scope_slug):
            raise ValueError('Unknown abc scope {}'.format(scope_slug))

        if scope_slug not in self.abc_services[service_slug]['scopes']:
            self.abc_services[service_slug]['scopes'][scope_slug] = []
        self.abc_services[service_slug]['scopes'][scope_slug].append(member)

    def available_abc_services(self, disable_deprecated=False, only_services=False):
        available = set()

        for service_slug, service in self.abc_services.items():
            available.add('abc:{}'.format(service_slug))

            if only_services:
                continue

            for scope_slug, scope_id in self.abc_scopes.items():
                available.add('abc:{}:{}'.format(service_slug, scope_slug))

                if not disable_deprecated:
                    available.add('abc:{}:role={}'.format(service_slug, scope_slug))
                    available.add('abc:{}:role={}'.format(service_slug, scope_id))
                    available.add('abc:{}:role_id={}'.format(service_slug, scope_id))
                    available.add('abc:{}:role_name={}'.format(service_slug, scope_slug))

        return available

    def mark_as_modified(self):
        self.modified = True

    def calc_cache(self):
        if self.db.version < '2.2.56':
            # cache by id
            self.abc_group_by_id = dict()
            for abc_group in self.abc_groups:
                self.abc_group_by_id[abc_group.id] = abc_group

            # cache by name
            self.abc_group_by_name = dict()
            for abc_group in self.abc_groups:
                self.abc_group_by_name[abc_group.name] = abc_group

            # cache role by id
            self.abc_role_by_id = dict()
            for abc_role in self.abc_roles:
                self.abc_role_by_id[abc_role.id] = abc_role

            # cache role by name
            self.abc_role_by_name = dict()
            for abc_role in self.abc_roles:
                self.abc_role_by_name[abc_role.name] = abc_role

    # macroses methods
    def has(self, name_or_id):
        if self.db.version < '2.2.56':
            if isinstance(name_or_id, int):
                return name_or_id in self.abc_group_by_id
            elif isinstance(name_or_id, basestring):
                return name_or_id in self.abc_group_by_name
            else:
                raise Exception('Do not know how to convert <{}> to abc group'.format(name_or_id))
        raise Exception('Not implemented in DB >= 2.2.56')

    def get(self, name_or_id):
        if self.db.version < '2.2.56':
            if isinstance(name_or_id, int):
                return self.abc_group_by_id[name_or_id]
            elif isinstance(name_or_id, basestring):
                return self.abc_group_by_name[name_or_id]
            else:
                raise Exception('Do not know how to convert <{}> to abc group'.format(name_or_id))
        raise Exception('Not implemented in DB >= 2.2.56')

    def get_all(self):
        if self.db.version < '2.2.56':
            return self.abc_groups
        raise Exception('Not implemented in DB >= 2.2.56')

    def add(self, id_, name, parent_group_id, roles):
        if self.db.version < '2.2.56':
            abc_group = TAbcGroups.TAbcGroup(self, dict(name=name, id=id_, parent_group_id=parent_group_id, roles=roles))
            self.abc_groups.append(abc_group)

            # update caches
            self.abc_group_by_id[abc_group.id] = abc_group
            self.abc_group_by_name[abc_group.name] = abc_group
        raise Exception('Not implemented in DB >= 2.2.56')

    def remove(self, abc_group):
        raise Exception('Not implemented')

    # roles methods
    def has_role(self, name_or_id):
        if self.db.version < '2.2.56':
            if isinstance(name_or_id, int):
                return name_or_id in self.abc_role_by_id
            elif isinstance(name_or_id, basestring):
                return name_or_id in self.abc_role_by_name
            else:
                raise Exception('Do not know how to convert <{}> to abc group'.format(name_or_id))
        raise Exception('Not implemented in DB >= 2.2.56')

    def get_role(self, name_or_id):
        if self.db.version < '2.2.56':
            if isinstance(name_or_id, int):
                return self.abc_role_by_id[name_or_id]
            elif isinstance(name_or_id, basestring):
                return self.abc_role_by_name[name_or_id]
            else:
                raise Exception('Do not know how to convert <{}> to abc role'.format(name_or_id))
        raise Exception('Not implemented in DB >= 2.2.56')

    def get_all_roles():
        if self.db.version < '2.2.56':
            return self.abc_roles
        raise Exception('Not implemented in DB >= 2.2.56')

    def add_role(self, id_, name):
        if self.db.version < '2.2.56':
            abc_role = TAbcGroups.TAbcRole(self, dict(id=id_, name=name))
            self.abc_roles.append(abc_role)

            # update caches
            self.abc_role_by_id[abc_role.id] = abc_role
            self.abc_role_by_name[abc_role.name] = abc_role
        raise Exception('Not implemented in DB >= 2.2.56')

    def remove_role(self, abc_role):
        raise Exception('Not implemented')

    def purge(self):
        if self.db.version < '2.2.56':
            self.abc_groups = []
            self.abc_roles = []
            self.calc_cache()
        else:
            self.abc_services = {}
            self.abc_scopes = {}

    def update(self, smart=False):
        if smart and not self.modified:
            return

        if self.db.version < '2.2.56':
            result = dict(
                groups=[x.to_json() for x in self.abc_groups],
                roles=[x.to_json() for x in self.abc_roles],
            )
        else:
            result = {
                'services': self.abc_services,
                'scopes': self.abc_scopes
            }

        with open(self.DATA_FILE, 'w') as f:
            f.write(json.dumps(result, indent=4, sort_keys=True))

    def fast_check(self, timeout):
        pass
