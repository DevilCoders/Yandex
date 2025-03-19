"""
Test creating solomon entities from templates
"""
import json
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
    with Path(fp).open() as f:
        return f.read()


loader = solomon_loader.SolomonLoader('cloud/mdb/solomon-charts/templates/')
j_env = Environment(loader=loader, undefined=StrictUndefined,
                    variable_start_string='<<', variable_end_string='>>')


class TestHealthGen:

    def test_loading_config(self):
        cfg = _load_config()
        assert 'envs' in cfg
        assert 'compute_prod' in cfg['envs']
        assert 'opts' in cfg
        assert 'db' in cfg['opts']

    def test_render_contexts_compute_prod_graph_cluster_status_pie(self):
        pie_json = _load_file("cloud/mdb/solomon-charts/templates/graphs/health/clusters-status-pie.j2")
        cfg = _load_config()
        ctxs = r.get_render_contexts('compute_prod', pie_json, cfg)
        assert len(ctxs) == len(cfg['opts']['db'])

    def test_render_contexts_compute_prod_graph_cluster_status(self):
        js = _load_file("cloud/mdb/solomon-charts/templates/graphs/health/clusters-status.j2")
        cfg = _load_config()
        ctxs = r.get_render_contexts('compute_prod', js, cfg)
        assert len(ctxs) == len(cfg['opts']['db']) * len(cfg['opts']['health_status'].keys())
        assert ctxs[0]["health_status"] == "total"
        assert ctxs[0]["health_status_val"] == "Total"

    def test_render_entity_compute_prod_graph_cluster_status_pie(self):
        pie_json = _load_file("cloud/mdb/solomon-charts/templates/graphs/health/clusters-status-pie.j2")
        cfg = _load_config()
        ctxs = r.get_render_contexts('compute_prod', pie_json, cfg)
        for ctx in ctxs:
            out = r.render_entity(pie_json, ctx, j_env)
            assert "<<" not in out
            assert ">>" not in out
            graph = json.loads(out)
            if ctx["db"] == "postgresql":
                assert graph["id"] == "mdb-prod-health-postgresql-clusters-pie"

    def test_render_entity_compute_prod_graph_cluster_status(self):
        pie_json = _load_file("cloud/mdb/solomon-charts/templates/graphs/health/clusters-status.j2")
        cfg = _load_config()
        ctxs = r.get_render_contexts('compute_prod', pie_json, cfg)
        for ctx in ctxs:
            out = r.render_entity(pie_json, ctx, j_env)
            assert "<<" not in out
            assert ">>" not in out
            graph = json.loads(out)
            if ctx["db"] == "postgresql" and ctx["health_status_val"] == "alive":
                assert graph["id"] == "mdb-prod-health-postgresql-clusters-alive"
