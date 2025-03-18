# coding: utf-8

from __future__ import unicode_literals

from django.contrib import admin
from django.contrib.admin import site

from dir_data_sync import models


class OrgStatisticsInline(admin.StackedInline):
    model = models.OrgStatistics
    fields = (
        'is_limits_exceeded',
        'statistics',
    )


class OrganizationAdmin(admin.ModelAdmin):
    list_display = (
        'dir_id',
        'name',
        'label',
        'lang',
        'mode',
        'status',
    )
    search_fields = (
        '=dir_id',
        '=label',
        'name',
    )
    list_filter = (
        'mode',
        'status',
    )
    inlines = [
        OrgStatisticsInline,
    ]


class OperatingModeAdmin(admin.ModelAdmin):
    list_display = (
        'name',
        'limits',
    )


site.register(models.Organization, OrganizationAdmin)
site.register(models.OperatingMode, OperatingModeAdmin)
