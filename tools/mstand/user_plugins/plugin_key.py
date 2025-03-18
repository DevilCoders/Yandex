# coding=utf-8
from collections import OrderedDict

import user_plugins.plugin_helpers as uph
import yaqutils.misc_helpers as umisc


@umisc.hash_and_ordering_from_key_method
class PluginKey(object):
    def __init__(self, name, kwargs_name=""):
        """
        :type name: str
        :type kwargs_name: str
        """
        if not kwargs_name:
            kwargs_name = uph.DEF_KWARGS_NAME

        self._name = None
        self._kwargs_name = None
        self.name = name
        self.kwargs_name = kwargs_name

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, name):
        PluginKey.validate_name(name, "name")
        self._name = name

    @property
    def kwargs_name(self):
        return self._kwargs_name

    @kwargs_name.setter
    def kwargs_name(self, kwargs_name):
        PluginKey.validate_name(kwargs_name, "kwargs name")
        self._kwargs_name = kwargs_name

    @staticmethod
    def validate_name(name, field):
        if not uph.is_valid_name(name):
            message = u"Plugin {} '{}' is invalid. Only {} allowed.".format(field,
                                                                            name,
                                                                            uph.ALLOWED_CHARS_READABLE)
            raise Exception(message.encode("utf-8"))

    def pretty_name(self):
        if uph.is_default_kwargs_name(self.kwargs_name):
            return "{}".format(self.name)
        return "{}({})".format(self.name, self.kwargs_name)

    def serialize(self):
        result = OrderedDict()
        result["name"] = self.name
        if not uph.is_default_kwargs_name(self.kwargs_name):
            result["kwargs_name"] = self.kwargs_name

        return result

    @staticmethod
    def deserialize(plugin_key_data):
        plugin_name = plugin_key_data["name"]
        kwargs_name = plugin_key_data.get("kwargs_name")
        return PluginKey(name=plugin_name, kwargs_name=kwargs_name)

    def str_key(self):
        # serialization to string for JSON dicts and file names
        fixed_name = uph.replace_bad_chars(self.name)
        if uph.is_default_kwargs_name(self.kwargs_name):
            return "{}".format(fixed_name)
        fixed_kwargs_name = uph.replace_bad_chars(self.kwargs_name)
        return "{}_{}".format(fixed_name, fixed_kwargs_name)

    def __str__(self):
        if uph.is_default_kwargs_name(self.kwargs_name):
            return "<{}>".format(self.name)
        return "<{}({})>".format(self.name, self.kwargs_name)

    def __repr__(self):
        return str(self)

    def key(self):
        return self._name, self._kwargs_name

    @staticmethod
    def generate(module_name, class_name, alias=None):
        plugin_name = PluginKey.make_name(module_name, class_name, alias)
        return PluginKey(name=plugin_name)

    @staticmethod
    def make_name(module_name, class_name, alias=None):
        if alias:
            return alias
        if not module_name or not class_name:
            raise Exception("Module name and class name should not be empty.")
        return "{}.{}".format(module_name, class_name)
