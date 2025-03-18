import six

if six.PY2:
    from library.python.monitoring.solo.handlers.solomon.v2.py2.handler import SolomonV2Handler  # noqa
elif six.PY3:
    from library.python.monitoring.solo.handlers.solomon.v2.py3.handler import SolomonV2Handler  # noqa
