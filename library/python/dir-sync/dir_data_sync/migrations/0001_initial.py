# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    initial = True

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='ChangeEvent',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('revision', models.PositiveIntegerField(null=True)),
                ('last_pull_at', models.DateTimeField(null=True)),
            ],
            options={
                'db_table': 'change_event',
            },
        ),
        migrations.CreateModel(
            name='Organization',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('dir_id', models.CharField(max_length=16, unique=True)),
                ('label', models.CharField(max_length=1000)),
                ('name', models.CharField(max_length=1000)),
            ],
        ),
        migrations.AddField(
            model_name='changeevent',
            name='org',
            field=models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='dir_data_sync.Organization'),
        ),
    ]
