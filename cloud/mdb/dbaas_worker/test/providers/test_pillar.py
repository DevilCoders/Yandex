import json

from cloud.mdb.dbaas_worker.internal.providers.pillar import pillar_without_keys


def test_pillar_without_keys():
    orig = {"cert.key": "some_key", "cert.crt": "some_crt", "cert.expiration": "today"}
    remove_keys = ("cert.key", "cert.crt")
    path = []
    desired_pillar = {"cert.expiration": "today"}
    keys_removed = pillar_without_keys(orig, path, remove_keys)

    assert json.dumps(desired_pillar) == json.dumps(keys_removed)

    orig = {"data": {"use_pgsync": True, "use_mdb_metrics": True}}
    remove_keys = ("use_pgsync",)
    path = ["data"]
    desired_pillar = {"data": {"use_mdb_metrics": True}}
    keys_removed = pillar_without_keys(orig, path, remove_keys)

    assert json.dumps(desired_pillar) == json.dumps(keys_removed)
