# coding: utf-8
from __future__ import absolute_import

from django.db import models
from django.utils.encoding import force_bytes
from closuretree.models import ClosureModel

# Therefore it’s recommended to put your AppConf subclass(es) there,
# too. (https://django-appconf.readthedocs.io/en/latest/#overview)
from django_abc_data.conf import AbcDataAppConf  # noqa


class AbcService(ClosureModel):
    external_id = models.IntegerField(unique=True, db_index=True)
    name = models.CharField(max_length=255)
    name_en = models.CharField(max_length=255)
    state = models.CharField(max_length=32)
    parent = models.ForeignKey('self', related_name='children', null=True, blank=True, on_delete=models.CASCADE)
    slug = models.SlugField(db_index=True)
    owner_login = models.CharField(max_length=32, null=True, blank=True)
    created_at = models.DateTimeField()
    modified_at = models.DateTimeField()
    path = models.CharField(max_length=1000)
    readonly_state = models.CharField(max_length=50, null=True, blank=True, default=None)

    def __repr__(self):
        result = '<ABC Service: pk=%s, external_id=%s, slug=%s>' % (
            self.pk, self.external_id, self.slug
        )
        return result

    def __str__(self):
        return '%s (id=%s)' % (self.slug, self.external_id)

    class Meta:
        ordering = ['external_id']
