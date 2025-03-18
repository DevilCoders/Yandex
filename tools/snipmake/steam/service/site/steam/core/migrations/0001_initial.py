# -*- coding: utf-8 -*-
import datetime
import os

from south.db import db
from south.v2 import SchemaMigration
from django.db import models

from core.settings import APP_ROOT

class Migration(SchemaMigration):

    def forwards(self, orm):
        # Adding model 'User'
        db.create_table(u'core_user', (
            ('login', self.gf('django.db.models.fields.CharField')(default='', max_length=32)),
            ('language', self.gf('django.db.models.fields.CharField')(default='RU', max_length=2)),
            ('role', self.gf('django.db.models.fields.CharField')(default='AS', max_length=2)),
            ('status', self.gf('django.db.models.fields.CharField')(default='W', max_length=2)),
            ('yandex_uid', self.gf('django.db.models.fields.CharField')(max_length=100, primary_key=True)),
        ))
        db.send_create_signal(u'core', ['User'])
        try:
            with open(os.path.join(APP_ROOT,
                                   'sql/user.mysql.sql')) as sql_file:
                for line in sql_file:
                    db.execute(line)
        except IOError:
            pass

        # Adding model 'QueryBin'
        db.create_table(u'core_querybin', (
            (u'id', self.gf('django.db.models.fields.AutoField')(primary_key=True)),
            ('user', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.User'])),
            ('title', self.gf('django.db.models.fields.CharField')(max_length=100)),
            ('upload_time', self.gf('django.db.models.fields.DateTimeField')()),
            ('country', self.gf('django.db.models.fields.CharField')(max_length=2)),
            ('count', self.gf('django.db.models.fields.PositiveIntegerField')()),
            ('storage_id', self.gf('django.db.models.fields.CharField')(max_length=64)),
        ))
        db.send_create_signal(u'core', ['QueryBin'])

        # Adding model 'SnippetPool'
        db.create_table(u'core_snippetpool', (
            ('user', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.User'])),
            ('title', self.gf('django.db.models.fields.CharField')(max_length=100)),
            ('upload_time', self.gf('django.db.models.fields.DateTimeField')()),
            ('count', self.gf('django.db.models.fields.PositiveIntegerField')()),
            ('storage_id', self.gf('django.db.models.fields.CharField')(max_length=64, primary_key=True)),
        ))
        db.send_create_signal(u'core', ['SnippetPool'])

        # Adding model 'Snippet'
        db.create_table(u'core_snippet', (
            ('snippet_id', self.gf('django.db.models.fields.CharField')(max_length=74, primary_key=True)),
            ('data', self.gf('core.models.TinyJSONField')()),
        ))
        db.send_create_signal(u'core', ['Snippet'])

        # Adding model 'TaskPool'
        db.create_table(u'core_taskpool', (
            (u'id', self.gf('django.db.models.fields.AutoField')(primary_key=True)),
            ('title', self.gf('django.db.models.fields.CharField')(unique=True, max_length=100)),
            ('create_time', self.gf('django.db.models.fields.DateTimeField')()),
            ('kind', self.gf('django.db.models.fields.CharField')(default='BLD', max_length=3)),
            ('country', self.gf('django.db.models.fields.CharField')(max_length=4)),
            ('status', self.gf('django.db.models.fields.CharField')(max_length=1)),
            ('priority', self.gf('django.db.models.fields.PositiveSmallIntegerField')(default=1)),
            ('overlap', self.gf('django.db.models.fields.PositiveSmallIntegerField')(default=1)),
            ('ang_taskset_id', self.gf('django.db.models.fields.CharField')(max_length=64)),
            ('first_pool', self.gf('django.db.models.fields.related.ForeignKey')(related_name='first_ref', to=orm['core.SnippetPool'])),
            ('second_pool', self.gf('django.db.models.fields.related.ForeignKey')(related_name='second_ref', to=orm['core.SnippetPool'])),
            ('count', self.gf('django.db.models.fields.PositiveIntegerField')()),
        ))
        db.send_create_signal(u'core', ['TaskPool'])

        # Adding model 'Tag'
        db.create_table(u'core_tag', (
            ('label', self.gf('django.db.models.fields.CharField')(max_length=20, primary_key=True)),
        ))
        db.send_create_signal(u'core', ['Tag'])

        # Adding model 'Task'
        db.create_table(u'core_task', (
            (u'id', self.gf('django.db.models.fields.AutoField')(primary_key=True)),
            ('taskpool', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.TaskPool'])),
            ('first_snippet', self.gf('django.db.models.fields.related.ForeignKey')(related_name='first_ref', to=orm['core.Snippet'])),
            ('second_snippet', self.gf('django.db.models.fields.related.ForeignKey')(related_name='second_ref', to=orm['core.Snippet'])),
            ('request', self.gf('django.db.models.fields.TextField')()),
            ('region', self.gf('django.db.models.fields.PositiveIntegerField')()),
            ('status', self.gf('django.db.models.fields.CharField')(max_length=1)),
        ))
        db.send_create_signal(u'core', ['Task'])

        # Adding model 'Estimation'
        db.create_table(u'core_estimation', (
            (u'id', self.gf('django.db.models.fields.AutoField')(primary_key=True)),
            ('task', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.Task'])),
            ('user', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.User'])),
            ('create_time', self.gf('django.db.models.fields.DateTimeField')()),
            ('start_time', self.gf('django.db.models.fields.DateTimeField')()),
            ('complete_time', self.gf('django.db.models.fields.DateTimeField')()),
            ('status', self.gf('django.db.models.fields.CharField')(max_length=1)),
            ('value', self.gf('django.db.models.fields.IntegerField')()),
            ('shuffle', self.gf('django.db.models.fields.BooleanField')(default=False)),
            ('comment', self.gf('django.db.models.fields.TextField')()),
            ('answer', self.gf('django.db.models.fields.TextField')()),
        ))
        db.send_create_signal(u'core', ['Estimation'])

        # Adding model 'Correction'
        db.create_table(u'core_correction', (
            (u'id', self.gf('django.db.models.fields.AutoField')(primary_key=True)),
            ('aadmin_est', self.gf('django.db.models.fields.related.ForeignKey')(related_name='aadmin_corrections', to=orm['core.Estimation'])),
            ('assessor_est', self.gf('django.db.models.fields.related.ForeignKey')(related_name='assessor_corrections', to=orm['core.Estimation'])),
            ('errors', self.gf('django.db.models.fields.PositiveIntegerField')()),
            ('time', self.gf('django.db.models.fields.DateTimeField')()),
            ('comment', self.gf('django.db.models.fields.TextField')()),
            ('status', self.gf('django.db.models.fields.CharField')(max_length=1)),
        ))
        db.send_create_signal(u'core', ['Correction'])

        # Adding model 'Estimation_Tag'
        db.create_table(u'core_estimation_tag', (
            (u'id', self.gf('django.db.models.fields.AutoField')(primary_key=True)),
            ('estimation', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.Estimation'])),
            ('tag', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.Tag'])),
        ))
        db.send_create_signal(u'core', ['Estimation_Tag'])

        # Adding unique constraint on 'Estimation_Tag', fields ['estimation', 'tag']
        db.create_unique(u'core_estimation_tag', ['estimation_id', 'tag_id'])

        # Adding model 'MetricReport'
        db.create_table(u'core_metricreport', (
            (u'id', self.gf('django.db.models.fields.AutoField')(primary_key=True)),
            ('user', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.User'])),
            ('kind', self.gf('django.db.models.fields.CharField')(max_length=3)),
            ('storage_id', self.gf('django.db.models.fields.CharField')(max_length=64)),
        ))
        db.send_create_signal(u'core', ['MetricReport'])

        # Adding model 'MetricReport_SnippetPool'
        db.create_table(u'core_metricreport_snippetpool', (
            (u'id', self.gf('django.db.models.fields.AutoField')(primary_key=True)),
            ('report', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.MetricReport'])),
            ('snippetpool', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.SnippetPool'])),
        ))
        db.send_create_signal(u'core', ['MetricReport_SnippetPool'])

        # Adding unique constraint on 'MetricReport_SnippetPool', fields ['report', 'snippetpool']
        db.create_unique(u'core_metricreport_snippetpool', ['report_id', 'snippetpool_id'])

        # Adding model 'BackgroundTask'
        db.create_table(u'core_backgroundtask', (
            ('celery_id', self.gf('django.db.models.fields.CharField')(max_length=36, primary_key=True)),
            ('title', self.gf('django.db.models.fields.CharField')(max_length=100)),
            ('task_type', self.gf('django.db.models.fields.CharField')(max_length=1)),
            ('status', self.gf('django.db.models.fields.CharField')(max_length=100)),
            ('start_time', self.gf('django.db.models.fields.DateTimeField')()),
            ('user', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.User'])),
            ('storage_id', self.gf('django.db.models.fields.CharField')(max_length=64)),
        ))
        db.send_create_signal(u'core', ['BackgroundTask'])

        # Adding model 'ANGReport'
        db.create_table(u'core_angreport', (
            (u'id', self.gf('django.db.models.fields.AutoField')(primary_key=True)),
            ('celery_id', self.gf('django.db.models.fields.CharField')(max_length=36)),
            ('time', self.gf('django.db.models.fields.DateTimeField')()),
            ('taskpool', self.gf('django.db.models.fields.related.ForeignKey')(to=orm['core.TaskPool'])),
            ('revision_number', self.gf('django.db.models.fields.PositiveIntegerField')()),
            ('status', self.gf('django.db.models.fields.CharField')(max_length=1)),
            ('storage_id', self.gf('django.db.models.fields.CharField')(max_length=64)),
        ))
        db.send_create_signal(u'core', ['ANGReport'])


    def backwards(self, orm):
        # Removing unique constraint on 'MetricReport_SnippetPool', fields ['report', 'snippetpool']
        db.delete_unique(u'core_metricreport_snippetpool', ['report_id', 'snippetpool_id'])

        # Removing unique constraint on 'Estimation_Tag', fields ['estimation', 'tag']
        db.delete_unique(u'core_estimation_tag', ['estimation_id', 'tag_id'])

        # Deleting model 'User'
        db.delete_table(u'core_user')

        # Deleting model 'QueryBin'
        db.delete_table(u'core_querybin')

        # Deleting model 'SnippetPool'
        db.delete_table(u'core_snippetpool')

        # Deleting model 'Snippet'
        db.delete_table(u'core_snippet')

        # Deleting model 'TaskPool'
        db.delete_table(u'core_taskpool')

        # Deleting model 'Tag'
        db.delete_table(u'core_tag')

        # Deleting model 'Task'
        db.delete_table(u'core_task')

        # Deleting model 'Estimation'
        db.delete_table(u'core_estimation')

        # Deleting model 'Correction'
        db.delete_table(u'core_correction')

        # Deleting model 'Estimation_Tag'
        db.delete_table(u'core_estimation_tag')

        # Deleting model 'MetricReport'
        db.delete_table(u'core_metricreport')

        # Deleting model 'MetricReport_SnippetPool'
        db.delete_table(u'core_metricreport_snippetpool')

        # Deleting model 'BackgroundTask'
        db.delete_table(u'core_backgroundtask')

        # Deleting model 'ANGReport'
        db.delete_table(u'core_angreport')


    models = {
        u'core.angreport': {
            'Meta': {'object_name': 'ANGReport'},
            'celery_id': ('django.db.models.fields.CharField', [], {'max_length': '36'}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'revision_number': ('django.db.models.fields.PositiveIntegerField', [], {}),
            'status': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'storage_id': ('django.db.models.fields.CharField', [], {'max_length': '64'}),
            'taskpool': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.TaskPool']"}),
            'time': ('django.db.models.fields.DateTimeField', [], {})
        },
        u'core.backgroundtask': {
            'Meta': {'object_name': 'BackgroundTask'},
            'celery_id': ('django.db.models.fields.CharField', [], {'max_length': '36', 'primary_key': 'True'}),
            'start_time': ('django.db.models.fields.DateTimeField', [], {}),
            'status': ('django.db.models.fields.CharField', [], {'max_length': '100'}),
            'storage_id': ('django.db.models.fields.CharField', [], {'max_length': '64'}),
            'task_type': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'title': ('django.db.models.fields.CharField', [], {'max_length': '100'}),
            'user': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.User']"})
        },
        u'core.correction': {
            'Meta': {'object_name': 'Correction'},
            'aadmin_est': ('django.db.models.fields.related.ForeignKey', [], {'related_name': "'aadmin_corrections'", 'to': u"orm['core.Estimation']"}),
            'assessor_est': ('django.db.models.fields.related.ForeignKey', [], {'related_name': "'assessor_corrections'", 'to': u"orm['core.Estimation']"}),
            'comment': ('django.db.models.fields.TextField', [], {}),
            'errors': ('django.db.models.fields.PositiveIntegerField', [], {}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'status': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'time': ('django.db.models.fields.DateTimeField', [], {})
        },
        u'core.estimation': {
            'Meta': {'object_name': 'Estimation'},
            'answer': ('django.db.models.fields.TextField', [], {}),
            'comment': ('django.db.models.fields.TextField', [], {}),
            'complete_time': ('django.db.models.fields.DateTimeField', [], {}),
            'create_time': ('django.db.models.fields.DateTimeField', [], {}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'shuffle': ('django.db.models.fields.BooleanField', [], {'default': 'False'}),
            'start_time': ('django.db.models.fields.DateTimeField', [], {}),
            'status': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'tags': ('django.db.models.fields.related.ManyToManyField', [], {'to': u"orm['core.Tag']", 'through': u"orm['core.Estimation_Tag']", 'symmetrical': 'False'}),
            'task': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.Task']"}),
            'user': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.User']"}),
            'value': ('django.db.models.fields.IntegerField', [], {})
        },
        u'core.estimation_tag': {
            'Meta': {'unique_together': "(('estimation', 'tag'),)", 'object_name': 'Estimation_Tag'},
            'estimation': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.Estimation']"}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'tag': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.Tag']"})
        },
        u'core.metricreport': {
            'Meta': {'object_name': 'MetricReport'},
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'kind': ('django.db.models.fields.CharField', [], {'max_length': '3'}),
            'pools': ('django.db.models.fields.related.ManyToManyField', [], {'to': u"orm['core.SnippetPool']", 'through': u"orm['core.MetricReport_SnippetPool']", 'symmetrical': 'False'}),
            'storage_id': ('django.db.models.fields.CharField', [], {'max_length': '64'}),
            'user': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.User']"})
        },
        u'core.metricreport_snippetpool': {
            'Meta': {'unique_together': "(('report', 'snippetpool'),)", 'object_name': 'MetricReport_SnippetPool'},
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'report': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.MetricReport']"}),
            'snippetpool': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.SnippetPool']"})
        },
        u'core.querybin': {
            'Meta': {'object_name': 'QueryBin'},
            'count': ('django.db.models.fields.PositiveIntegerField', [], {}),
            'country': ('django.db.models.fields.CharField', [], {'max_length': '2'}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'storage_id': ('django.db.models.fields.CharField', [], {'max_length': '64'}),
            'title': ('django.db.models.fields.CharField', [], {'max_length': '100'}),
            'upload_time': ('django.db.models.fields.DateTimeField', [], {}),
            'user': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.User']"})
        },
        u'core.snippet': {
            'Meta': {'object_name': 'Snippet'},
            'data': ('core.models.TinyJSONField', [], {}),
            'snippet_id': ('django.db.models.fields.CharField', [], {'max_length': '74', 'primary_key': 'True'})
        },
        u'core.snippetpool': {
            'Meta': {'object_name': 'SnippetPool'},
            'count': ('django.db.models.fields.PositiveIntegerField', [], {}),
            'storage_id': ('django.db.models.fields.CharField', [], {'max_length': '64', 'primary_key': 'True'}),
            'title': ('django.db.models.fields.CharField', [], {'max_length': '100'}),
            'upload_time': ('django.db.models.fields.DateTimeField', [], {}),
            'user': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.User']"})
        },
        u'core.tag': {
            'Meta': {'object_name': 'Tag'},
            'label': ('django.db.models.fields.CharField', [], {'max_length': '20', 'primary_key': 'True'})
        },
        u'core.task': {
            'Meta': {'object_name': 'Task'},
            'first_snippet': ('django.db.models.fields.related.ForeignKey', [], {'related_name': "'first_ref'", 'to': u"orm['core.Snippet']"}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'region': ('django.db.models.fields.PositiveIntegerField', [], {}),
            'request': ('django.db.models.fields.TextField', [], {}),
            'second_snippet': ('django.db.models.fields.related.ForeignKey', [], {'related_name': "'second_ref'", 'to': u"orm['core.Snippet']"}),
            'status': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'taskpool': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.TaskPool']"})
        },
        u'core.taskpool': {
            'Meta': {'object_name': 'TaskPool'},
            'ang_taskset_id': ('django.db.models.fields.CharField', [], {'max_length': '64'}),
            'count': ('django.db.models.fields.PositiveIntegerField', [], {}),
            'country': ('django.db.models.fields.CharField', [], {'max_length': '4'}),
            'create_time': ('django.db.models.fields.DateTimeField', [], {}),
            'first_pool': ('django.db.models.fields.related.ForeignKey', [], {'related_name': "'first_ref'", 'to': u"orm['core.SnippetPool']"}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'kind': ('django.db.models.fields.CharField', [], {'default': "'BLD'", 'max_length': '3'}),
            'overlap': ('django.db.models.fields.PositiveSmallIntegerField', [], {'default': '1'}),
            'priority': ('django.db.models.fields.PositiveSmallIntegerField', [], {'default': '1'}),
            'second_pool': ('django.db.models.fields.related.ForeignKey', [], {'related_name': "'second_ref'", 'to': u"orm['core.SnippetPool']"}),
            'status': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'title': ('django.db.models.fields.CharField', [], {'unique': 'True', 'max_length': '100'})
        },
        u'core.user': {
            'Meta': {'object_name': 'User'},
            'language': ('django.db.models.fields.CharField', [], {'default': "'RU'", 'max_length': '2'}),
            'login': ('django.db.models.fields.CharField', [], {'default': "''", 'max_length': '32'}),
            'role': ('django.db.models.fields.CharField', [], {'default': "'AS'", 'max_length': '2'}),
            'status': ('django.db.models.fields.CharField', [], {'default': "'W'", 'max_length': '2'}),
            'yandex_uid': ('django.db.models.fields.CharField', [], {'max_length': '100', 'primary_key': 'True'})
        }
    }

    complete_apps = ['core']
