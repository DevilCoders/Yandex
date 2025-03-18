# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import django


if django.VERSION < (1, 7):
    from south.utils import datetime_utils as datetime
    from south.db import db
    from south.v2 import SchemaMigration
    from django.db import models


    class Migration(SchemaMigration):

        def forwards(self, orm):
            # Adding model 'StampRecord'
            db.create_table(u'django_alive_stamprecord', (
                (u'id', self.gf('django.db.models.fields.AutoField')(primary_key=True)),
                ('timestamp', self.gf('django.db.models.fields.DateTimeField')(auto_now=True, blank=True)),
                ('name', self.gf('django.db.models.fields.CharField')(max_length=255)),
                ('host', self.gf('django.db.models.fields.CharField')(max_length=255)),
                ('group', self.gf('django.db.models.fields.CharField')(max_length=255)),
                ('data', self.gf('django.db.models.fields.TextField')()),
            ))
            db.send_create_signal(u'django_alive', ['StampRecord'])

            # Adding unique constraint on 'StampRecord', fields ['name', 'host']
            db.create_unique(u'django_alive_stamprecord', ['name', 'host'])


        def backwards(self, orm):
            # Removing unique constraint on 'StampRecord', fields ['name', 'host']
            db.delete_unique(u'django_alive_stamprecord', ['name', 'host'])

            # Deleting model 'StampRecord'
            db.delete_table(u'django_alive_stamprecord')


        models = {
            u'django_alive.stamprecord': {
                'Meta': {'unique_together': "[(u'name', u'host')]", 'object_name': 'StampRecord'},
                'data': ('django.db.models.fields.TextField', [], {}),
                'group': ('django.db.models.fields.CharField', [], {'max_length': '255'}),
                'host': ('django.db.models.fields.CharField', [], {'max_length': '255'}),
                u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
                'name': ('django.db.models.fields.CharField', [], {'max_length': '255'}),
                'timestamp': ('django.db.models.fields.DateTimeField', [], {'auto_now': 'True', 'blank': 'True'})
            }
        }

        complete_apps = ['django_alive']
else:
    from django.db import migrations, models


    class Migration(migrations.Migration):

        initial = True

        dependencies = [
        ]

        operations = [
            migrations.CreateModel(
                name='StampRecord',
                fields=[
                    ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                    ('timestamp', models.DateTimeField(auto_now=True)),
                    ('name', models.CharField(max_length=255)),
                    ('host', models.CharField(max_length=255)),
                    ('group', models.CharField(max_length=255)),
                    ('data', models.TextField()),
                ],
            ),
            migrations.AlterUniqueTogether(
                name='stamprecord',
                unique_together=set([('name', 'host', 'group')]),
            ),
        ]
