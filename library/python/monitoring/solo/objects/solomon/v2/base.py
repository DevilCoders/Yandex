# -*- coding: utf-8 -*-
import six
from library.python.monitoring.solo.objects.common import HashableJsonObject


class SolomonObject(HashableJsonObject):

    def __init__(self, *args, **kwargs):
        if args and len(args) == 1 and isinstance(args, dict):
            args = {k: v for k, v in six.iteritems(args) if k in self.properties().keys()}
        elif kwargs:
            for k in six.iterkeys(kwargs):
                if k not in self.properties().keys():
                    raise AttributeError("\"{}\" object has no attribute \"{}\"".format(self.__class__.__name__, k))
        super(SolomonObject, self).__init__(*args, **kwargs)

    def to_json(self):
        self.on_serialize()
        return super(SolomonObject, self).to_json()

    # TODO(lazuka23) - get rid of
    def on_serialize(self):
        pass
