#!/usr/bin/env python3

import argparse
import os
import json
import subprocess
import sys
import shutil
import tempfile
import textwrap
from typing import List
import yaml


PERSISTENT_PLUGIN_DIR = "/var/tmp/tfcheck/plugins"
CREATOR_PLUGIN_DIR = "/root/.terraform.d/plugins"
TFVARS_FILE_NAME = "tfcheck.tfvars.json"
TF_CLI_CONFIG_FILE = ".terraformrc"
TF_CREATOR_FILES = ["terraform_providers.tf.json"]
TFFILES = {
    "state.tf": textwrap.dedent("""\
        terraform {
            backend "local" {}
        }"""),
    "provider.tf": textwrap.dedent("""\
        variable "conductor_token" {
            type = string
            description = "Conductor token to validate ytr states. Specify via TF_VAR_conductor_token"
        }

        provider "ycp" {
            ycp_profile = var.ycp_profile
        }
        provider "ytr" {
            conductor_token = var.conductor_token
        }
        provider "yandex" {
            endpoint = var.yc_endpoint
        }"""),
}


class TerraformChecker:
    def __init__(self, args):
        self.args = args
        self.tmpdir = None

    def __enter__(self):
        self.tmpdir = tempfile.TemporaryDirectory(prefix="mr-prober-tfcheck-")
        print("Temporary directory:", self.tmpdir.name)
        return self

    def __exit__(self, *excinfo):
        if self.args.save:
            shutil.copytree(self.tmpdir.name, self.args.save)

        self.tmpdir.cleanup()

    def run(self):
        cluster_data = self.load_yaml(self.args.cluster_file)
        recipe_dir = os.path.dirname(cluster_data["recipe"])
        recipe_data = self.load_yaml(cluster_data["recipe"])

        variables = cluster_data["variables"]
        with open(os.path.join(self.tmpdir.name, TFVARS_FILE_NAME), "w") as tfvars_file:
            json.dump(variables, tfvars_file)

        with open(self.creatorpath("files", TF_CLI_CONFIG_FILE)) as cli_cfg_file:
            cli_cfg = cli_cfg_file.read()
            cli_cfg = cli_cfg.replace(CREATOR_PLUGIN_DIR, PERSISTENT_PLUGIN_DIR)
        with open(self.tmppath(TF_CLI_CONFIG_FILE), "w") as cli_cfg_file:
            cli_cfg_file.write(cli_cfg)

        for fname in TF_CREATOR_FILES:
            shutil.copy2(self.creatorpath("files", fname), self.tmppath(fname))
        for tffname, tfcontent in TFFILES.items():
            with open(os.path.join(self.tmpdir.name, tffname), "w") as tf_file:
                print(tfcontent, file=tf_file)

        for file_spec in recipe_data["files"]:
            excluded_files = file_spec.pop("exclude", [])
            recursive = file_spec.pop("recursive", False)
            if set(file_spec.keys()) != {"directory"}:
                raise ValueError(("File spec {!r} is unsupported, only directories can be"
                                  " used by tfcheck").format(file_spec))

            self.copy_tree(recipe_dir, file_spec["directory"], excluded_files,
                           recursive=recursive)

        env = os.environ.copy()
        env["TFCHECK"] = "1"

        # Cache some results of terraform to avoid downloading
        # plugins every time we're running tfcheck.py
        os.makedirs(PERSISTENT_PLUGIN_DIR, exist_ok=True)
        env["TF_PLUGIN_CACHE_DIR"] = PERSISTENT_PLUGIN_DIR
        env["TF_CLI_CONFIG_FILE"] = os.path.join(self.tmpdir.name, TF_CLI_CONFIG_FILE)

        subprocess.run(["terraform", "init"], cwd=self.tmpdir.name, env=env)
        subprocess.run(["terraform", "plan", "-var-file=" + TFVARS_FILE_NAME,
                        "-input=false"], cwd=self.tmpdir.name, env=env)

    def copy_tree(self, recipe_dir: str, copy_dir: str, excluded_files: List[str],
                  recursive: bool):
        def copy_no_overwrite(srcpath, dstpath):
            if os.path.exists(dstpath):
                raise ValueError("Destination file {!r} already exists".format(dstpath))

            shutil.copy2(srcpath, dstpath)

        path = self.datapath(os.path.join(recipe_dir, copy_dir))
        for name in os.listdir(path):
            srcpath = os.path.join(path, name)
            dstpath = self.tmppath(name)
            if not os.path.isdir(srcpath):
                if name not in excluded_files:
                    copy_no_overwrite(srcpath, dstpath)
                continue

            if not recursive:
                continue

            excluded_paths = [os.path.join(srcpath, name) for name in excluded_files]
            def ignore(subpath: str, names: List[str]) -> List[str]:
                return [name for name in names if os.path.join(srcpath, subpath, name) in excluded_paths]

            shutil.copytree(srcpath, dstpath, ignore=ignore, copy_function=copy_no_overwrite)

    def load_yaml(self, filepath):
        with open(self.datapath(filepath)) as yaml_file:
            return yaml.safe_load(yaml_file)

    def datapath(self, *relpath) -> str:
        return os.path.join(self.args.data_path, *relpath)

    def creatorpath(self, *relpath) -> str:
        return os.path.join(self.args.creator_path, *relpath)

    def tmppath(self, *relpath) -> str:
        return os.path.join(self.tmpdir.name, *relpath)


def deduce_mr_prober_path() -> str:
    tfcheck_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
    return os.path.normpath(os.path.join(tfcheck_dir, ".."))


def parse_args():
    mr_prober_path = deduce_mr_prober_path()

    parser = argparse.ArgumentParser(
        description="Combines all files from cluster & recipe directories and runs terraform plan")
    parser.add_argument("--save", metavar="DIR", help="save a copy to a resulting dir")
    parser.add_argument("--creator-path", metavar="PATH", dest="creator_path",
                        default=os.path.join(mr_prober_path, "creator"),
                        help="/path/to/mr-prober/creator")
    parser.add_argument("--data-path", metavar="PATH", dest="data_path",
                        default=os.path.join(mr_prober_path, ".data"),
                        help="/path/to/mr-prober/.data")
    parser.add_argument("cluster_file", help="path/to/cluster.yaml")
    return parser.parse_args()


def main():
    args = parse_args()
    with TerraformChecker(args) as tfcheck:
        tfcheck.run()


if __name__ == "__main__":
    main()
