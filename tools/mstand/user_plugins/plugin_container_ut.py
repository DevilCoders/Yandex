import os
import pytest

from user_plugins import PluginABInfo
from user_plugins import PluginBatch
from user_plugins import PluginContainer
from user_plugins import PluginKwargs
from user_plugins import PluginSource

import mstand_utils.mstand_module_helpers as mstand_umodule


@pytest.fixture
def source_path(request):
    try:
        import yatest.common  # noqa
        return str(yatest.common.source_path("tools/mstand/user_plugins"))
    except:
        return str(request.config.rootdir.join("user_plugins"))


# noinspection PyClassHasNoInit
class TestPluginContainer:
    def test_plugin_container_create(self):
        pkwargs1 = PluginKwargs(name="sample_kwargs", kwargs={"single_param": 1})

        assert "sample_kwargs" in str(pkwargs1)
        assert "single_param" in str(pkwargs1)

        ab_info = PluginABInfo("group", "elite", "description", "hname")

        psource1 = PluginSource("user_plugins.plugin_tests",
                                "PluginSampleOne",
                                alias="One",
                                kwargs_list=[pkwargs1],
                                ab_info=ab_info)

        assert psource1.ab_info is not None

        assert "One" in str(psource1)

        pkwargs2 = PluginKwargs(name="sample_kwargs", kwargs={"abc": 100, "xyz": 500})
        psource2 = PluginSource("user_plugins.plugin_tests",
                                "PluginSampleTwo",
                                alias="Two",
                                kwargs_list=[pkwargs2])

        pbatch = PluginBatch(plugin_sources=[psource1, psource2])
        pcont = PluginContainer(plugin_batch=pbatch)
        assert "2 plugins" in str(pcont)

    def test_plugin_container_duplicates(self):
        pkwargs = PluginKwargs(name="sample_kwargs", kwargs={"single_param": 1})

        module_name = "user_plugins.plugin_tests"
        class_name = "PluginSampleOne"
        psource1 = PluginSource(module_name=module_name, class_name=class_name, alias="One", kwargs_list=[pkwargs])

        psource2 = PluginSource(module_name=module_name, class_name=class_name, alias="One", kwargs_list=[pkwargs])

        pbatch = PluginBatch(plugin_sources=[psource1, psource2])
        with pytest.raises(Exception):
            PluginContainer(plugin_batch=pbatch)

    def test_plugin_container_broken_plugins(self):
        psource_good = PluginSource("user_plugins.plugin_tests", "PluginSample", alias="Good")
        psource_bad = PluginSource("user_plugins.plugin_tests", "NonExistingPlugin", alias="BadNonExisting")
        bad_kwargs = PluginKwargs("bad_kwargs", {"unknown_param": "some_value"})
        bad_kwargs_list = [bad_kwargs]
        psource_bad_existing = PluginSource("user_plugins.plugin_tests", "PluginSample", alias="BadExisting",
                                            kwargs_list=bad_kwargs_list)

        pbatch = PluginBatch(plugin_sources=[psource_good, psource_bad, psource_bad_existing])
        with pytest.raises(Exception):
            PluginContainer(plugin_batch=pbatch)

        pcont_with_skip = PluginContainer(plugin_batch=pbatch, skip_broken_plugins=True)
        assert len(pcont_with_skip.plugin_instances) == 1
        assert len(pcont_with_skip.broken_plugins) == 2

    def test_plugin_dir_change_location(self, tmpdir, source_path):
        plugin_dir = str(tmpdir.join("plugins/user_module"))

        container = PluginContainer.create_single("plugin_tests",
                                                  "PluginSample",
                                                  source=source_path,
                                                  plugin_dir=plugin_dir)

        plugin_subdir = mstand_umodule.hash_source_path(source_path)
        user_module_path = os.path.join(plugin_dir, plugin_subdir, "user_module")
        assert os.path.exists(os.path.join(user_module_path, "__init__.py"))
        assert os.path.exists(os.path.join(user_module_path, "plugin_tests.py"))

        instance = container.plugin_instances[0]
        assert len(container.plugin_instances) == 1
        assert "user_module.plugin_tests.PluginSample" in str(instance)
