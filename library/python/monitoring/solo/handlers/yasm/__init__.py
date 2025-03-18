import six

from library.python.monitoring.solo.objects.yasm import YasmObject
if six.PY2:
    from library.python.monitoring.solo.handlers.yasm.py2.handler import YasmHandler  # noqa
if six.PY3:
    from library.python.monitoring.solo.handlers.yasm.py3.handler import YasmHandler  # noqa


def group_by_yasm_handlers(resources):
    def filter_yasm(_resource):
        return isinstance(_resource.local_state, YasmObject) or \
            (_resource.provider_id and
             _resource.provider_id.get("handler_type", None) == YasmHandler.__name__)

    resources_yasm = list(filter(filter_yasm, resources))
    if resources_yasm:
        handler = YasmHandler()
        yield handler, YasmObject.__name__, resources_yasm
