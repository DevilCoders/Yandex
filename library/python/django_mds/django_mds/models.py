from django.conf import settings

# South migrations legacy support
if 'south' in settings.INSTALLED_APPS:
    from south.modelsinspector import add_introspection_rules
    add_introspection_rules([], ["^django_mds\.fields\.MDSFileField"])
