from django.core.files.storage import Storage
from django.core.files.base import ContentFile
from django.conf import settings
from django.utils.encoding import force_text
try:
    from django.utils.deconstruct import deconstructible
except Exception:
    def deconstructible(cls):
        return cls

from django_mds.client import MDSClient


@deconstructible
class MDSStorage(Storage):
    def __init__(self, host=None, write_host=None, namespace=None,
                 write_token=None, read_token=None, timeout=None,
                 full_url=False, max_retries=None, expire=None, use_tvm2=None,
                 allowed_hosts=None):
        self.host = host or getattr(settings, 'MDS_HOST', None)
        self.write_host = write_host or getattr(settings, 'MDS_WRITE_HOST', None)
        self.namespace = namespace or getattr(settings, 'MDS_NAMESPACE', None)
        _access_token = getattr(settings, 'MDS_ACCESS_TOKEN', None)
        self.write_token = write_token or getattr(settings, 'MDS_WRITE_TOKEN', _access_token)
        self.read_token = read_token or getattr(settings, 'MDS_READ_TOKEN', _access_token)
        self.expire = expire or getattr(settings, 'MDS_EXPIRE', None)
        self.use_tvm2 = use_tvm2 or getattr(settings, 'MDS_USE_TVM2', False)
        self.allowed_hosts = allowed_hosts or getattr(settings, 'MDS_ALLOWED_HOSTS', None)
        self.validate_config()
        self.full_url = full_url

        if max_retries is None:
            max_retries = getattr(settings, 'MDS_MAX_RETRIES', None)

        self._client = MDSClient(
            read_host=self.host,
            write_host=self.write_host,
            namespace=self.namespace,
            write_token=self.write_token,
            read_token=self.read_token,
            timeout=timeout or getattr(settings, 'MDS_TIMEOUT', None),
            max_retries=max_retries,
            use_tvm2=self.use_tvm2,
            allowed_hosts=self.allowed_hosts,
        )

    def validate_config(self):
        if not self.host:
            raise AttributeError('MDS_HOST is not defined in project settings')
        if not self.write_host:
            raise AttributeError('MDS_WRITE_HOST is not defined in project settings')
        if not self.namespace:
            raise AttributeError('MDS_NAMESPACE is not defined in project settings')
        if not self.write_token:
            raise AttributeError('Neither MDS_ACCESS_TOKEN nor MDS_WRITE_TOKEN '
                                 'is defined in project settings')

    def _open(self, name, mode='rb'):
        contents = self._client.get(name)
        return ContentFile(contents)

    def exists(self, name):
        return self._client.exists(name)

    def size(self, name):
        return self._client.size(name)

    def save(self, name, content, max_length=None):
        """
        Saves content to Media Storage.

        :param content is a FieldFile from FileField of model
        """
        content.seek(0)
        path = self._client.upload(content, filename=name, expire=self.expire)

        if self.full_url:
            path = self._client.read_url(path)

        if path:
            return force_text(path.encode('UTF-8'))

        return None

    def url(self, name):
        return self._client.read_url(name)

    def get_valid_name(self, name):
        return name

    def get_available_name(self, name, max_length=None):
        return name

    def path(self, name):
        raise NotImplementedError("This backend doesn't support absolute paths.")

    def delete(self, name):
        if not name:
            return None

        if name.startswith('http'):
            name = self._client.path_from_url(name)

        self._client.delete(name)

    def listdir(self, path):
        raise NotImplementedError("This backend doesn't support directory listing.")
