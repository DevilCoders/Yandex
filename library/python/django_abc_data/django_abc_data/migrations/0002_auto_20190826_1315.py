# -*- coding: utf-8 -*-
# Generated by Django 1.11.23 on 2019-08-26 10:15
from __future__ import unicode_literals

from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    dependencies = [
        ('django_abc_data', '0001_initial'),
    ]

    operations = [
        migrations.CreateModel(
            name='AbcServiceClosure',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('depth', models.IntegerField()),
                ('child', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, related_name='abcserviceclosure_parents', to='django_abc_data.AbcService')),
                ('parent', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, related_name='abcserviceclosure_children', to='django_abc_data.AbcService')),
            ],
            options={
                'db_table': 'django_abc_data_abcserviceclosure',
            },
        ),
        migrations.AlterUniqueTogether(
            name='abcserviceclosure',
            unique_together=set([('parent', 'child')]),
        ),
    ]
