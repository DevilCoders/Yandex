# coding=utf-8

from mstand_structs import SqueezeVersions
from mstand_structs import squeeze_versions
from yaqutils import nirvana_helpers


# noinspection PyClassHasNoInit
class TestSqueezeVersions:
    def test_clone(self):
        versions = SqueezeVersions({"web": 3, "touch": 2}, common=1, history=123, workflow_url="workflow_url")
        clone = versions.clone(with_history=True)
        assert clone.service_versions == versions.service_versions
        assert clone.common == versions.common
        assert clone.history == versions.history
        assert clone.python_version == versions.python_version
        assert clone.revision == versions.revision
        assert clone.workflow_url == versions.workflow_url

    def test_clone_without_history(self):
        versions = SqueezeVersions({"web": 3, "touch": 2}, common=1, history=123)
        clone = versions.clone(with_history=False)
        assert clone.service_versions == versions.service_versions
        assert clone.common == versions.common
        assert clone.history is None

    def test_clone_specific_service(self):
        versions = SqueezeVersions({"web": 3, "touch": 2}, common=1, history=123)
        clone = versions.clone(service="touch")
        assert clone.service_versions == {"touch": 2}
        assert clone.common == versions.common
        assert clone.history == versions.history

    def test_serialize(self, monkeypatch):
        revision = 12345
        monkeypatch.setattr(squeeze_versions, "get_revision", lambda: revision)

        versions = SqueezeVersions({"web": 3, "touch": 2}, common=1, revision=None, workflow_url="workflow_url")
        serialized = versions.serialize()
        expected = {
            "web": 3, "touch": 2, "_common": 1,
            "_python": squeeze_versions.get_python_version(),
            "_revision": revision,
            "_workflow_url": "workflow_url",
        }
        assert serialized == expected

    def test_serialize_with_history(self, monkeypatch):
        revision = 12345
        workflow_url = "workflow_url"
        monkeypatch.setattr(squeeze_versions, "get_revision", lambda: revision)
        monkeypatch.setattr(nirvana_helpers, "get_nirvana_workflow_url", lambda: workflow_url)

        versions = SqueezeVersions({"web": 111, "touch": 222}, common=333, history=444)
        serialized = versions.serialize()
        expected = {
            "web": 111, "touch": 222, "_common": 333, "_history": 444,
            "_python": squeeze_versions.get_python_version(),
            "_revision": revision,
            "_workflow_url": workflow_url,
        }
        assert serialized == expected

    def test_deserialize(self):
        serialized = {
            "web": 3, "touch": 2, "_common": 1, "_python": "3.6", "_revision": 12345, "_workflow_url": "workflow_url",
        }
        versions = SqueezeVersions.deserialize(serialized)
        assert versions.common == 1
        assert versions.history is None
        assert versions.service_versions == {"web": 3, "touch": 2}
        assert versions.python_version == "3.6"
        assert versions.revision == 12345
        assert versions.workflow_url == "workflow_url"

    def test_deserialize_with_history(self):
        serialized = {"web": 3, "touch": 2, "_common": 1, "_history": 4}
        versions = SqueezeVersions.deserialize(serialized)
        assert versions.common == 1
        assert versions.history == 4
        assert versions.service_versions == {"web": 3, "touch": 2}

    def test_is_older_services(self):
        v1 = SqueezeVersions({"web": 10, "touch": 10}, common=1, history=1, filters=1)
        v2 = SqueezeVersions({"web": 1, "touch": 10}, common=1, history=1, filters=1)
        v3 = SqueezeVersions({"web": 10, "touch": 1}, common=1, history=1, filters=1)
        v4 = SqueezeVersions({"web": 1, "touch": 1}, common=1, history=1, filters=1)

        assert not v1.is_older_than(v1)
        assert not v1.is_older_than(v2)
        assert not v1.is_older_than(v3)
        assert not v1.is_older_than(v4)

        assert v2.is_older_than(v1)
        assert not v2.is_older_than(v2)
        assert v2.is_older_than(v3)
        assert not v2.is_older_than(v4)

        assert v3.is_older_than(v1)
        assert v3.is_older_than(v2)
        assert not v3.is_older_than(v3)
        assert not v3.is_older_than(v4)

        assert v4.is_older_than(v1)
        assert v4.is_older_than(v2)
        assert v4.is_older_than(v3)
        assert not v4.is_older_than(v4)

    def test_is_older_common(self):
        v1 = SqueezeVersions({"web": 111, "touch": 222}, common=333, history=444, filters=1)
        v2 = SqueezeVersions({"web": 111, "touch": 222}, common=1, history=444, filters=1)
        v3 = SqueezeVersions({"web": 111, "touch": 222}, common=500, history=444, filters=1)
        assert not v1.is_older_than(v1)
        assert not v1.is_older_than(v2)
        assert v1.is_older_than(v3)

        assert v2.is_older_than(v1)
        assert not v2.is_older_than(v2)
        assert v2.is_older_than(v3)

        assert not v3.is_older_than(v1)
        assert not v3.is_older_than(v2)
        assert not v3.is_older_than(v3)

    def test_is_older_history(self):
        v1 = SqueezeVersions({"images": 111}, common=333, history=444, filters=1)
        v2 = SqueezeVersions({"images": 111}, common=333, history=1, filters=1)
        v3 = SqueezeVersions({"images": 111}, common=333, history=500, filters=1)
        assert v1.is_older_than(v3)
        assert v2.is_older_than(v1)
        assert v2.is_older_than(v3)

    def test_is_older_filters(self):
        v1 = SqueezeVersions({"images": 111}, common=333, history=1, filters=444)
        v2 = SqueezeVersions({"images": 111}, common=333, history=1, filters=1)
        v3 = SqueezeVersions({"images": 111}, common=333, history=1, filters=1500)
        assert v1.is_older_than(v3)
        assert v2.is_older_than(v1)
        assert v2.is_older_than(v3)

    def test_is_older_ignore_history(self):
        v1 = SqueezeVersions({"video": 222}, common=333, history=444)
        v2 = SqueezeVersions({"video": 222}, common=500, history=1)
        v3 = SqueezeVersions({"video": 222}, common=1, history=500)

        assert not v1.is_older_than(v1, check_history=False)
        assert v1.is_older_than(v2, check_history=False)
        assert not v1.is_older_than(v3, check_history=False)

        assert not v2.is_older_than(v1, check_history=False)
        assert not v2.is_older_than(v2, check_history=False)
        assert not v2.is_older_than(v3, check_history=False)

        assert v3.is_older_than(v1, check_history=False)
        assert v3.is_older_than(v2, check_history=False)
        assert not v3.is_older_than(v3, check_history=False)

    def test_is_older_different(self):
        v1 = SqueezeVersions({"web": 2}, common=1, filters=1)
        v2 = SqueezeVersions({"watchlog": 1, "web": 2}, common=1, filters=1)
        assert not v1.is_older_than(v2)
        assert not v2.is_older_than(v1)

    def test_newest_versions(self):
        v1 = SqueezeVersions({"web": 10, "touch": 10}, common=15)
        v2 = SqueezeVersions({"web": 1, "touch": 20}, common=5)
        v3 = SqueezeVersions({"web": 10, "touch": 1}, common=50, history=1)
        v4 = SqueezeVersions({"web": 1, "touch": 1}, common=1, history=5, filters=1)
        v_list = [v1, v2, v3, v4]
        newest_versions = SqueezeVersions.get_newest(v_list)
        assert not any(newest_versions.is_older_than(v) for v in v_list)
        assert newest_versions.common == 50
        assert newest_versions.history == 5
        assert newest_versions.filters == 1
        assert newest_versions.service_versions == {"web": 10, "touch": 20}

    def test_oldest_versions(self):
        v1 = SqueezeVersions({"web": 10, "touch": 10}, common=15)
        v2 = SqueezeVersions({"web": 1, "touch": 20}, common=5)
        v3 = SqueezeVersions({"web": 10, "touch": 15}, common=50, history=1)
        v4 = SqueezeVersions({"web": 5, "touch": 1}, common=1, history=5, filters=1)
        v_list = [v1, v2, v3, v4]
        oldest_versions = SqueezeVersions.get_oldest(v_list)
        print(oldest_versions)
        assert any(oldest_versions.is_older_than(v) for v in v_list)
        assert oldest_versions.common == 1
        assert oldest_versions.history == 1
        assert oldest_versions.filters == 1
        assert oldest_versions.service_versions == {"web": 1, "touch": 1}

    def test_is_empty(self):
        empty_version = SqueezeVersions()
        partly_empty_version = SqueezeVersions({}, common=None)
        not_empty_version = SqueezeVersions({"web": 1, "touch": 20}, common=5)
        assert empty_version.is_empty()
        assert partly_empty_version.is_empty()
        assert not not_empty_version.is_empty()

    def test_python_version_influence(self):
        v1 = SqueezeVersions({"web": 10, "touch": 20}, common=30, history=40, python_version="2.7", revision=1, workflow_url="wu_1")
        v2 = SqueezeVersions({"web": 10, "touch": 20}, common=30, history=40, python_version="3.7", revision=1, workflow_url="wu_1")
        assert not v1.is_older_than(v2)
        assert not v1.is_older_than(v2, check_history=False)
        assert not v2.is_older_than(v1)
        assert not v2.is_older_than(v1, check_history=False)

    def test_revision_influence(self):
        v1 = SqueezeVersions({"web": 10, "touch": 20}, common=30, history=40, python_version="3.7", revision=1, workflow_url="wu_1")
        v2 = SqueezeVersions({"web": 10, "touch": 20}, common=30, history=40, python_version="3.7", revision=2, workflow_url="wu_1")
        assert not v1.is_older_than(v2)
        assert not v1.is_older_than(v2, check_history=False)
        assert not v2.is_older_than(v1)
        assert not v2.is_older_than(v1, check_history=False)

    def test_workflow_url_influence(self):
        v1 = SqueezeVersions({"web": 10, "touch": 20}, common=30, history=40, python_version="3.7", revision=1, workflow_url="wu_1")
        v2 = SqueezeVersions({"web": 10, "touch": 20}, common=30, history=40, python_version="3.7", revision=1, workflow_url="wu_2")
        assert not v1.is_older_than(v2)
        assert not v1.is_older_than(v2, check_history=False)
        assert not v2.is_older_than(v1)
        assert not v2.is_older_than(v1, check_history=False)
