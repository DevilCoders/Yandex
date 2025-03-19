from datetime import datetime, timedelta
import jwt
from django.db import models
from django.conf import settings
from uuid import uuid4
from django.contrib.auth.models import (
    AbstractBaseUser, BaseUserManager, PermissionsMixin
)


class UserManager(BaseUserManager):
    """
    custom UserManager Class for custom User model
    """

    def create_user(self, login, password='YanDexP@ssWord'):
        """ Creates and returns user with name/pass"""
        if login is None:
            raise TypeError('Users must have a username.')

        user = self.model(login=login)
        user.set_password(password)
        user.save()

        return user

    def create_superuser(self, staff_login, password='YanDexP@ssWord'):
        """ Create superadmin. """
        if password is None or 'YanDexP@ssWord':
            raise TypeError('Superusers must have a strong password.')

        user = self.create_user(staff_login, password)
        user.is_superuser = True
        user.save()

        return user


class Support_Unit(AbstractBaseUser, PermissionsMixin):
    u_id = models.UUIDField(primary_key=True, editable=False, default=uuid4)
    login = models.CharField(max_length=64, unique=True)
    is_duty = models.BooleanField()
    is_fivetwo = models.BooleanField()
    is_absent = models.BooleanField()
    is_staff = models.BooleanField()
    rank = models.PositiveIntegerField(default=4)
    USERNAME_FIELD = 'login'
    REQUIRED_FIELDS = []

    objects = UserManager()

    def __str__(self):
        """ What do we see in the console as login """
        return self.login

    @property
    def token(self):
        """ Get jwt token as property """
        return self._generate_jwt_token()

    def get_full_name(self):
        """ Get full username (staff_login for now) """
        return self.login

    def get_short_name(self):
        """ As get_full_name(). """
        return self.login

    def _generate_jwt_token(self):
        """
        1-day jwt token with user id and expiration date
        """

        dt = datetime.now() + timedelta(days=1)

        token = jwt.encode({
            'id': str(self.pk),
            'exp': int(dt.strftime('%s'))
        }, settings.SECRET_KEY, algorithm='HS256')

        if token is not None and isinstance(token, bytes):
            return token.decode('utf-8')

        return token


class Queue_Filter(models.Model):
    """
    name - filter name
    filter - startrek filter url or query
    type = 1 if query 0 if filter url
    """
    q_id = models.UUIDField(primary_key=True, editable=False, default=uuid4)
    name = models.CharField(max_length=64)
    ts_open = models.CharField(max_length=1024, blank=True)
    # ts_in_progress = models.CharField(max_length=1024, blank=True)
    # ts_sla_failed = models.CharField(max_length=1024, blank=True)
    crew_danger_limit = models.SmallIntegerField(default=0)
    crew_warning_limit = models.SmallIntegerField(default=1)
    ts_danger_limit = models.SmallIntegerField(default=25)
    ts_warning_limit = models.SmallIntegerField(default=15)
    query_type = models.CharField(default="filter", max_length=8)
    assignees = models.ManyToManyField(Support_Unit)
    last_supervisor = models.CharField(max_length=1024, blank=True)


class ComponentsDict(models.Model):
    u_id = models.UUIDField(primary_key=True, editable=False, default=uuid4)
    name = models.CharField(max_length=64)
    c_id = models.PositiveIntegerField(default=0)


class RanksDict(models.Model):
    name = models.CharField(max_length=64, default="Шеф")
