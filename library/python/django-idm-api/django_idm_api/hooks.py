# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import warnings

from django.conf import settings
from django.db.models import Q, Max

from django_idm_api import validation
from django_idm_api.compat import get_user_model
from django_idm_api.constants import SUBJECT_TYPES
from django_idm_api.exceptions import RoleNotFound, UserNotFound, GroupNotFound, TvmAppNotFound, TvmAppGroupNotFound
from django_idm_api.signals import (
    role_added, role_removed,
    tvm_role_added, tvm_role_removed,
    membership_added, membership_removed,
    org_role_added, org_role_removed)
from django_idm_api.utils import with_environment_options, is_b2b

__all__ = ('BaseHooks', 'AuthHooks', 'RoleNotFound', 'UserNotFound', 'GroupNotFound')


class RoleStream(object):
    name = ""  # Должно быть всегда задано и уникально в рамках одного get_roles

    def __init__(self, hooks):
        self.hooks = hooks

    def get_queryset(self):
        """Результат должен быть отсортирован по pk."""
        return NotImplemented

    def values_list(self, queryset):
        """Первым значением каждого элемента должен быть pk."""
        return NotImplemented

    def row_as_dict(self, row):
        return NotImplemented


class SuperUsersStream(RoleStream):
    name = 'superusers'

    def get_queryset(self):
        return get_user_model().objects.filter(is_superuser=True).order_by('pk')

    def values_list(self, queryset):
        return queryset.values_list('pk', 'username')

    def row_as_dict(self, row):
        pk, username = row
        return {
            'login': username,
            'path': '/role/superuser/',
        }


class MembershipStream(RoleStream):
    name = 'memberships'

    def get_queryset(self):
        try:
            Membership = get_user_model()._meta.get_field('groups').remote_field.through
        except AttributeError:
            # fallback to Django < 2.0
            Membership = get_user_model()._meta.get_field('groups').rel.through
        return Membership.objects.filter(group_id__in=self.hooks.get_group_queryset().values('id')).order_by('pk')

    def values_list(self, queryset):
        return queryset.values_list('pk', 'user__username', 'group__pk')

    def row_as_dict(self, row):
        pk, username, group_pk = row
        return {
            'login': username,
            'path': '/role/group-%d/' % group_pk
        }


class Hooks(object):
    _successful_result = {'code': 0}

    # Set these options as you wish
    GET_ROLES_PAGE_SIZE = getattr(settings, 'ROLES_PAGE_SIZE', 100)
    GET_ROLES_STREAMS = []  # type: List[RoleStream]
    GET_ROLES_FORM_CLASS = validation.GetRolesForm

    def get_stream_by_name(self, name, streams):
        for i, stream_class in enumerate(streams):
            if stream_class.name == name:
                return i, stream_class(self)
        return -1, None

    def get_items(self, request, streams, form_class, item_type, limit):
        if not streams:
            return NotImplemented

        next_url = None
        form = form_class(request.GET, self)
        if not form.is_valid():
            form.raise_first()
        stream_name, since = form.get_clean_data()
        stream_num, stream = self.get_stream_by_name(stream_name, streams)

        qs = stream.get_queryset()
        maxpk = qs.aggregate(maxpk=Max('pk')).get('maxpk')
        qs = stream.values_list(qs)
        if since:
            qs = qs.filter(pk__gt=since)
        qs = qs[:limit]
        exhausted = maxpk is None
        items = []
        pk = None
        for row in qs:
            pk = row[0]
            if pk == maxpk:
                exhausted = True
            items.append(stream.row_as_dict(row))
        page = {
            'code': 0,
            item_type: items,
        }
        if exhausted:
            if stream_num < len(streams) - 1:
                next_url = '%s?type=%s' % (request.path, streams[stream_num + 1].name)
        else:
            next_url = '%s?type=%s&since=%s' % (request.path, stream.name, pk)
        if next_url:
            page['next-url'] = next_url
        return page

    def get_roles(self, request):
        """Этот метод можно определить изменением следующих полей:
        GET_ROLES_PAGE_SIZE, GET_ROLES_STREAMS, GET_ROLES_FORM_CLASS
        Список стримов должен быть непуст.
        """
        return self.get_items(
            request,
            streams=self.GET_ROLES_STREAMS,
            form_class=self.GET_ROLES_FORM_CLASS,
            item_type='roles',
            limit=self.GET_ROLES_PAGE_SIZE
        )

    def info(self):
        """Срабатывает, когда Управлятор спрашивает систему о том, какие роли она поддерживает."""
        values = self.info_impl()
        return {
            'code': 0,
            'roles': {
                'slug': 'role',
                'name': 'роль',
                'values': values
            }
        }

    def get_user_roles(self, login):
        """Вызывается, если Управлятор запрашивает все роли пользователя в системе.
        """
        return dict(
            self._successful_result,
            roles=self.get_user_roles_impl(login),
        )

    def get_all_roles(self):
        """Вызывается для аудита и первичной синхронизации. Deprecated метод.

        Отдает всех пользователей, которые известны в системе, и их роли.
        """
        return {
            'code': 0,
            'users': [
                {
                    'login': login,
                    'roles': roles
                } for login, roles in self.get_all_roles_impl()
            ]
        }

    # Абстрактные методы
    def info_impl(self, **kwargs):
        """Возвращает словарь, где ключом является slug роли, а значением
        либо строка с названием роли, либо словарь с расширенным описанием роли.
        """
        raise NotImplementedError()

    def get_user_roles_impl(self, login, **kwargs):
        """Возвращает список описаний-ролей пользователя с указанным логином.
        """
        raise NotImplementedError()

    def get_all_roles_impl(self, **kwargs):
        """Возвращает список пар (login, список описаний ролей,
        где каждое описание роли это словарь) всех пользователей.
        """
        raise NotImplementedError()


class IntranetHooksMixin(object):
    GET_MEMBERSHIP_PAGE_SIZE = getattr(settings, 'MEMBERSHIP_PAGE_SIZE', 100)
    GET_MEMBERSHIP_STREAMS = []  # type: List[RoleStream]
    GET_MEMBERSHIP_FORM_CLASS = validation.GetMembershipsForm

    def add_role(self, login, role, fields, subject_type):
        """Обрабатывает назначение новой роли пользователю с указанным логином."""
        if subject_type == SUBJECT_TYPES.TVM_APP:
            method = self.add_tvm_role_impl
            signal = tvm_role_added
        else:
            method = self.add_role_impl
            signal = role_added

        result = method(login, role, fields) or {}
        # Если мы здесь, то добавление роли произошло успешно
        signal.send(sender=self.__class__, login=login, role=role, fields=fields)
        return dict(self._successful_result, **result)

    def add_group_role(self, group, role, fields, subject_type):
        """Обрабатывает назначение новой роли группе с указанным ID."""
        if subject_type == SUBJECT_TYPES.TVM_APP:
            method = self.add_tvm_group_role_impl
            signal = tvm_role_added
        else:
            method = self.add_group_role_impl
            signal = role_added

        result = method(group, role, fields) or {}
        # Если мы здесь, то добавление роли произошло успешно
        signal.send(sender=self.__class__, group=group, role=role, fields=fields)
        return dict(self._successful_result, **result)

    def remove_role(self, login, role, data, is_fired, subject_type):
        """Вызывается, если у пользователя с указанным логином отзывают роль."""

        if subject_type == SUBJECT_TYPES.TVM_APP:
            method = self.remove_tvm_role_impl
            signal = tvm_role_removed
        else:
            method = self.remove_role_impl
            signal = role_removed

        try:
            method(login, role, data, is_fired)
        except (UserNotFound, TvmAppNotFound):
            # сотрудник не найден, но мы всё равно возвращаем OK,
            # потому что вдруг это пытаются отозвать доступ кого-то,
            # кого вынесли из админки руками
            pass
        except RoleNotFound:
            # роль неизвестна, но мы всё равно возвращаем OK,
            # потому что вдруг это пытаются отозвать какую-то
            # старую роль
            pass
        else:
            signal.send(sender=self.__class__, login=login, role=role, data=data, is_fired=is_fired)
        return self._successful_result

    def remove_group_role(self, group, role, data, is_deleted, subject_type):
        """Вызывается, если у группы с указанным ID отзывают роль."""
        if subject_type == SUBJECT_TYPES.TVM_APP:
            method = self.remove_tvm_group_role_impl
            signal = tvm_role_removed
        else:
            method = self.remove_group_role_impl
            signal = role_removed

        try:
            method(group, role, data, is_deleted)
        except (GroupNotFound, TvmAppGroupNotFound):
            # группа не найдена, но мы всё равно возвращаем OK,
            # потому что вдруг это пытаются отозвать доступ кого-то,
            # кого вынесли из админки руками
            pass
        except RoleNotFound:
            # роль неизвестна, но мы всё равно возвращаем OK,
            # потому что вдруг это пытаются отозвать какую-то
            # старую роль
            pass
        else:
            signal.send(sender=self.__class__, group=group, role=role, data=data, is_deleted=is_deleted)
        return self._successful_result

    def get_memberships(self, request):
        """Этот метод можно определить изменением следующих полей:
        GET_MEMBERSHIP_PAGE_SIZE, GET_MEMBERSHIP_STREAMS, GET_MEMBERSHIP_FORM_CLASS
        Список стримов должен быть непуст.
        """
        return self.get_items(
            request,
            streams=self.GET_MEMBERSHIP_STREAMS,
            form_class=self.GET_MEMBERSHIP_FORM_CLASS,
            item_type='memberships',
            limit=self.GET_MEMBERSHIP_PAGE_SIZE,
        )

    def add_membership(self, login, group_external_id, passport_login=''):
        """Обрабатывает назначение новой роли пользователю с указанным логином."""
        result = self.add_membership_impl(login, group_external_id, passport_login) or {}
        # Если мы здесь, то добавление участника группы произошло успешно
        membership_added.send(
            sender=self.__class__,
            login=login,
            group_external_id=group_external_id,
            passport_login=passport_login,
        )
        return dict(self._successful_result, **result)

    def remove_membership(self, login, group_external_id):
        """Вызывается, если у пользователя с указанным логином отзывают роль."""
        try:
            self.remove_membership_impl(login, group_external_id)
        except UserNotFound:
            # сотрудник не найден, но мы всё равно возвращаем OK,
            # потому что вдруг это пытаются отозвать членство кого-то,
            # кого убрали из админки руками
            pass
        else:
            membership_removed.send(
                sender=self.__class__,
                login=login,
                group_external_id=group_external_id,
            )
        return self._successful_result

    # Абстрактные методы
    def add_role_impl(self, login, role, fields, **kwargs):
        """Выдаёт пользователю с логином login роль role с дополнительными полями fields"""
        raise NotImplementedError()

    def add_tvm_role_impl(self, login, role, fields, **kwargs):
        """Выдаёт tvm-приложению с id login роль role с дополнительными полями fields"""
        raise NotImplementedError()

    def add_group_role_impl(self, group, role, fields, **kwargs):
        """Выдаёт группе с ID group роль role с дополнительными полями fields"""
        raise NotImplementedError()

    def add_tvm_group_role_impl(self, group, role, fields, **kwargs):
        """Выдаёт группе tvm-приложений с ID group роль role с дополнительными полями fields"""
        raise NotImplementedError()

    def remove_role_impl(self, login, role, data, is_fired, **kwargs):
        """Отбирает у пользователя login роль role с дополнительными данными data
        и флагом уволенности (is_fired)
            Может бросить UserNotFound и RoleNotFound
        """
        raise NotImplementedError()

    def remove_tvm_role_impl(self, login, role, data, is_fired, **kwargs):
        """Отбирает у tvm-приложения с id login роль role с дополнительными данными data
        и флагом уволенности (is_fired)
            Может бросить TvmAppNotFound и RoleNotFound
        """
        raise NotImplementedError()

    def remove_group_role_impl(self, group, role, data, is_deleted, **kwargs):
        """Отбирает у пользователя login роль role с дополнительными данными data
        и флагом уволенности (is_fired)
            Может бросить UserNotFound и RoleNotFound
        """
        raise NotImplementedError()

    def remove_tvm_group_role_impl(self, group, role, data, is_deleted, **kwargs):
        """Отбирает у группы tvm-приложений с ID group роль role с дополнительными данными data
        и флагом удаленности (is_deleted)
            Может бросить TvmAppGroupNotFound и RoleNotFound
        """
        raise NotImplementedError()

    def add_membership_impl(self, login, group_external_id, passport_login, **kwargs):
        """Добавляет пользователя login в группу с id на стаффе - group_external_id с паспортным логином passport_login
        Может бросить UserNotFound
        """
        raise NotImplementedError()

    def remove_membership_impl(self, login, group_external_id, **kwargs):
        """Исключает пользователя login с паспортным логином passport_login из группы с id в staff-api group_external_id
        Может бросить UserNotFound и RoleNotFound
        """
        raise NotImplementedError()


class B2BHooksMixin(object):
    def add_role(self, role, fields, subject_type, org_id, uid):
        """Обрабатывает назначение новой роли пользователю с указанным логином."""
        if subject_type == SUBJECT_TYPES.ORGANIZATION:
            method = self.add_org_role_impl
            signal = org_role_added
        else:
            method = self.add_role_impl
            signal = role_added

        result = method(uid, role, fields, org_id) or {}
        # Если мы здесь, то добавление роли произошло успешно
        signal.send(sender=type(self), uid=uid, role=role, fields=fields, org_id=org_id)
        return dict(self._successful_result, **result)

    def add_group_role(self, group, role, fields, subject_type, org_id):
        """Обрабатывает назначение новой роли группе с указанным ID."""
        result = self.add_group_role_impl(group, role, fields, org_id) or {}
        # Если мы здесь, то добавление роли произошло успешно
        role_added.send(sender=self.__class__, group=group, role=role, fields=fields, org_id=org_id)
        return dict(self._successful_result, **result)

    def remove_role(self, role, data, is_fired, subject_type, org_id, uid):
        """Вызывается, если у пользователя с указанным логином отзывают роль."""

        if subject_type == SUBJECT_TYPES.ORGANIZATION:
            method = self.remove_org_role_impl
            signal = org_role_removed
        else:
            method = self.remove_role_impl
            signal = role_removed

        try:
            method(uid, role, data, is_fired, org_id)
        except (UserNotFound, TvmAppNotFound):
            # сотрудник не найден, но мы всё равно возвращаем OK,
            # потому что вдруг это пытаются отозвать доступ кого-то,
            # кого вынесли из админки руками
            pass
        except RoleNotFound:
            # роль неизвестна, но мы всё равно возвращаем OK,
            # потому что вдруг это пытаются отозвать какую-то
            # старую роль
            pass
        else:
            signal.send(sender=type(self), uid=uid, role=role, data=data, is_fired=is_fired, org_id=org_id)
        return self._successful_result

    def remove_group_role(self, group, role, data, is_deleted, subject_type, org_id):
        """Вызывается, если у группы с указанным ID отзывают роль."""
        try:
            self.remove_group_role_impl(group, role, data, is_deleted, org_id)
        except (GroupNotFound, TvmAppGroupNotFound):
            # группа не найдена, но мы всё равно возвращаем OK,
            # потому что вдруг это пытаются отозвать доступ кого-то,
            # кого вынесли из админки руками
            pass
        except RoleNotFound:
            # роль неизвестна, но мы всё равно возвращаем OK,
            # потому что вдруг это пытаются отозвать какую-то
            # старую роль
            pass
        else:
            role_removed.send(sender=self.__class__, group=group, role=role, data=data, is_deleted=is_deleted, org_id=org_id)
        return self._successful_result

    # Абстрактные методы
    def add_role_impl(self, uid, role, fields, org_id, **kwargs):
        """Выдаёт пользователю с логином login роль role с дополнительными полями fields"""
        raise NotImplementedError()

    def add_org_role_impl(self, uid, role, fields, org_id, **kwargs):
        """Выдаёт организации роль role с дополнительными полями fields"""
        raise NotImplementedError()

    def add_group_role_impl(self, group, role, fields, org_id, **kwargs):
        """Выдаёт группе с ID group роль role с дополнительными полями fields"""
        raise NotImplementedError()

    def remove_role_impl(self, uid, role, data, is_fired, org_id, **kwargs):
        """Отбирает у пользователя login роль role с дополнительными данными data
        и флагом уволенности (is_fired)
            Может бросить UserNotFound и RoleNotFound
        """
        raise NotImplementedError()

    def remove_org_role_impl(self, uid, role, data, is_fired, org_id, **kwargs):
        """Отбирает у организации роль role с дополнительными данными data
        и флагом уволенности (is_fired)
            Может бросить UserNotFound и RoleNotFound
        """
        raise NotImplementedError()

    def remove_group_role_impl(self, group, role, data, is_deleted, org_id, **kwargs):
        """Отбирает у пользователя login роль role с дополнительными данными data
        и флагом уволенности (is_fired)
            Может бросить UserNotFound и RoleNotFound
        """
        raise NotImplementedError()


class BaseHooks(Hooks, with_environment_options(intranet=IntranetHooksMixin, b2b=B2BHooksMixin)):
    pass


class AuthHooks(BaseHooks):
    """Реализация хуков поверх django.contrib.auth
    работает только в интранете
    """
    GET_ROLES_STREAMS = [SuperUsersStream, MembershipStream]

    def __init__(self, *args, **kwargs):
        if is_b2b():
            raise NotImplementedError('AuthHooks implemented only in intranet')
        super(AuthHooks, self).__init__(*args, **kwargs)

    def info_impl(self, **kwargs):
        roles = {'superuser': 'суперпользователь'}
        roles.update(('group-%s' % group.pk, group.name.lower())
                     for group in self.get_group_queryset())
        return roles

    def add_role_impl(self, login, role, fields, **kwargs):
        role = role.get('role')
        if not role:
            raise RoleNotFound('Role is not not provided')

        user = self._get_or_create_user(login)

        if role == 'superuser':
            user.is_staff = True
            user.is_superuser = True
            user.save(update_fields=('is_staff', 'is_superuser'))
        elif role.startswith('group-'):
            group = self._get_group(role[6:])
            user.is_staff = True
            user.groups.add(group)
            user.save(update_fields=('is_staff', 'is_superuser'))
        else:
            raise RoleNotFound('Role does not exist: %s' % role)

    def remove_role_impl(self, login, role, data, is_fired, **kwargs):
        role = role.get('role')
        if not role:
            raise RoleNotFound('Role is not not provided')

        try:
            user = self._get_user(login)
        except UserNotFound:
            raise

        if role == 'superuser':
            user.is_superuser = False
            self._remove_is_staff_if_needed(user)
            user.save(update_fields=('is_staff', 'is_superuser'))
        elif role.startswith('group-'):
            group = self._get_group(role[6:])

            user.groups.remove(group)
            self._remove_is_staff_if_needed(user)
            user.save(update_fields=('is_staff', 'is_superuser'))
        else:
            raise RoleNotFound('Role does not exist: %s' % role)

    def get_user_roles_impl(self, login, **kwargs):
        warnings.warn('Method `get_user_roles_impl` is deprecated and will be removed soon.')
        user = self._get_user(login)
        roles = []

        for group in self.get_group_queryset().filter(user=user):
            roles.append('group-%s' % group.id)

        if user.is_superuser:
            roles.append('superuser')
        return [{'role': role} for role in roles]
    get_user_roles_impl.is_deprecated = True

    def get_all_roles_impl(self, **kwargs):
        users = (
            get_user_model().objects.
            filter(Q(groups__id__in=self.get_group_queryset().values('id')) | Q(is_superuser=True)).
            prefetch_related('groups').
            order_by('username').
            distinct()
        )
        all_roles = []
        for user in users:
            if not hasattr(self.get_user_roles_impl, 'is_deprecated'):
                # fallback для обратной совместимости
                roles = self.get_user_roles_impl(user.username)
            else:
                roles = []
                if user.is_superuser:
                    roles.append({'role': 'superuser'})
                for group in user.groups.all():
                    roles.append({'role': 'group-%s' % group.id})
            all_roles.append((user.username, roles))
        return all_roles

    # Вспомогательные методы
    def _remove_is_staff_if_needed(self, user):
        if not user.is_superuser and user.groups.count() == 0:
            user.is_staff = False

    def _get_user(self, login):
        """Получить пользователя по логину или кинуть эксепшн."""
        try:
            return get_user_model().objects.get(username=login)
        except get_user_model().DoesNotExist:
            raise UserNotFound('User does not exist: %s' % login)

    def _get_or_create_user(self, login):
        """Получить существующего пользователя по логину или создать нового."""
        try:
            user = self._get_user(login)
        except UserNotFound:
            user = get_user_model().objects.create(username=login)
        return user

    def _get_group(self, group_id):
        """Получить группу по id или кинуть эксепшн."""
        from django.contrib.auth.models import Group

        try:
            return self.get_group_queryset().get(pk=int(group_id))
        except Group.DoesNotExist:
            raise RoleNotFound('Group does not exist: %s' % group_id)

    def get_group_queryset(self):
        from django.contrib.auth.models import Group

        return Group.objects.all()
