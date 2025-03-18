from django.db.models import FileField
from .storage import MDSStorage


class MDSFileField(FileField):
    """
    Subclass for overriding storage default value
    """
    def __init__(self, **kwargs):
        self.full_url = kwargs.pop('full_url', False)
        if 'storage' not in kwargs:
            kwargs['storage'] = MDSStorage(full_url=self.full_url)

        super(MDSFileField, self).__init__(**kwargs)
        # overriding fields
        self.upload_to = kwargs.pop('upload_to', 'MDS')

    def deconstruct(self):
        name, path, args, kwargs = super(MDSFileField, self).deconstruct()
        if self.full_url:
            kwargs["full_url"] = self.full_url
        return name, path, args, kwargs
