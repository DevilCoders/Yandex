import time
import logging
import traceback

from collections import OrderedDict

import mstand_utils.mstand_module_helpers as mstand_umodule

import yaqutils.module_helpers as umodule
import yaqutils.misc_helpers as umisc
import yaqutils.json_helpers as ujson
import yaqutils.six_helpers as usix

import user_plugins.plugin_helpers as uph
from user_plugins import PluginKey
from yaqutils import UserModuleException


class PluginKwargs(object):
    def __init__(self, name=None, kwargs=None):
        """
        :type name: str | None
        :type kwargs: dict | None
        """
        self.name = name if name else "default"
        if kwargs:
            assert isinstance(kwargs, dict)
        self.kwargs = kwargs or {}

    def serialize(self):
        result = OrderedDict()
        result["name"] = self.name
        result["kwargs"] = self.kwargs
        return result

    @staticmethod
    def deserialize(json_data):
        name = json_data.get("name", "default")
        kwargs = json_data.get("kwargs", {})
        return PluginKwargs(name=name, kwargs=kwargs)

    def __repr__(self):
        return str(self)

    def __str__(self):
        if uph.is_default_kwargs_name(self.name):
            if not self.kwargs:
                return "<None>"
            else:
                return "Kw(kwargs={})".format(self.kwargs)
        else:
            if not self.kwargs:
                return "Kw({})".format(self.name)
            else:
                return "Kw({}, kwargs={})".format(self.name, self.kwargs)


class PluginStar:
    VALIDATED = "controls_validated"
    RELIABLE = "reliable"
    ELITE = "elite"
    ALL = {VALIDATED, RELIABLE, ELITE}


class PluginABInfo(object):
    def __init__(self, group=None, star=None, description=None, hname=None):
        """
        :type group: str | None
        :type star: str | None
        :type description: str | None
        :type hname: str | None
        """
        self.group = group
        if star is not None and star not in (PluginStar.VALIDATED, PluginStar.RELIABLE, PluginStar.ELITE):
            raise Exception("Value 'star' in 'ab_info' supposed to be one of the following values: " +
                            "{}, {}, {}".format(PluginStar.VALIDATED, PluginStar.RELIABLE, PluginStar.ELITE))
        self.star = star
        if not description:
            description = None
        self.description = description
        if not hname:
            hname = None
        self.hname = hname

    def serialize(self):
        result = OrderedDict()
        if self.group is not None:
            result["group"] = self.group
        if self.star is not None:
            result["star"] = self.star
        if self.description is not None:
            result["description"] = self.description
        if self.hname is not None:
            result["hname"] = self.hname
        return result

    @staticmethod
    def deserialize(ab_info_data):
        """
        :type ab_info_data: dict
        :rtype: PluginABInfo | None
        """
        if ab_info_data is None:
            return None
        else:
            return PluginABInfo(group=ab_info_data.get("group"), star=ab_info_data.get("star"),
                                description=ab_info_data.get("description"), hname=ab_info_data.get("hname"))

    def empty(self):
        return self.group is None and self.star is None and self.description is None and self.hname is None


class PluginSource(object):
    def __init__(self, module_name, class_name, alias=None, kwargs_list=None, url=None, revision=None, ab_info=None):
        """
        :type module_name:
        :type class_name:
        :type alias: str | None
        :type kwargs_list: list[PluginKwargs]
        :type ab_info: PluginABInfo | None
        """
        self.alias = alias
        if not module_name or not class_name:
            raise Exception("Module name or class name should not be empty.")
        self.module_name = module_name
        self.class_name = class_name
        if not kwargs_list:
            logging.info("Assuming empty kwargs_list as single argument set.")
            kwargs_list = [PluginKwargs()]
        self.kwargs_list = kwargs_list
        self.url = url
        self.revision = revision
        self.ab_info = ab_info

    def serialize(self):

        result = OrderedDict()
        if self.alias:
            result["alias"] = self.alias

        result["module"] = self.module_name
        result["class"] = self.class_name
        result["kwargs_list"] = umisc.serialize_array(self.kwargs_list)

        if self.url:
            result["url"] = self.url

        if self.revision:
            result["revision"] = self.revision

        if self.ab_info is not None and not self.ab_info.empty():
            result["ab_info"] = self.ab_info.serialize()

        return result

    @staticmethod
    def deserialize(json_data):
        alias = json_data.get("alias")
        module_name = json_data["module"]
        class_name = json_data["class"]
        url = json_data.get("url")
        revision = json_data.get("revision")
        ab_info = PluginABInfo.deserialize(json_data.get("ab_info"))

        kwargs_list = PluginSource.parse_kwargs_fields(json_data)

        return PluginSource(alias=alias, module_name=module_name, class_name=class_name,
                            kwargs_list=kwargs_list, url=url, revision=revision, ab_info=ab_info)

    def __repr__(self):
        return str(self)

    def __str__(self):
        main_info = u"a={}, m={}, c={}".format(self.alias, self.module_name, self.class_name)

        if len(self.kwargs_list) == 1:
            return u"Src({}, kwargs={})".format(main_info, self.kwargs_list[0])
        else:
            return u"Src({}, {} sets of kwargs)".format(main_info, len(self.kwargs_list))

    @staticmethod
    def parse_kwargs_fields(json_data):
        if "kwargs_list" in json_data and "kwargs" in json_data:
            raise Exception("Both 'kwargs' and 'kwargs_list' specified, which is forbidden.")

        if "kwargs" in json_data:
            # simple variant
            kwargs = json_data.get("kwargs", {})
            kwargs_list = [PluginKwargs(kwargs=kwargs)]
        else:
            # multiple argsets variant
            json_kwargs_list = json_data.get("kwargs_list")
            if not json_kwargs_list:
                kwargs_list = [PluginKwargs()]
            else:
                kwargs_list = umisc.deserialize_array(json_kwargs_list, PluginKwargs)
        return kwargs_list


class PluginBatch(object):
    def __init__(self, plugin_sources, source=None):
        """
        :type plugin_sources: list[PluginSource]
        """
        self.plugin_sources = plugin_sources
        self.source = source

    def serialize(self):
        result = {
            "plugins": umisc.serialize_array(self.plugin_sources)
        }
        if self.source:
            result["source"] = self.source
        return result

    @staticmethod
    def deserialize(plugin_batch_data):
        """
        :type plugin_batch_data: dict
        :rtype: PluginBatch
        """
        raw_plugin_sources = plugin_batch_data["plugins"]
        if not isinstance(raw_plugin_sources, list):
            raise Exception("Incorrect metric batch format: 'plugins' field is not an array")
        plugin_sources = umisc.deserialize_array(raw_plugin_sources, PluginSource)
        source = plugin_batch_data.get("source")
        return PluginBatch(plugin_sources, source)

    @staticmethod
    def load_from_file(batch_file_name):
        try:
            plugin_batch_data = ujson.load_from_file(batch_file_name)
        except Exception as exc:
            logging.error("Cannot load metric batch file: %s, details: %s", exc, traceback.format_exc())
            raise Exception("Failed to load metric batch file. For details see the previous traceback.")

        return PluginBatch.deserialize(plugin_batch_data)


def validate_single_mode_params(module_name, class_name, plugin_alias, user_kwargs):
    def all_params_are_not_set(params):
        return all([umisc.is_empty_value(param) for param in params])

    param_names_and_values = {"module": module_name, "class": class_name, "alias": plugin_alias, "kwargs": user_kwargs}
    if not all_params_are_not_set(param_names_and_values.values()):
        not_empty = {name: value for name, value in usix.iteritems(param_names_and_values)
                     if not umisc.is_empty_value(value)}
        raise Exception("For batch mode, class/module/alias/kwargs must be empty (or '-'). You have {}".format(not_empty))


class PluginContainer(object):
    def __init__(self, plugin_batch=None, skip_broken_plugins=False, plugin_dir=None):
        """
        :type plugin_batch: PluginBatch | None
        :type skip_broken_plugins: bool
        :type plugin_dir: str | None
        """

        self.plugin_instances = OrderedDict()
        self.plugin_names = set()
        self.plugin_keys = set()
        self.plugin_key_map = {}
        self.plugin_ab_infos = {}
        self.skip_broken_plugins = skip_broken_plugins
        self.broken_plugins = []
        self.plugins_dir = plugin_dir

        self.module_source_cache = mstand_umodule.ModuleSourceCache()

        if plugin_batch is None:
            logging.info("Plugin batch is not set, creating empty container")
        else:
            logging.info("Creating plugin instances from batch")
            assert isinstance(plugin_batch, PluginBatch)
            start_time = time.time()
            self.load_plugin_batch(plugin_batch)
            umisc.log_elapsed(start_time, "plugin batch loaded")

    def size(self):
        """
        :rtype: int
        """
        return len(self.plugin_instances)

    def load_plugin_batch(self, plugin_batch):
        """
        :type plugin_batch: PluginBatch
        """
        logging.info("Loading plugin batch with %d sources", len(plugin_batch.plugin_sources))
        for plugin_index, plugin_source in enumerate(plugin_batch.plugin_sources):
            logging.info("--> processing source %s", plugin_source)
            for kwargs_index, plugin_kwargs in enumerate(plugin_source.kwargs_list):
                logging.info("   --> processing set of kwargs %s", plugin_kwargs)
                self._load_one_plugin(plugin_source=plugin_source,
                                      plugin_kwargs=plugin_kwargs,
                                      source=plugin_batch.source,
                                      source_index=plugin_index,
                                      kwargs_index=kwargs_index)

    def _load_one_plugin(self, plugin_source, plugin_kwargs, source, source_index, kwargs_index):
        """
        :type plugin_source: PluginSource
        :type plugin_kwargs: PluginKwargs
        :type source:
        :type source_index
        :type kwargs_index
        :rtype: None
        """
        assert isinstance(plugin_source, PluginSource)
        assert isinstance(plugin_kwargs, PluginKwargs)

        logging.info("Creating plugin instance from %s with set %s", plugin_source, plugin_kwargs)
        try:
            plugin_instance = self._create_plugin_instance(plugin_source, plugin_kwargs, source, do_validate_pickling=True)

            alias = self._get_plugin_alias(plugin_instance, plugin_source.alias)
            key_name = PluginKey.make_name(plugin_source.module_name, plugin_source.class_name, alias)
            plugin_key = PluginKey(name=key_name, kwargs_name=plugin_kwargs.name)
            ab_info = plugin_source.ab_info
            self.add_plugin_instance(plugin_key, plugin_instance, ab_info)

        except UserModuleException as exc:
            error_details = traceback.format_exc()
            logging.error("Cannot create plugin instance: %s. Details: %s", exc, error_details)
            if self.skip_broken_plugins:
                broken_plugin_info = {
                    "error": str(exc),
                    "error_details": error_details,
                    "alias": plugin_source.alias,
                    "plugin_source": str(plugin_source),
                    "plugin_kwargs": str(plugin_kwargs),
                    "plugin_index": source_index,
                    "kwargs_index": kwargs_index
                }
                self.broken_plugins.append(broken_plugin_info)
            else:
                raise

    def unload_broken_plugin(self, plugin_id, error):
        plugin_key = self.plugin_key_map[plugin_id]
        broken_plugin_info = {
            "error": error,
            "error_details": None,
            "plugin_id": plugin_id,
            "plugin_key": plugin_key
        }

        del self.plugin_instances[plugin_id]
        del self.plugin_key_map[plugin_id]
        self.broken_plugins.append(broken_plugin_info)

    def add_plugin_instance(self, plugin_key, plugin_instance, plugin_ab_info=None):
        logging.info("Registering plugin %s in container", plugin_key)
        if plugin_key in self.plugin_keys:
            raise UserModuleException("Duplicate plugin: {}".format(plugin_key))

        plugin_id = len(self.plugin_key_map)
        self.plugin_keys.add(plugin_key)
        self.plugin_key_map[plugin_id] = plugin_key
        self.plugin_instances[plugin_id] = plugin_instance
        self.plugin_names.add(plugin_key.pretty_name())
        if plugin_ab_info is not None:
            self.plugin_ab_infos[plugin_id] = plugin_ab_info
        logging.info("Plugin %s registered with ID %d", plugin_key, plugin_id)

    @staticmethod
    def create_single_direct(plugin_key, plugin_instance):
        """
        :type plugin_key: PluginKey
        :type plugin_instance:
        :rtype: PluginContainer
        """
        assert isinstance(plugin_key, PluginKey)
        plugin_container = PluginContainer()
        plugin_container.add_plugin_instance(plugin_key, plugin_instance)
        return plugin_container

    @staticmethod
    def create_single(module_name, class_name, source=None, alias=None, kwargs=None, skip_broken_plugins=False,
                      ab_info=None, plugin_dir=None):
        """
        :type module_name: str
        :type class_name: str
        :type source: str | None
        :type alias: str | None
        :type kwargs: dict | None
        :type skip_broken_plugins: bool
        :type ab_info: PluginABInfo | None
        :type plugin_dir: str | None
        :rtype: PluginContainer
        """
        plugin_kwargs = PluginKwargs(name="default", kwargs=kwargs)

        plugin_source = PluginSource(module_name=module_name, class_name=class_name,
                                     alias=alias, kwargs_list=[plugin_kwargs], ab_info=ab_info)
        plugin_batch = PluginBatch([plugin_source], source=source)

        return PluginContainer(plugin_batch, skip_broken_plugins=skip_broken_plugins, plugin_dir=plugin_dir)

    @staticmethod
    def create_from_command_line(cli_args, batch_file, source, skip_broken_plugins=False):
        # TODO: replace to 'create_from_config' (and PluginConfig is constructed by cli_args)

        plugin_alias = cli_args.set_alias
        module_name = cli_args.module_name
        class_name = cli_args.class_name
        user_kwargs = cli_args.user_kwargs
        plugin_dir = cli_args.plugin_dir

        logging.info("plugin_alias = %s", plugin_alias)
        logging.info("module_name = %s", module_name)
        logging.info("class_name = %s", class_name)
        logging.info("user_kwargs = %s", user_kwargs)
        logging.info("plugin_dir = %s", plugin_dir)

        if batch_file:
            logging.info("Using batch mode (taking plugin list and params from BATCH).")
            validate_single_mode_params(module_name, class_name, plugin_alias, user_kwargs)

            plugin_batch = PluginBatch.load_from_file(batch_file)
            if source:
                logging.info("Using source from command line: %s", source)
                plugin_batch.source = source
            container = PluginContainer(plugin_batch, skip_broken_plugins=skip_broken_plugins, plugin_dir=plugin_dir)
        else:
            logging.info("Using single mode (taking plugin info from command line).")
            if not cli_args.module_name or not cli_args.class_name:
                raise Exception("No plugin module/class name specified. ")

            parsed_kwargs = umodule.parse_user_kwargs(user_kwargs)
            container = PluginContainer.create_single(module_name=module_name,
                                                      class_name=class_name,
                                                      source=source,
                                                      alias=plugin_alias,
                                                      kwargs=parsed_kwargs,
                                                      skip_broken_plugins=skip_broken_plugins,
                                                      plugin_dir=plugin_dir)
        return container

    def _create_plugin_instance(self, plugin_source, plugin_kwargs, source=None, do_validate_pickling=False):
        """
        :type plugin_source: PluginSource
        :type plugin_kwargs: PluginKwargs
        :type source: str | None
        :rtype:
        """
        plugin_instance = mstand_umodule.create_user_object(plugin_source.module_name,
                                                            plugin_source.class_name,
                                                            source=source,
                                                            kwargs=plugin_kwargs.kwargs,
                                                            module_source_cache=self.module_source_cache,
                                                            plugin_dir=self.plugins_dir,
                                                            do_validate_pickling=do_validate_pickling)
        return plugin_instance

    @staticmethod
    def _get_plugin_alias(plugin_instance, alias):
        """
        :type plugin_instance:
        :type alias: str | None
        :rtype: str
        """
        if alias:
            return alias
        assert plugin_instance is not None

        if not hasattr(plugin_instance, "name"):
            return None
        name = plugin_instance.name()
        if not name:
            raise Exception("Plugin name() could not be empty. ")
        return name

    def __repr__(self):
        return str(self)

    def __str__(self):
        if len(self.plugin_instances) == 1:
            plugin_name = tuple(self.plugin_names)[0]
            return "Cont({})".format(plugin_name)
        return "Cont({} plugins, {} instances total)".format(len(self.plugin_names), len(self.plugin_instances))

    def dump_broken_plugins(self, output_file):
        logging.info("Saving broken plugins info to %s", output_file)
        ujson.dump_to_file(self.broken_plugins, output_file)
