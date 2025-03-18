# coding: utf-8

from cached_property import cached_property


FIELD_PREFIX = 'field_'


class BaseProvider(object):
    required_kwargs = []

    @cached_property
    def _field_providers(self):
        return {
            attr_name[len(FIELD_PREFIX):]: getattr(self, attr_name)
            for attr_name in dir(self)
            if attr_name.startswith(FIELD_PREFIX) and callable(getattr(self, attr_name))
        }

    def _dry_ctx(self, big_ctx):
        return {
            key: big_ctx[key] for key in self.required_kwargs
        }

    def __call__(self, **kwargs):
        provided_fields = {}
        ctx = self._dry_ctx(kwargs)
        for field_name in self._field_providers:
            provided_fields[field_name] = self._field_providers[field_name](**ctx)
        return provided_fields
