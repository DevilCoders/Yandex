import operator

from cachetools import TTLCache, cachedmethod


class User:
    def __init__(self, login, roles):
        self.login = login
        self.roles = roles

    def is_anonymous(self):
        return self.login is None

    def has_role(self, role):
        return role in self.roles


class Role:
    def __init__(self, name, role_config):
        self.name = name

        self._staff_groups = set()
        if 'staff_groups' in role_config:
            self._staff_groups.update(role_config['staff_groups'])

    def has_role(self, groups):
        return any((g['group']['type'] == 'servicerole' or g['group']['type'] == 'wiki') and g['group']['url'] in self._staff_groups for g in groups)


class Authenticator:
    def __init__(self, staff, config):
        self._staff = staff
        self._cache = TTLCache(maxsize=config['cache']['max_size'],
                               ttl=config['cache']['ttl'])

        self._roles = []
        for role in config:
            self._roles.append(Role(role, config[role]))

    @cachedmethod(operator.attrgetter('_cache'))
    def authenticate(self, tg_username):
        user = self._staff.get_account(tg_username)
        if user is None:
            return User()

        user_roles = set()
        for role in self._roles:
            if role.has_role(user['groups']):
                user_roles.add(role.name)

        return User(login=user['login'], roles=user_roles)
