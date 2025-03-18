# -*- coding: utf-8 -*-
import jsonobject
import six
from library.python.monitoring.solo.objects.common import HashableJsonObject


class YasmObject(HashableJsonObject):

    def __init__(self, *args, **kwargs):
        if args and len(args) == 1 and isinstance(args, dict):
            args = {k: v for k, v in six.iteritems(args) if k in self.properties().keys()}
        elif kwargs:
            for k in six.iterkeys(kwargs):
                if k not in self.properties().keys():
                    raise AttributeError("\"{}\" object has no attribute \"{}\"".format(self.__class__.__name__, k))
        super(YasmObject, self).__init__(*args, **kwargs)

    def to_json(self):
        """
        Collect results from nested objects too.
        """
        result = super(YasmObject, self).to_json()
        for property_attr, property_object in six.iteritems(self.properties()):
            if isinstance(property_object, jsonobject.ObjectProperty):
                value = getattr(self, property_attr, None)
                if value:
                    result[property_object.name] = value.to_json()
        return result
