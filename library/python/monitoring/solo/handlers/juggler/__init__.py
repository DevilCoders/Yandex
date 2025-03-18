import logging

import six

from library.python.monitoring.solo.objects.juggler import Check, Dashboard

if six.PY2:
    from library.python.monitoring.solo.handlers.juggler.py2.handler import JugglerHandler  # noqa
    from library.python.monitoring.solo.handlers.juggler.py2.solo_handler import SoloJugglerHandler  # noqa
if six.PY3:
    from library.python.monitoring.solo.handlers.juggler.py3.solo_handler import SoloJugglerHandler # noqa
    from library.python.monitoring.solo.handlers.juggler.py3.handler import JugglerHandler  # noqa

logger = logging.getLogger(__name__)


def group_by_juggler_handlers(configuration, resources):
    def filter_solo_juggler(_resource):
        return isinstance(_resource.local_state, Dashboard) or \
            (_resource.provider_id and
             _resource.provider_id.get("handler_type", None) == SoloJugglerHandler.__name__)

    def filter_juggler(_resource):
        return isinstance(_resource.local_state, Check) or \
            (_resource.provider_id and
             _resource.provider_id.get("handler_type", None) == JugglerHandler.__name__)

    resources_juggler = list(filter(filter_juggler, resources))
    resources_solo_juggler = list(filter(filter_solo_juggler, resources))

    if resources_solo_juggler:
        handler = SoloJugglerHandler(configuration["juggler_token"], configuration["juggler_endpoint"])

        yield handler, Dashboard.__name__, resources_solo_juggler

    if resources_juggler:
        handler = JugglerHandler(
            configuration["juggler_token"],
            configuration["juggler_mark"],
            configuration["juggler_endpoint"]
        )
        yield handler, Check.__name__, resources_juggler
