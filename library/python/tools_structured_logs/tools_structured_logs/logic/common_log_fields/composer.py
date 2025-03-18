# coding: utf-8

from importlib import import_module
from cached_property import cached_property
from .field_structure import profiling_uuid
from tools_structured_logs.logic.errors import ConfigurationError


class FieldComposer(object):
    def _providers_import_paths(self):
        from tools_structured_logs.logic.configuration.library import get_config
        return get_config().get_context_providers()

    def _resolve(self, import_path):
        try:
            module = import_module(import_path)
        except ImportError:
            raise ConfigurationError('cannot import "%s"' % import_path)
        try:
            the_callable = getattr(module, 'Provider')
        except AttributeError:
            raise ConfigurationError('no "Provider" class in "%s"' % import_path)
        if not callable(the_callable):
            raise ConfigurationError('"%s" is not callable' % import_path)
        return the_callable

    @cached_property
    def providers(self):
        providers = [
            profiling_uuid.Provider(),  # запрещено без этого провайдера
        ]
        for path in self._providers_import_paths():
            provider = self._resolve(path)
            if provider:
                providers.append(provider())
        return providers

    def validate_kwargs(self, kwargs):
        required_params = set()
        for provider in self.providers:
            required_params.update(set(provider.required_kwargs))
        diff = required_params - set(kwargs)
        if diff:
            raise ConfigurationError('missing required {0}'.format(','.join(diff)))

    def do(self, **kwargs):
        ctx = {}
        self.validate_kwargs(kwargs)
        for provider in self.providers:
            ctx.update(provider(**kwargs))
        return ctx
