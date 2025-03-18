import pytest
from user_plugins import PluginKey


# noinspection PyClassHasNoInit
class TestPluginKey:
    def test_serialize_plugin_key(self):
        key = PluginKey(name="metric_name", kwargs_name="default")

        serialized = {
            "name": "metric_name"
        }
        assert serialized == key.serialize()

    def test_deserialize_plugin_key(self):
        serialized = {
            "name": "metric_name",
            "kwargs_name": "metric_kwargs"
        }
        plugin_key = PluginKey.deserialize(serialized)
        assert plugin_key.name == "metric_name"
        assert plugin_key.kwargs_name == "metric_kwargs"

        serialized = {
            "name": "metric_name",
            "kwargs_name": ""
        }
        plugin_key = PluginKey.deserialize(serialized)
        assert plugin_key.name == "metric_name"
        assert plugin_key.kwargs_name == "default"

    def test_pretty_name(self):
        assert PluginKey("metric", "kwargs").pretty_name() == "metric(kwargs)"

    def test_make_plugin_name(self):
        assert PluginKey.make_name("mod", "cls", "alias") == "alias"
        assert PluginKey.make_name("mod", "cls", "") == "mod.cls"
        assert PluginKey.make_name("mod", "cls") == "mod.cls"

    def test_str_key(self):
        pk = PluginKey("name with spaces")
        assert pk.str_key() == "name_with_spaces"

        pk = PluginKey("name$with$dollars")
        assert pk.str_key() == "name.DOLLAR.with.DOLLAR.dollars"

        pk = PluginKey("name/with/slashes")
        assert pk.str_key() == "name.SLASH.with.SLASH.slashes"

        pk = PluginKey("name:with:colons")
        assert pk.str_key() == "name:with:colons"

        pk_with_kwargs = PluginKey("name with spaces", "kwargs with spaces")
        assert pk_with_kwargs.str_key() == "name_with_spaces_kwargs_with_spaces"

    def test_invalid_names(self):
        with pytest.raises(Exception):
            PluginKey("name%with%percents")

        with pytest.raises(Exception):
            PluginKey("name#with#hashes")

        with pytest.raises(Exception):
            PluginKey("good_name", "bad%kwargs%name")

        with pytest.raises(Exception):
            pk = PluginKey("good_name", "good_kwargs_name")
            pk.name = "bad%name%"

        with pytest.raises(Exception):
            pk = PluginKey("good_name", "good_kwargs_name")
            pk.kwargs_name = "bad^kwargs*name"

    def test_key_method(self):
        pk1 = PluginKey(name="name1", kwargs_name="kw1")
        pk2 = PluginKey(name="name1", kwargs_name="kw2")
        pk3 = PluginKey(name="name1", kwargs_name="kw1")
        assert len({pk1, pk2, pk3}) == 2
