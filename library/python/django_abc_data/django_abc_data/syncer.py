# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import six
from sync_tools.diff_merger import DjangoDiffMerger
from sync_tools.constants import NOTHING, CREATE, UPDATE
from django.utils.module_loading import import_string

from django_abc_data.conf import settings
from django_abc_data import signals
from django_abc_data.models import AbcService

logger = logging.getLogger(__name__)


class AbcServiceSyncer(DjangoDiffMerger):
    def __init__(self, **kwargs):
        if isinstance(settings.ABC_DATA_GENERATOR, six.string_types):
            data_generator_class = import_string(settings.ABC_DATA_GENERATOR)
        else:
            data_generator_class = settings.ABC_DATA_GENERATOR

        external_ids = kwargs.pop('external_ids', None)
        kwargs['data_generator'] = data_generator_class(external_ids) if external_ids else data_generator_class()

        super(AbcServiceSyncer, self).__init__(**kwargs)

        self.deferred_parents_update = []

    def execute(self):
        super(AbcServiceSyncer, self).execute()
        for obj, diff_data, parent_external_id, action in self.deferred_parents_update:
            try:
                parent = AbcService.objects.get(external_id=parent_external_id)
            except AbcService.DoesNotExist:
                logger.error('Could not find parent with external_id=%d for service %s (id=%d)',
                             parent_external_id, obj.slug, obj.external_id)
            else:
                obj.parent = parent
                obj.save(update_fields=['parent'])
                if action == CREATE:
                    signals.service_created.send(sender=self, object=obj, data=diff_data)
                elif action == UPDATE:
                    signals.service_updated.send(sender=self, object=obj, data=diff_data)

    def check_parent(self, obj, diff_data, parent_external_id, action):
        if parent_external_id is not NOTHING and parent_external_id is not None:
            try:
                parent = AbcService.objects.get(external_id=parent_external_id)
            except AbcService.DoesNotExist:
                self.deferred_parents_update.append((obj, diff_data, parent_external_id, action))
                return False
            else:
                obj.parent = parent
                obj.save(update_fields=['parent'])
                return True
        else:
            return True

    def create_object(self, diff_data):
        parent_external_id = diff_data.pop('parent', NOTHING)
        created_object = super(AbcServiceSyncer, self).create_object(diff_data)

        if self.check_parent(created_object, diff_data, parent_external_id, CREATE):
            signals.service_created.send(sender=self, object=created_object, data=diff_data)

        logger.info('Created service %s', created_object.slug)
        return created_object

    def update_object(self, obj, diff_data):
        parent_external_id = diff_data.pop('parent', NOTHING)
        updated_object = super(AbcServiceSyncer, self).update_object(obj, diff_data)

        if self.check_parent(updated_object, diff_data, parent_external_id, UPDATE):
            signals.service_updated.send(sender=self, object=updated_object, data=diff_data)

        logger.info('Updated service %s', updated_object.slug)
        return updated_object
