import six

if six.PY2:
    from library.python.monitoring.solo.handlers.solomon.v3.py2.handler import SolomonV3Handler  # noqa
elif six.PY3:
    from library.python.monitoring.solo.handlers.solomon.v3.py3.handler import SolomonV3Handler  # noqa
