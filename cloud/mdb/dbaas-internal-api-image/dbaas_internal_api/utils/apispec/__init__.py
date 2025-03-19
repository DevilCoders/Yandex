"""
Extensions to apispec
"""
from apispec import __version__

if __version__ >= '1.3.3':
    from .apispec_v1_3_3 import schema_to_jsonschema  # noqa
else:
    from .apispec_v0_3_9 import schema_to_jsonschema  # noqa
