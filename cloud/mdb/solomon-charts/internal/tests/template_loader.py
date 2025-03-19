"""
Test creating solomon entities from templates
"""
import json
import os
from pathlib import Path


# pylint: disable=import-error, import-outside-toplevel
import yatest

# pylint: disable=import-error, import-outside-toplevel
from jinja2 import Environment, StrictUndefined
from importlib import import_module
r = import_module("cloud.mdb.solomon-charts.internal.lib.render")
solomon_loader = import_module("cloud.mdb.solomon-charts.internal.lib.solomon_loader")

_cfg_file = "cloud/mdb/solomon-charts/solomon.json"


def _load_config():
    return json.loads(_load_file(_cfg_file))


def _load_file(file_path: str):
    fp = yatest.common.source_path(file_path)
    return Path(fp).read_text()


class TestMetaDBAlertProgram:

    def test_render_alert_program(self):
        os.environ["YATEST"] = "1"
        alert = _load_file("cloud/mdb/solomon-charts/templates/alerts/metadb/create-cluster-timing.j2")
        cfg = _load_config()
        ctxs = r.get_render_contexts('compute_prod', alert, cfg)
        loader = solomon_loader.SolomonLoader('cloud/mdb/solomon-charts/templates/')
        j_env = Environment(loader=loader, undefined=StrictUndefined,
                            variable_start_string='<<', variable_end_string='>>')
        for ctx in ctxs:
            out = r.render_entity(alert, ctx, j_env)
            assert "<<" not in out
            assert ">>" not in out
            alert_body = json.loads(out)
            if ctx["db"] == "mongodb":
                assert alert_body["id"] == "mdb-prod-metadb-mongodb-create-cluster-timing"
            assert "warn_if" in alert_body["type"]["expression"]["program"]
