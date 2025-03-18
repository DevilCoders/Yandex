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
            # Removing unique constraint on 'StampRecord', fields ['name', 'host']
            db.delete_unique(u'django_alive_stamprecord', ['name', 'host'])

            # Adding unique constraint on 'StampRecord', fields ['name', 'host', 'group']
            db.create_unique(u'django_alive_stamprecord', ['name', 'host', 'group'])


        def backwards(self, orm):
            # Removing unique constraint on 'StampRecord', fields ['name', 'host', 'group']
            db.delete_unique(u'django_alive_stamprecord', ['name', 'host', 'group'])

            # Adding unique constraint on 'StampRecord', fields ['name', 'host']
            db.create_unique(u'django_alive_stamprecord', ['name', 'host'])


        models = {
            u'django_alive.stamprecord': {
                'Meta': {'unique_together': "[(u'name', u'host', u'group')]", 'object_name': 'StampRecord'},
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

        dependencies = [
            ('django_alive', '0001_initial'),
        ]

        operations = [
        ]
