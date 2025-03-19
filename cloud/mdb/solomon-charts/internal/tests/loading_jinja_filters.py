"""
Test filtration entities which aren't satisfy the current environment
"""
import json
from pathlib import Path
import os

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


class TestLoaderJinjaFilters:

    def test_loading_modules(self):
        os.environ["YATEST"] = "1"
        modules = r.load_filters_modules()
        assert "cloud.mdb.solomon-charts.internal.lib.filters.filter_grpc" in modules
        assert all([x.startswith("cloud.mdb.solomon-charts.internal.lib.filters.filter_") for x in modules])

    def test_loading_grpc_filters(self):
        os.environ["YATEST"] = "1"
        filters = r.module_filters("cloud.mdb.solomon-charts.internal.lib.filters.filter_grpc")
        assert any([x[0] == "status_to_signal" for x in filters])
        assert any([x[0] == "status_to_color" for x in filters])

    def test_filter_functions(self):
        os.environ["YATEST"] = "1"
        loader = solomon_loader.SolomonLoader('cloud/mdb/solomon-charts/templates/')
        j_env = Environment(loader=loader, undefined=StrictUndefined,
                            variable_start_string='<<', variable_end_string='>>')
        r.load_filters(j_env)
        cfg = _load_config()
        # status_to_signal = j_env.filters["grpc_status_to_signal"]
        schema_to_list = j_env.filters["signal_to_color_schema_to_list"]
        assert cfg["common_ctx"]["grpc"]["groups"]["OK"][0] in schema_to_list(cfg["common_ctx"]["grpc"])
        assert cfg["common_ctx"]["grpc"]["groups"]["Auth"][1] in schema_to_list(cfg["common_ctx"]["grpc"])

        pp_int_api = cfg["envs"]["porto_prod"]["grpc_int_api"]
        sts = j_env.filters["grpc_status_to_signal"]
        assert sts("OK", pp_int_api["prefix"], pp_int_api["postfix"]) == pp_int_api["prefix"]+"OK"+pp_int_api["postfix"]
        assert sts("ALREADY_EXISTS", pp_int_api["prefix"], pp_int_api["postfix"]) == pp_int_api["prefix"]+"AlreadyExists"+pp_int_api["postfix"]

        stc = j_env.filters["grpc_status_to_color"]
        assert stc("OK", cfg["common_ctx"]["grpc"]) == "#008000"
        assert stc("UNKNOWN", cfg["common_ctx"]["grpc"]) == "#ff4500"
