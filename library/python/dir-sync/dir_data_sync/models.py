# coding: utf-8

from __future__ import unicode_literals

from copy import deepcopy
from datetime import timedelta
from django.conf import settings
from django.db import models
from model_utils.models import TimeStampedModel
from model_utils import Choices
from jsonfield import JSONField

from dir_data_sync import OPERATING_MODE_NAMES


def get_default_operating_mode():
    return OperatingMode.objects.get(name=OPERATING_MODE_NAMES.free).pk


def get_default_operating_mode_limits():
    return deepcopy(settings.DIRSYNC_DEFAULT_OPERATING_MODE_LIMITS)


class OperatingMode(TimeStampedModel):
    """
    Справочник режимов работы организации.
    """

    name = models.CharField(max_length=50, unique=True, help_text='Название режима')

    limits = JSONField(default=get_default_operating_mode_limits, help_text='значение "-1" означает "без ограничений"')

    def __unicode__(self):
        return self.name


class OrgManager(models.Manager):
    def get_queryset(self):
        return super(OrgManager, self).get_queryset().select_related('mode', 'orgstatistics')


class Organization(TimeStampedModel):
    """
    Модель организации в Директории.
    """
    ORG_STATUSES = Choices('enabled', 'disabled')

    dir_id = models.CharField(max_length=16, unique=True,
                              help_text='Идентификатор организации в Директории')

    label = models.CharField(max_length=1000,
                             help_text='Уникальный человеко-читаемый id организации')

    name = models.CharField(max_length=1000,
                            help_text='Название организации')

    lang = models.CharField(max_length=2,
                            help_text='Язык организации', default='en')

    mode = models.ForeignKey(
        OperatingMode,
        default=get_default_operating_mode,
        help_text='Текущий режим работы организации',
    )

    status = models.CharField(
        choices=ORG_STATUSES,
        max_length=20,
        help_text='Статус подключенности организации'
    )

    objects = OrgManager()

    def save(self, *args, **kwargs):
        super(Organization, self).save(*args, **kwargs)
        try:
            self.orgstatistics
        except OrgStatistics.DoesNotExist:
            stats = OrgStatistics()
            stats.org = self
            stats.save()

    def __unicode__(self):
        return 'id={id}: "{name}", dir_id={dir_id}, lang={lang}, mode={mode}'.format(
            id=self.id, name=self.name, dir_id=self.dir_id, lang=self.lang, mode=self.mode.name
        )


class SyncStatistics(TimeStampedModel):
    """
    Статистика синхронизации с директорией.
    """
    PULL_STATUSES = Choices('success', 'failed')

    dir_org_id = models.CharField(
        max_length=16, db_index=True,
        help_text='Идентификатор организации в Директории',
    )

    last_pull_status = models.CharField(
        default=PULL_STATUSES.success,
        choices=PULL_STATUSES,
        max_length=100,
        help_text='Чем завершилась последняя синхронизация'
    )

    last_pull_duration = models.DurationField(
        help_text='Сколько длилась последняя синхронизация',
        default=timedelta(seconds=0)
    )

    successful_attempts = models.PositiveIntegerField(default=0)

    def __unicode__(self):
        return 'org_id={org_id}, last time finished with "{status}" at {at}'.format(
            org_id=self.dir_org_id,
            status=self.last_pull_status,
            at=self.modified,
        )


class ChangeEvent(TimeStampedModel):
    """
    Модель данных для контроля синхронизации с данными в Директории по каждой организации.
    """

    org = models.ForeignKey(
        Organization,
        help_text='Организация, в которой произошли изменения данных',
    )

    last_pull_at = models.DateTimeField(
        null=True,
        help_text='дата и время последнего успешного обновления данных в рамках последней ревизии')

    revision = models.PositiveIntegerField(
        null=True,
        help_text='номер последней ревизии импортированных данных')

    def __unicode__(self):
        return 'org_id={org_id}, pulled at: {pulled_at}'.format(
            org_id=self.org_id,
            pulled_at=self.last_pull_at if self.last_pull_at else 'never'
        )

    class Meta:
        db_table = 'change_event'


def get_default_org_statistics():
    return deepcopy(settings.DIRSYNC_DEFAULT_ORG_STATISTICS)


class OrgStatistics(TimeStampedModel):
    """
    Статистика по организации.
    """

    org = models.OneToOneField(
        Organization,
        help_text='Организация',
    )

    is_limits_exceeded = models.BooleanField(
        default=False,
        help_text='Превышены ограничения режима работы организации',
    )

    statistics = JSONField(default=get_default_org_statistics, help_text='набор общих для организации статистических значений')
