import hashlib
import importlib
import pickle
import logging
import os
import sys
import tarfile
import traceback

import mstand_utils.mstand_misc_helpers as mstand_umisc
import yaqutils.module_helpers as umodule
import yaqutils.misc_helpers as umisc


class ModuleSourceCache(object):
    def __init__(self):
        self.sources = set()
        self.source_module_names = {}

    def prepare_module_source(self, module_name, source, dest_dir, dest_module):
        """
        :type module_name: str
        :type source: str | None
        :type dest_dir: str
        :type dest_module: str
        :rtype: str
        """
        user_module_name = module_name

        if not source:
            return user_module_name

        if source in self.source_module_names:
            # do not check one source multiple times
            user_module_name = self.source_module_names[source]
        else:
            if not (os.path.isdir(source) or tarfile.is_tarfile(source)):
                if os.path.isfile(source) or os.path.islink(source):
                    module_name_by_source = umodule.get_module_name(source)
                    self.source_module_names[source] = module_name_by_source
                    user_module_name = module_name_by_source

        source_key = (source, dest_dir)
        if source_key in self.sources:
            logging.info("source %s is already prepared", source_key)
        else:
            logging.info("loading source %s", source_key)
            umodule._prepare_user_module_dir(source, dest_dir)
            self.sources.add(source_key)

        full_module_name = "{}.{}".format(dest_module, user_module_name)
        return full_module_name


# this method should not be user directly. use create_user_object instead.
def import_user_object(module_name, object_name, source, dest_dir, dest_module, module_source_cache=None):
    """
    :type module_name: str
    :type object_name: str
    :type source: str | None
    :type dest_dir: str
    :type dest_module: str
    :type module_source_cache: ModuleSourceCache | None
    :rtype:
    """

    logging.info("Import %s in module %s from %s", object_name, module_name, source)

    if module_source_cache is None:
        logging.warning("No module_source_cache passed, source caching is disabled")
        module_source_cache = ModuleSourceCache()

    full_module_name = module_source_cache.prepare_module_source(module_name=module_name, source=source,
                                                                 dest_dir=dest_dir, dest_module=dest_module)

    try:
        logging.debug("importing module %s", full_module_name)
        module = importlib.import_module(full_module_name)
        # if module cannot be loaded due to SyntaxError, it's raised here.
    except Exception as exc:
        message = "Can't import module '{}': {}\n{}".format(full_module_name, exc, traceback.format_exc())
        raise umodule.UserModuleException(message)

    try:
        logging.debug("loading object %s", object_name)
        entity = getattr(module, object_name)
    except AttributeError:
        raise umodule.UserModuleException("Object '{}' in module '{}' not found".format(object_name, full_module_name))
    if not callable(entity):
        raise umodule.UserModuleException("Object '{}' in module '{}' should be callable".format(object_name, full_module_name))
    return entity


def hash_source_path(source):
    """
    :type source: str | None
    """
    source_enc = umisc.to_string(source) or ""
    return hashlib.md5(source_enc.encode("utf-8")).hexdigest()


def create_user_object(module_name, class_name, source, kwargs, parse_json=True, module_source_cache=None,
                       plugin_dir=None, do_validate_pickling=False):
    """
    :type module_name: str
    :type class_name: str
    :type source: str | None
    :type kwargs: list[str] | None | dict
    :type parse_json: bool
    :type module_source_cache: ModuleSourceCache | None
    :type plugin_dir: str | None
    :type do_validate_pickling: bool
    :rtype:
    """
    if isinstance(kwargs, dict):
        logging.debug("kwargs is already parsed")
        object_kwargs = kwargs
    else:
        object_kwargs = umodule.parse_user_kwargs(kwargs, parse_json)

    if plugin_dir is None:
        dest_dir = mstand_umisc.get_project_path("user_plugins/user_module")
        dest_module = "user_plugins.user_module"
    else:
        dest_module = "user_module"
        plugin_subdir = hash_source_path(source)
        plugin_dir = os.path.join(plugin_dir, plugin_subdir)
        dest_dir = os.path.join(plugin_dir, dest_module)
        if sys.path[0] != plugin_dir:
            sys.path.insert(0, plugin_dir)

    object_class = import_user_object(module_name, class_name, source=source, dest_dir=dest_dir,
                                      dest_module=dest_module, module_source_cache=module_source_cache)

    signature = umodule.get_class_method_signature(object_class, "__init__")

    try:
        object_instance = object_class(**object_kwargs)
    except Exception as exc:
        logging.error("Cannot create user object %s.%s: %s", module_name, class_name, exc)
        logging.info("--> proper args should be: %s", signature)
        message = "Cannot create user object {}.{}: {}\n{}".format(module_name, class_name, exc, traceback.format_exc())
        raise umodule.UserModuleException(message)

    if do_validate_pickling:
        try:
            pickle.dumps(object_instance)
        except Exception as exc:
            logging.error("Cannot pickle user object %s.%s: %s", module_name, class_name, exc)
            message = "Cannot pickle user object {}.{}: {}\n{}".format(module_name, class_name, exc, traceback.format_exc())
            raise umodule.UserModuleException(message)

    return object_instance
