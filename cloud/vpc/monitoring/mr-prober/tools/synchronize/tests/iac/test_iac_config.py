import pathlib

import tools.synchronize.iac.models as iac_models


def test_config_loaded(testdata):
    config = iac_models.Config.load_from_file(pathlib.Path("config.yaml"))

    assert config.environments.keys() == {"local"}

    local = config.environments["local"]
    assert local.endpoint == "http://127.0.0.1:8080"
    assert len(local.clusters) == 2
    assert local.clusters[0] == "clusters/prod/*/cluster.yaml"
    assert local.clusters[1] == "clusters/preprod/*/cluster.yaml"
    assert len(local.probers) == 1
    assert local.probers[0] == "probers/network/**/prober.yaml"
