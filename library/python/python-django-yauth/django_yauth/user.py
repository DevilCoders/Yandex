# coding: utf-8

import weakref
import logging

import six
from django.conf import settings
from django.core.exceptions import ImproperlyConfigured, PermissionDenied

import blackbox

from .exceptions import (
    NoAuthMechanismError,
)

try:
    from django.contrib.auth import get_user_model
except ImportError:  # Django < 1.5
    from django.contrib.auth.models import User as _User

    _User.USERNAME_FIELD = 'username'

    def get_user_model():
        return _User

from django_yauth.util import get_setting, parse_time


def get_blackbox():
    return getattr(settings, 'YAUTH_BLACKBOX_INSTANCE') or blackbox.XmlBlackbox(url=blackbox.BLACKBOX_URL)


log = logging.getLogger(__name__)


class BaseYandexUser(object):
    authenticated_by = None

    def is_authenticated(self):
        raise NotImplementedError

    def needs_new_sessguard(self):
        raise NotImplementedError

    def is_lite(self):
        raise NotImplementedError

    def is_social(self):
        raise NotImplementedError

    def is_secure(self):
        raise NotImplementedError

    def is_yandexoid(self):
        raise NotImplementedError

    def get_username(self):
        raise NotImplementedError

    def __unicode__(self):
        raise NotImplementedError

    def has_perm(self, perm, obj=None):
        from django.contrib.auth.models import _user_has_perm

        return _user_has_perm(self, perm, obj)

    def has_perms(self, perm_list, obj=None):
        return all(self.has_perm(perm, obj) for perm in perm_list)


class YandexUser(BaseYandexUser):
    '''
    Модель яндексового пользователя.
    '''

    def __init__(self, uid, is_lite=False, fields=None, need_reset=False, emails=None, default_email=None,
                 new_session=None, oauth=None, auth_secure=False, new_sslsession=None, ticket=None,
                 blackbox_result=None, mechanism=None, service_ticket=None, user_ticket=None,
                 raw_service_ticket=None, raw_user_ticket=None, user_ip=None, attributes=None,
                 ):
        """
        @param uid: UID пользователя
        @param is_lite: признак лайт-авторизации
        @param fields: значения запрошенных полей базы данных Паспорта
        @param need_reset: признак, соответствующий статусу авторизации NEED_RESET
        @param emails: cписок электронных адресов пользователя
        @param default_email: электронный адрес пользователя по умолчанию
        @param new_session: значение обновленной куки Session_id
        @param oauth: словарь со значениями из ответа блэкбокса на запрос проверки токена
        @param auth_secure: gризнак дополнительной защиты аутентификации с помощью второй куки sessionid2
        @param new_sslsession: значение обновленной куки sessionid2
        @param ticket: tvm-тикет
        @param blackbox_result: ответ от блэкбокса целиком
        @param service_ticket: распрарсенный сервисный tvm-ticket
        @param user_ticket: распрарсенный персонализированный tvm-ticket
        @param raw_user_ticket: персонализированный tvm-ticket
        @param raw_service_ticket: сервисный tvm-ticket
        @param user_ip: user ip from request
        @param attributes: значения запрошенных атрибутов Паспорта
        """
        self.uid = uid
        self.is_lite_uid = is_lite
        self.fields = fields or {}
        self.attributes = attributes or {}
        self.need_reset = need_reset

        self.emails = emails or []
        self.default_email = default_email

        self.new_session = new_session

        self.oauth = oauth
        self.auth_secure = auth_secure
        self.new_sslsession = new_sslsession

        self.ticket = ticket
        self.service_ticket = service_ticket
        self.user_ticket = user_ticket

        self.raw_service_ticket = raw_service_ticket
        self.raw_user_ticket = raw_user_ticket

        self.blackbox_result = blackbox_result

        self.authenticated_by = mechanism

        self.user_ip = user_ip

    def is_authenticated(self):
        return not self.is_lite()

    def needs_new_sessguard(self):
        return False

    def is_lite(self):
        return bool(self.is_lite_uid)

    def is_social(self):
        return bool(self.social)

    def is_secure(self):
        """
        Признак дополнительной защиты аутентификации с помощью второй куки sessionid2.

        @return False, если вторая кука не передавалась, не соответствует переданной куке Session_id или невалидна,
                True, если переданная вторая кука валидна и соответствует куке Session_id
        """
        return self.auth_secure

    def is_yandexoid(self):
        """
        Признак внешнего аккаунта, привязанного к аккаунту на Стаффе.
        """
        return self.get_alias(id_=PassportAlias.YANDEXOID) is not None

    def get_alias(self, id_, default=None):
        value = default
        for alias_id, alias_value in self.fields.get('aliases', []):
            if alias_id == id_:
                value = alias_value
                break
        return value

    def get_username(self):
        return getattr(self, settings.YAUTH_YAUSER_USERNAME_FIELD)

    def __getattr__(self, field):
        try:
            return self.fields[field]
        except KeyError:
            raise AttributeError(field)

    def __unicode__(self):
        return self.display_name


class AnonymousYandexUser(BaseYandexUser):
    '''
    Заглушка незалогиненного пользователя.
    '''
    login = display_name = '(Not logged in)'
    uid = None
    need_reset = False

    emails = []
    default_email = None
    oauth = None

    def __init__(self, blackbox_result=None, mechanism=None):
        self.blackbox_result = blackbox_result
        self.authenticated_by = mechanism

    def is_authenticated(self):
        return False

    def needs_new_sessguard(self):
        return self.blackbox_result is not None and self.blackbox_result['status'] == 'WRONG_GUARD'

    def is_lite(self):
        return False

    def is_secure(self):
        return False

    def is_yandexoid(self):
        return False

    def __unicode__(self):
        return self.display_name


class YandexTestUser(BaseYandexUser):
    def __init__(self, **params):
        self.fields = params

    def is_authenticated(self):
        return True

    def needs_new_sessguard(self):
        return False

    def is_lite(self):
        return self.fields['is_lite']

    def is_social(self):
        return self.fields['is_social']

    def is_secure(self):
        return self.fields['is_secure']

    def is_yandexoid(self):
        return self.fields['is_yandexoid']

    def get_username(self):
        return self.fields['login']

    def __getattr__(self, field):
        try:
            return self.fields[field]
        except KeyError:
            raise AttributeError(field)

    def __unicode__(self):
        return u'<test user: %s>' % self.get_username()


class Application(object):
    def __init__(self, id=None, name=None, home_page=None, icon=None, uid=None,
                 ctime=None, issue_time=None, expire_time=None, scope=None):
        self.id = id
        self.name = name
        self.home_page = home_page
        self.icon = icon

        self.uid = uid

        self.ctime = parse_time(ctime) if ctime else None
        self.issue_time = parse_time(issue_time) if issue_time else None
        self.expire_time = parse_time(expire_time) if expire_time and expire_time != 'None' else None

        self.scope = scope.split(' ') if scope else []


class YandexUserDescriptor(object):
    """
    Ленивый дескриптор, вынимает яндексовый UID при первом обращении к
    себе и конструирует объект пользователя.
    """
    def __get__(self, request, owner):
        if not hasattr(request, '_yauser'):
            request._yauser = self._get_yandex_user(request)
        return request._yauser

    @property
    def authentication_mechanisms(self):
        from importlib import import_module
        for path in settings.YAUTH_MECHANISMS:
            try:
                module = import_module(path)
            except ImportError as exc:
                log.exception('Cannot import auth mechanism "%s", check that module exists', path)
                raise NoAuthMechanismError(path, exc)
            try:
                mechanism = getattr(module, 'Mechanism')
            except AttributeError as exc:
                log.exception('Cannot import auth mechanism "%s", it should be called "Mechanism"', path)
                raise NoAuthMechanismError(path, exc)
            yield mechanism()

    def _get_yandex_user(self, request):
        for mechanism in self.authentication_mechanisms:
            try:
                params = mechanism.is_applicable(request)
                if params is not None:
                    user = mechanism.apply(**params)
                    if user is not None:
                        return user
            except PermissionDenied:
                # в джанго требуется прервать выполнение
                break

        log.debug('No applicable mechanisms for this request')
        return AnonymousYandexUser()


class YandexTestUserDescriptor(YandexUserDescriptor):
    def _get_yandex_user(self, request):
        if settings.YAUTH_TEST_USER is None:
            raise ImproperlyConfigured('`YAUTH_TEST_USER` setting must be set '
                                       'to use yauser mocking')

        if settings.YAUTH_TEST_USER is False:
            return AnonymousYandexUser()

        defaults = {
            'login': 'yauth-test-user',
            'default_email': 'yauth-test-user@unknown.unknown',
            'need_reset': False,
            'is_lite': False,
            'is_social': False,
            'is_secure': False,
            'is_yandexoid': False,
            'new_session': None,
            'new_sslsession': None,
        }

        if isinstance(settings.YAUTH_TEST_USER, six.string_types):
            defaults['login'] = settings.YAUTH_TEST_USER
        else:
            defaults.update(settings.YAUTH_TEST_USER)

        return YandexTestUser(**defaults)


class ProfileDescriptor(object):
    '''
    Дескриптор доступа к профайлу пользователя, специфичному для
    приложения.  В конструктор принимает модель, у которой обязано
    быть поле 'yandex_uid', и которая должна создаваться без других
    дополнительных полей.  При первом обращении к дескриптору объект
    модели либо находится, либо создается при установленном YAUTH_CREATE_PROFILE_ON_ACCESS.
    '''
    def __init__(self, model):
        self.model = model
        self.attr = '_' + self.__class__.name_for_model(model)

    def __get__(self, yauser, owner):
        if yauser is None:
            return self

        if not hasattr(yauser, self.attr):
            try:
                profile = self.model.objects.get(yandex_uid=yauser.uid)
            except self.model.DoesNotExist:
                if get_setting(['CREATE_PROFILE_ON_ACCESS', 'YAUTH_CREATE_PROFILE_ON_ACCESS']):
                    profile = create_profile(yauser, self.model)
                else:
                    profile = None

            self.contribute_to_yauser(yauser, profile)

        return getattr(yauser, self.attr)

    def contribute_to_yauser(self, yauser, profile):
        setattr(yauser, self.attr, profile)

    @classmethod
    def name_for_model(cls, model):
        return '%s_%s' % (model._meta.app_label, model._meta.object_name.lower())


class ProfileUserDescriptor(object):
    '''
    Ленивый дескриптор, записываемый в классы профайлов, который умеет
    создавать экземпляры YandexUser по uid'у.
    '''
    def __get__(self, profile, owner):
        if not hasattr(profile, '_yauser'):
            bb = get_blackbox()

            authinfo = bb.userinfo(
                profile.yandex_uid,
                '127.0.0.1',
                dbfields=list(get_setting(['PASSPORT_FIELDS', 'YAUTH_PASSPORT_FIELDS']))
                         + [blackbox.FIELD_LOGIN_RULE],
                **settings.YAUTH_BLACKBOX_PARAMS
            )
            authinfo.url = bb.url

            profile._yauser = YandexUser(
                uid=profile.yandex_uid,
                is_lite=bb._check_login_rule(authinfo.fields['login_rule']),
                fields=authinfo.fields,
                need_reset=False,
                emails=authinfo.emails,
                default_email=authinfo.default_email,
                blackbox_result=authinfo,
            )

        return profile._yauser


class DjangoUserDescriptor(object):
    '''
    Ленивый дескриптор для навешивания на request.user. Представляется обычным
    джанговским пользователем (экземпляром auth.User), но авторизацию
    делает не стандартным способом по ключу в сессии, а просто ищет
    пользователя с логином, соответствующим уже авторизованному request.yauser.
    '''
    def __get__(self, request, owner):
        from django.contrib.auth.models import AnonymousUser

        User = get_user_model()

        if request is None:
            return self

        if not hasattr(request, '_user'):
            user = None
            if request.yauser.is_authenticated():
                try:
                    user = User.objects.get(**{User.USERNAME_FIELD: request.yauser.get_username()})
                except User.DoesNotExist:
                    if get_setting(['CREATE_USER_ON_ACCESS', 'YAUTH_CREATE_USER_ON_ACCESS']):
                        user = create_user(request.yauser)
                    elif not settings.YAUTH_ANONYMOUS_ON_MISSING:
                        raise
            request._user = user or AnonymousUser()
        return request._user

    def contribute_to_request(self, request, user):
        from django.contrib.auth.models import AnonymousUser
        request._user = user or AnonymousUser()


class ApplicationDescriptor(object):
    def __get__(self, request, owner):
        if request.yauser.oauth:
            oauth = request.yauser.oauth

            return Application(oauth.client_id, oauth.client_name, oauth.client_homepage,
                               oauth.client_icon, oauth.uid, oauth.ctime, oauth.issue_time,
                               oauth.expire_time, oauth.scope)

        return Application()


class TestApplicationDescriptor(object):
    def __get__(self, request, owner):
        if settings.YAUTH_TEST_APPLICATION is None:
            raise ImproperlyConfigured('`YAUTH_TEST_APPLICATION` setting must be set '
                                       'to use application mocking')

        if settings.YAUTH_TEST_APPLICATION is False:
            return Application()

        defaults = {}

        if isinstance(settings.YAUTH_TEST_APPLICATION, six.string_types):
            defaults['id'] = settings.YAUTH_TEST_APPLICATION
        else:
            defaults.update(settings.YAUTH_TEST_APPLICATION)

        return Application(**defaults)


class PassportAlias(object):
    """https://wiki.yandex-team.ru/passport/dbmoving/#tipyaliasov"""
    YANDEXOID = '13'


def create_profile(yauser, model):
    '''Создает профиль заданной модели, если его ещё нет'''
    profile, created = model.objects.get_or_create(yandex_uid=yauser.uid)

    if created:
        if hasattr(profile, 'profile_created'):
            profile.profile_created()
            profile._yauser = weakref.proxy(yauser)

    return profile


def create_user(yauser):
    '''Создаёт Джанго-пользователя, если его ещё нет'''
    User = get_user_model()

    defaults = {
        'email': yauser.default_email or '',
    }

    for yauser_field, user_field in settings.YAUTH_USER_EXTRA_FIELDS:
        defaults[user_field] = getattr(yauser, yauser_field)

    kwargs = {
        User.USERNAME_FIELD: yauser.get_username(),
        'defaults': defaults,
    }

    user, created = User.objects.get_or_create(**kwargs)

    if created:
        user.set_unusable_password()
        user.save()

    return user


def profile_should_be_created(request):
    '''Проверяет нужно ли создавать профиль или пользователя для данного yauser'''
    from django_yauth import models

    User = get_user_model()

    if not request.yauser.is_authenticated():
        return False

    if get_setting(['CREATE_USER_ON_ACCESS', 'YAUTH_CREATE_USER_ON_ACCESS']):
        try:
            user = User.objects.get(**{User.USERNAME_FIELD: request.yauser.get_username()})
            request.__class__.user.contribute_to_request(request, user)
        except User.DoesNotExist:
            return True

    if get_setting(['CREATE_PROFILE_ON_ACCESS', 'YAUTH_CREATE_PROFILE_ON_ACCESS']):
        for model in models.get_profile_models():
            try:
                profile = model.objects.get(yandex_uid=request.yauser.uid)

                descriptor_name = ProfileDescriptor.name_for_model(model)
                descriptor = getattr(YandexUser, descriptor_name)
                descriptor.contibute_to_yauser(request.yauser, profile)
            except model.DoesNotExist:
                return True

    return False
