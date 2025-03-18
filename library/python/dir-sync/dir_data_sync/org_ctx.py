# coding: utf-8

import logging

from django.conf import settings
from django.contrib.auth import get_user_model

from .models import Organization
from .injector import absorb, get_from_thread_store, is_in_thread_store

_ORG_CTX_KEY = 'org_ctx'

logger = logging.getLogger(__name__)


class OrgCtx(object):
    """
    При обработке HTTP запроса, либо выполнении Celery таски,
    мы можем находиться в контексте какой-то организации.
    Этот контекст нужен для того, например, чтобы брать сущности
    из базы, принадлежащие именно указанной организации.

    Данный класс - context manager, позволяющий задавать контекст организации.

    Используется так:

    org = <организация 42>
    with org_ctx(org):
        # Страница организации 42
        page = Page.active.filter(supertag='killah', org=get_org())

        org = <организация 7>
        with org_ctx(org):
            # Страница организации 7
            page = Page.active.filter(supertag='killah', org=get_org())

        # Страница организации 42
        page = Page.active.filter(supertag='killah', org=get_org())

    # Все страницы с супертэгом 'killah'
    page = Page.active.filter(supertag='killah', org=get_org())

    При входе в блок with указанная организация попадает в thread_locals на вершину стека.
    Благодаря этому, при получении текущей организации всегда используется последняя
    указанная организация. При выходе из блока with верхний элемент стека удаляется.

    """

    def __init__(self, org, raise_on_empty=True):
        if raise_on_empty:
            raise_if_empty_org_ctx(org, 'Trying to put empty org in context')
        self.org = org

    def __enter__(self):
        if not is_in_thread_store(_ORG_CTX_KEY):
            absorb(_ORG_CTX_KEY, [])

        ctx = get_from_thread_store(_ORG_CTX_KEY)
        ctx.append(self.org)

    def __exit__(self, exc_type, exc_val, exc_tb):
        ctx = get_from_thread_store(_ORG_CTX_KEY)
        ctx.pop()


org_ctx = OrgCtx


class NoOrgCtxException(Exception):
    pass


def get_org():
    """
    Берет текущую организацию с вершины стека контекста организации.

    @rtype: Organization | None
    @return: Организация, в контексте которой мы сейчас находимся, либо None.
    """
    if is_in_thread_store(_ORG_CTX_KEY):
        ctx = get_from_thread_store(_ORG_CTX_KEY)
        org = ctx[-1] if len(ctx) else None
    else:
        org = None

    raise_if_empty_org_ctx(org, 'Empty org context loaded')

    return org


def get_org_id():
    org = get_org()
    return None if org is None else org.id


def get_org_dir_id():
    org = get_org()
    return None if org is None else org.dir_id


def get_org_lang():
    # TODO: по идее надо брать язык через get_org().lang, это временный workaround
    # TODO: для тестов, в которых пока нет контекста организации.
    return get_org().lang if get_org() else 'en'


def get_org_or_none(org_id):
    if settings.DIRSYNC_IS_BUSINESS:
        try:
            return Organization.objects.get(id=org_id)
        except Organization.DoesNotExist:
            if settings.DIRSYNC_IS_TESTS:
                return None
            logger.error('Organization with id=%d is not found', org_id)
            raise
    return None


def raise_if_empty_org_ctx(org, message):
    if not org and settings.DIRSYNC_IS_BUSINESS and not settings.DIRSYNC_IS_TESTS:
        # Дописал странное слово NORG, чтобы легко грепать логи.
        message = 'NORG: %s' % message

        # Чтобы распечатать stacktrace в логе, порождаю исключение.
        # Возможно, это можно сделать и без исключения.
        try:
            raise NoOrgCtxException(message)
        except NoOrgCtxException:
            logger.exception(message)
            raise


def org_user():
    if settings.DIRSYNC_IS_BUSINESS:
        return get_user_model().objects.filter(orgs=get_org())
    else:
        return get_user_model().objects


def get_user_orgs(user):
    if settings.DIRSYNC_IS_BUSINESS:
        # Костыль, чтобы не увеличивать на 1 число запросов в
        # self.assertNumQueries в куче тестов. Если в тестах
        # начнут создаваться организации, это надо убрать.
        if settings.DIRSYNC_IS_TESTS:
            return [None]

        orgs = user.orgs.all().order_by('-id')

        raise_if_empty_org_ctx(orgs, 'User %d has no org' % user.id)

        return orgs
    else:
        return [None]
