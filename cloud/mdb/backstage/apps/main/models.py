from django.conf import settings
import json
import logging

import django.db

import cloud.mdb.backstage.lib.helpers as mod_helpers


logger = logging.getLogger('backstage.main.models')


class UserProfile(django.db.models.Model):
    sub = django.db.models.CharField(max_length=256, unique=True)
    settings = django.db.models.JSONField(null=True)

    def __str__(self):
        return json.dumps(self.settings)


class AuditLog(django.db.models.Model, mod_helpers.LinkedMixin):
    datetime = django.db.models.DateTimeField(
        auto_now_add=True,
        db_index=True,
    )
    event_type = django.db.models.CharField(
        max_length=128,
        db_index=True,
    )
    username = django.db.models.CharField(
        max_length=128,
        db_index=True,
    )
    user_address = django.db.models.CharField(max_length=512)
    user_comment = django.db.models.TextField(
        null=True,
        db_index=True,
    )
    request_id = django.db.models.CharField(
        max_length=255,
        db_index=True,
    )
    message = django.db.models.TextField(
        null=True,
        db_index=True,
    )
    obj_pk = django.db.models.CharField(
        max_length=255,
        null=True,
        db_index=True,
    )
    obj_app = django.db.models.CharField(
        max_length=255,
        null=True,
        db_index=True,
    )
    obj_model = django.db.models.CharField(
        max_length=255,
        null=True,
        db_index=True,
    )
    obj_action = django.db.models.CharField(
        max_length=255,
        null=True,
        db_index=True,
    )
    meta = django.db.models.JSONField(null=True)

    @classmethod
    def write_action(
        self,
        message,
        username,
        user_address,
        request_id,
        obj,
        action,
        user_comment=None,
        meta=None,
    ):
        audit_data = {
            'username': username,
            'user_address': user_address,
            'user_comment': user_comment,
            'request_id': request_id,
            'message': message,
            'event_type': 'object_action',
            'obj_pk': obj.pk,
            'obj_app': obj.self_app,
            'obj_model': obj.self_model,
            'obj_action': action,
            'meta': meta,
        }
        if settings.CONFIG.no_database:
            logger.info(audit_data)
        else:
            return AuditLog.objects.create(**audit_data)
