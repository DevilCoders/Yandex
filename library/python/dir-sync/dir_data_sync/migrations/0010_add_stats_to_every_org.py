# coding: utf-8

from django.db import migrations


def add_stats(apps, schema_editor):
    OrganizationModel = apps.get_model("dir_data_sync", "Organization")
    OrgStatisticsModel = apps.get_model("dir_data_sync", "OrgStatistics")
    orgs = OrganizationModel.objects.all()
    for org in orgs:
        try:
            org.orgstatistics
        except OrgStatisticsModel.DoesNotExist:
            stats = OrgStatisticsModel()
            stats.org = org
            stats.save()


class Migration(migrations.Migration):
    dependencies = [
        ('dir_data_sync', '0009_add_mode_to_org'),
    ]

    operations = [
        migrations.RunPython(add_stats),
    ]
