import pathlib
from unittest.mock import patch, MagicMock

import pytest

import api.models
import tools.synchronize.data.models as data_models
import tools.synchronize.iac.models as iac_models
from database.models import UploadProberLogPolicy
from tools.synchronize.data.collection import ObjectsCollection
from tools.synchronize.iac import errors


def test_prober_config_load_from_iac_expose_clusters_list(testdata):
    iac_config = iac_models.ProberConfig(
        is_prober_enabled=True,
        clusters=[
            "clusters/some-environment/**/cluster.yaml",
        ]
    )
    configs = list(data_models.ProberConfig.load_from_iac(iac_config))
    assert {config.cluster.arcadia_path for config in configs} == {
        "clusters/some-environment/variables/cluster.yaml",
        "clusters/some-environment/slug/cluster.yaml",
        "clusters/some-environment/iac-untracked-variables/cluster.yaml"
    }

    for config in configs:
        assert config.id is None
        assert config.is_prober_enabled == True
        assert config.timeout_seconds is None
        assert config.interval_seconds is None
        assert config.s3_logs_policy is None
        assert config.hosts_re is None


def test_prober_config_load_from_iac(testdata):
    iac_config = iac_models.ProberConfig(
        is_prober_enabled=True,
        interval_seconds=60,
        timeout_seconds=10,
        s3_logs_policy=UploadProberLogPolicy.FAIL,
        hosts_re="agent*"
    )
    configs = list(data_models.ProberConfig.load_from_iac(iac_config))

    assert len(configs) == 1
    config = configs[0]

    assert config.id is None
    assert config.is_prober_enabled == True
    assert config.timeout_seconds == 10
    assert config.interval_seconds == 60
    assert config.s3_logs_policy == UploadProberLogPolicy.FAIL
    assert config.hosts_re == "agent*"
    assert config.cluster is None


def test_prober_load_from_iac(testdata):
    prober = data_models.Prober.load_from_iac(pathlib.Path("probers/network/dns/dns-resolve-yandex-host/prober.yaml"))
    assert prober.arcadia_path == "probers/network/dns/dns-resolve-yandex-host/prober.yaml"
    assert prober.name == "[DNS] Resolve yandex host"
    assert prober.slug == "dns-resolve-yandex-host"
    assert prober.description == "Send AAAA query about storage-int.mds.yandex.net"
    assert isinstance(prober.runner, api.models.BashProberRunner)
    assert prober.runner.command == "./dns.sh storage-int.mds.yandex.net"

    assert len(prober.files) == 1
    assert prober.files[0].relative_file_path == "dns.sh"
    assert prober.files[0].content.startswith(b"#!/bin/bash")

    assert len(prober.configs) == 2


def test_prober_load_from_iac_with_conflicted_files(testdata):
    with pytest.raises(errors.IacDataError):
        data_models.Prober.load_from_iac(pathlib.Path("probers/error/conflicted-files/prober.yaml"))


def test_prober_update_in_api_creates_valid_prober(testdata):
    prober_without_id = data_models.Prober.load_from_iac(
        pathlib.Path("probers/network/dns/dns-resolve-yandex-host/prober.yaml")
    )
    prober = prober_without_id.copy(update={"id": 1})
    new_prober = prober.copy(update={"description": "new description"})
    expected_prober_update_request = api.models.UpdateProberRequest(
        name=new_prober.name,
        manually_created=False,
        arcadia_path=new_prober.arcadia_path,
        slug=new_prober.slug,
        description=new_prober.description,
        runner=new_prober.runner,
    )
    client = MagicMock()
    client.probers.update = MagicMock()

    with patch.object(data_models.Prober, "load_from_api_model", new=lambda *args: new_prober):
        with pytest.raises(AssertionError):
            prober_without_id.update_in_api(client, None, new_prober)

        prober_from_api = prober.update_in_api(client, None, new_prober)
        assert new_prober.equals(prober_from_api)

    client.probers.update.assert_called_once_with(prober.id, expected_prober_update_request)


def test_prober_load_from_api_model_twice(testdata):
    api_prober_model = api.models.Prober(
        id=1,
        name="test",
        slug="test",
        description="Test",
        manually_created=False,
        arcadia_path="probers/test/prober.yaml",
        runner=api.models.BashProberRunner(command="./test"),
        files=[api.models.ProberFile(
            id=1, relative_file_path="test", is_executable=True, md5_hexdigest="8c9e2b29384526d6debb7e283158ff88"
        )],
        configs=[],
    )

    client = MagicMock()
    client.probers.get = MagicMock()

    with patch.object(client.probers, "get_file_content", return_value="bla-bla-bla") as get_prober_file_content_mock:
        collection = ObjectsCollection()
        # 1. Load prober from API model for the first time
        data_prober = data_models.Prober.load_from_api_model(client, api_prober_model, collection)
        # 2. Put it into the objects collection
        collection.probers[data_prober.arcadia_path] = data_prober
        # 3. Try to load prober from API model for the second time. Passing the collection should save the time,
        # because prober should be returned from the cache
        data_models.Prober.load_from_api_model(client, api_prober_model, collection)

    # 4. Check that prober files are loaded from API only once
    assert get_prober_file_content_mock.call_count == 1
