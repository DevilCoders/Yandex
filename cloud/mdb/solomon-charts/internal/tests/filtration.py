"""
Test filtration entities which aren't satisfy the current environment
"""
import json
from pathlib import Path
import os

# pylint: disable=import-error, import-outside-toplevel
import yatest
from yatest.common import source_path

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


class TestFiltrationLogic:

    def test_loading_config(self):
        cfg = _load_config()
        assert 'envs' in cfg
        assert 'compute_preprod' in cfg['envs']
        assert 'opts' in cfg
        assert 'db' in cfg['opts']

    def test_walle_templates_compute_preprod(self):
        os.environ["YATEST"] = "1"
        env_name = "compute_preprod"
        templates = r.get_files(env_name, "cloud/mdb/solomon-charts/templates/", "alerts", "cms")
        assert len(templates) == 0

    def test_walle_templates_porto_prod(self):
        os.environ["YATEST"] = "1"
        env_name = "porto_prod"
        templates_dir = "cloud/mdb/solomon-charts/templates/"
        templates = r.get_files(env_name, templates_dir, "alerts", "cms")
        path = os.path.join(source_path(templates_dir), "alerts", "cms")
        files = os.listdir(path)
        j2_files = [f for f in files if f.endswith(".j2")]
        assert len(templates) == len(j2_files)
