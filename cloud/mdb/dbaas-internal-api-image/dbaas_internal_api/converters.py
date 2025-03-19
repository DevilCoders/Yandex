# -*- coding: utf-8 -*-
"""
Url converters module
"""
from werkzeug.routing import BaseConverter, ValidationError

from .utils.register import get_cluster_traits, get_cluster_type_by_url_prefix


class ClusterTypeConverter(BaseConverter):
    """
    Checks supported cluster type
    """

    def to_python(self, value):
        """
        Converts value to python representation
        """
        try:
            return get_cluster_type_by_url_prefix(value)
        except Exception:
            raise ValidationError()

    def to_url(self, value):
        """
        Converts value to string representation
        """
        return get_cluster_traits(value).url_prefix
