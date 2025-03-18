# -*- coding: utf-8 -*-
import datetime
from south.db import db
from south.v2 import SchemaMigration
from django.db import models


class Migration(SchemaMigration):

    def forwards(self, orm):
        db.delete_foreign_key(u'core_task', 'first_snippet_id')
        db.delete_foreign_key(u'core_task', 'second_snippet_id')

    def backwards(self, orm):
        sql = db.foreign_key_sql(u'core_task', 'first_snippet_id', u'core_snippet', 'snippet_id')
        db.execute(sql)

        sql = db.foreign_key_sql(u'core_task', 'second_snippet_id', u'core_snippet', 'snippet_id')
        db.execute(sql)

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
            'Meta': {'ordering': "('-time',)", 'object_name': 'Correction'},
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
            'c_time': ('django.db.models.fields.DateTimeField', [], {'default': 'datetime.datetime(1970, 1, 1, 0, 0)'}),
            'comment': ('django.db.models.fields.TextField', [], {}),
            'complete_time': ('django.db.models.fields.DateTimeField', [], {}),
            'correction': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.Correction']", 'null': 'True', 'on_delete': 'models.SET_NULL', 'blank': 'True'}),
            'create_time': ('django.db.models.fields.DateTimeField', [], {}),
            'i_time': ('django.db.models.fields.DateTimeField', [], {'default': 'datetime.datetime(1970, 1, 1, 0, 0)'}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'json_value': ('core.models.TinyJSONField', [], {'null': 'True'}),
            'r_time': ('django.db.models.fields.DateTimeField', [], {'default': 'datetime.datetime(1970, 1, 1, 0, 0)'}),
            'rendering_time': ('django.db.models.fields.DateTimeField', [], {'default': 'datetime.datetime(1970, 1, 1, 0, 0)'}),
            'start_time': ('django.db.models.fields.DateTimeField', [], {}),
            'status': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'tags': ('django.db.models.fields.related.ManyToManyField', [], {'to': u"orm['core.Tag']", 'through': u"orm['core.Estimation_Tag']", 'symmetrical': 'False'}),
            'task': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.Task']"}),
            'taskpack': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.TaskPack']", 'null': 'True', 'on_delete': 'models.SET_NULL', 'blank': 'True'}),
            'user': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.User']"})
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
        u'core.notification': {
            'Meta': {'object_name': 'Notification'},
            'estimation': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.Estimation']", 'null': 'True'}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'kind': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'taskpack': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.TaskPack']", 'null': 'True'}),
            'user': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.User']"})
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
            'tpl': ('django.db.models.fields.CharField', [], {'default': "'ANY'", 'max_length': '3'}),
            'upload_time': ('django.db.models.fields.DateTimeField', [], {}),
            'user': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.User']"})
        },
        u'core.syncronizer': {
            'Meta': {'object_name': 'Syncronizer'},
            'aadmin_email': ('django.db.models.fields.DateField', [], {}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'})
        },
        u'core.tag': {
            'Meta': {'object_name': 'Tag'},
            'label': ('django.db.models.fields.CharField', [], {'max_length': '20', 'primary_key': 'True'})
        },
        u'core.task': {
            'Meta': {'object_name': 'Task'},
            'first_snippet': ('django.db.models.fields.CharField', [], {'max_length': '74'}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'region': ('django.db.models.fields.PositiveIntegerField', [], {}),
            'request': ('django.db.models.fields.TextField', [], {}),
            'second_snippet': ('django.db.models.fields.CharField', [], {'max_length': '74'}),
            'shuffle': ('django.db.models.fields.BooleanField', [], {'default': 'False'}),
            'status': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'taskpool': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.TaskPool']"})
        },
        u'core.taskpack': {
            'Meta': {'object_name': 'TaskPack'},
            'emailed': ('django.db.models.fields.BooleanField', [], {'default': 'False'}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'last_update': ('django.db.models.fields.DateTimeField', [], {}),
            'status': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'taskpool': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.TaskPool']"}),
            'user': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.User']"})
        },
        u'core.taskpool': {
            'Meta': {'object_name': 'TaskPool'},
            'ang_taskset_id': ('django.db.models.fields.CharField', [], {'max_length': '64'}),
            'count': ('django.db.models.fields.PositiveIntegerField', [], {}),
            'create_time': ('django.db.models.fields.DateTimeField', [], {}),
            'deadline': ('django.db.models.fields.DateField', [], {'default': 'datetime.datetime(1970, 1, 1, 0, 0)'}),
            'first_pool': ('django.db.models.fields.related.ForeignKey', [], {'related_name': "'first_ref'", 'to': u"orm['core.SnippetPool']"}),
            'first_pool_tpl': ('django.db.models.fields.CharField', [], {'default': "'ANY'", 'max_length': '3'}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'kind': ('django.db.models.fields.CharField', [], {'default': "'BLD'", 'max_length': '3'}),
            'overlap': ('django.db.models.fields.PositiveSmallIntegerField', [], {'default': '1'}),
            'pack_size': ('django.db.models.fields.PositiveIntegerField', [], {'default': '1'}),
            'priority': ('django.db.models.fields.PositiveSmallIntegerField', [], {'default': '1'}),
            'second_pool': ('django.db.models.fields.related.ForeignKey', [], {'related_name': "'second_ref'", 'to': u"orm['core.SnippetPool']"}),
            'second_pool_tpl': ('django.db.models.fields.CharField', [], {'default': "'ANY'", 'max_length': '3'}),
            'status': ('django.db.models.fields.CharField', [], {'max_length': '1'}),
            'title': ('django.db.models.fields.CharField', [], {'unique': 'True', 'max_length': '100'}),
            'user': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.User']", 'null': 'True'})
        },
        u'core.tpcountry': {
            'Meta': {'object_name': 'TPCountry'},
            'country': ('django.db.models.fields.CharField', [], {'max_length': '2'}),
            u'id': ('django.db.models.fields.AutoField', [], {'primary_key': 'True'}),
            'taskpool': ('django.db.models.fields.related.ForeignKey', [], {'to': u"orm['core.TaskPool']"})
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
