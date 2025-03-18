from sqlalchemy.orm.exc import NoResultFound
from django.conf import settings

from django_idm_api.hooks import BaseHooks
from django_idm_api.exceptions import RoleNotFound, UserNotFound

from antirobot.cbb.cbb_django.cbb.models import UserRole
from antirobot.cbb.cbb_django.cbb.library import db


class Hooks(BaseHooks):
    def info_impl(self):
        """Возврашает список пар (id-роли, имя-роли)
        """
        roles = {}
        for role, value in settings.ROLES.items():
            roles[role] = value.get("name", role)
        return roles

    @db.main.use_master(commit_on_exit=True)
    def add_role_impl(self, login, role, fields):
        """Дает пользователю (login) роль (role)
        """
        role = role.get("role")
        if role not in settings.ROLES:
            raise RoleNotFound(u"Нет роли %s" % role)
        user_role = UserRole.query.get(login) or UserRole(login=login, role=role)
        user_role.role = role
        db.main.session.add(user_role)

    @db.main.use_master(commit_on_exit=True)
    def remove_role_impl(self, login, role, data, is_fired):
        """Отбирает у пользователя (login) роль (role)
        Может бросить UserNotFound и RoleNotFound
        """
        role = role.get("role")
        if role not in settings.ROLES:
            raise RoleNotFound(u"Нет роли %s" % role)
        user_role = UserRole.query.get(login)
        if not user_role:
            raise UserNotFound(f"Пользователь с логином {login} не найден.")
        if user_role.role == role:
            db.main.session.delete(user_role)

    @db.main.use_slave()
    def get_user_roles_impl(self, login):
        """Возвращает список описаний-ролей пользователя с указанным логином.
        """
        try:
            user_role = UserRole.query.filter_by(login=login).one()
        except NoResultFound:
            raise UserNotFound(f"Пользователь с логином {login} не найден.")

        return [{"role": user_role.role}]

    @db.main.use_slave()
    def get_all_roles_impl(self):
        """Возвращает список пар (login, список описаний ролей,
        где каждое описание роли это словарь) всех пользователей.
        """
        return [(user_role.login, [{"role": user_role.role}])
                for user_role in UserRole.query.all()]
