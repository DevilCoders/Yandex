#!/usr/bin/python3

import argparse
import filecmp
import io
import json
import logging
import os
import tempfile
from typing import Dict, List
import subprocess
import shutil
import sys
import yaml
import string


def run_subprocess_with_retries(command: List[str], retries: int = 3):
    for retry in range(retries):
        try:
            return subprocess.check_output(command)
        except subprocess.CalledProcessError as e:
            logging.warning(f"[Try {retry+1}/{retries}] Error occurred while running {' '.join(command)!r}: exit code {e.returncode}")
            logging.warning(f"Stdout: {e.stdout!r}")
            logging.warning(f"Stderr: {e.stderr!r}")
            if retry + 1 == retries:
                raise e


def generate_iam_token():
    out = run_subprocess_with_retries(["/root/yandex-cloud/bin/yc", "iam", "create-token", "--format", "json"])
    data = json.loads(out)
    os.environ["YC_TOKEN"] = data["iam_token"]


def load_yaml(fname: str):
    path = os.path.dirname(__file__)
    with open(os.path.join(path, fname)) as yaml_file:
        return yaml.safe_load(yaml_file)


def append_yaml(obj, yaml_file: io.IOBase):
    yaml.safe_dump(obj, yaml_file)
    yaml_file.flush()


def cache_file(args, skm_cfg_file: tempfile.NamedTemporaryFile, name: str) -> bool:
    file_path = os.path.join(args.cache_dir, name)
    if os.path.exists(file_path) and filecmp.cmp(file_path, skm_cfg_file.name):
        return True

    shutil.copy(skm_cfg_file.name, file_path)
    return False


def load_cached_file(args, skm_cfg_file: tempfile.NamedTemporaryFile, name: str):
    file_path = os.path.join(args.cache_dir, name)

    shutil.copy(file_path, skm_cfg_file.name)
    skm_cfg_file.seek(0, os.SEEK_END)


def render_config_var(dct, path, raw_args):
    if isinstance(path, str):
        path = path.split(":")

    key = path.pop(0)
    val = dct.get(key)
    if val is None:
        return

    if path:
        render_config_var(val, path, raw_args)
        return

    if "$" in val:
        dct[key] = string.Template(val).safe_substitute(raw_args)


def build_base(raw_args: Dict):
    base = load_yaml("base.yaml")
    render_config_var(base, "yc_kms:api_endpoint", raw_args)
    render_config_var(base, "yc_kms:iam_private_endpoint", raw_args)
    render_config_var(base, "yc_kms:kms_private_endpoint", raw_args)
    render_config_var(base, "kek:kms:key_uri", raw_args)

    return base


def build_secrets(args, raw_args: Dict):
    secrets = load_yaml(args.secrets_file)
    for secret in secrets["secrets"]:
        render_config_var(secret, "source:yav:secret_id", raw_args)
        render_config_var(secret, "source:yav:key", raw_args)
        render_config_var(secret, "source:file", raw_args)

    return secrets


def main():
    if "TFCHECK" in os.environ:
        print(json.dumps({"skm": ""}))
        sys.exit(0)

    raw_args = json.load(sys.stdin)
    args = argparse.Namespace(**raw_args)

    generate_iam_token()
    os.makedirs(args.cache_dir, exist_ok=True)

    base = build_base(raw_args)
    secrets = build_secrets(args, raw_args)

    with tempfile.NamedTemporaryFile(mode="w", prefix="yc-generate-skm") as skm_cfg_file:
        # Regenerate DEK only if base config (endpoints, key id) is changed
        # Otherwise terraform will update IG each time plan/apply is called
        # This is a small violation of principle that external data source
        # should have no visible side effects, though.
        # NOTE: that if local cache is absent, IG will still be updated
        append_yaml(base, skm_cfg_file)
        need_regenerate_dek = not cache_file(args, skm_cfg_file, "base.yaml")

        if need_regenerate_dek:
            dek = subprocess.check_output(["skm", "generate-dek", "--config", skm_cfg_file.name], env=os.environ)
            append_yaml({"encrypted_dek": str(dek.strip(), encoding="ascii")}, skm_cfg_file)

            cache_file(args, skm_cfg_file, "with-dek.yaml")
        else:
            # If we already generated dek and base.yaml did not change, use previous file
            load_cached_file(args, skm_cfg_file, "with-dek.yaml")

        append_yaml(secrets, skm_cfg_file)
        cache_file(args, skm_cfg_file, "final.yaml")

        subprocess.check_call(["skm", "encrypt-md", "--config", skm_cfg_file.name, "--format", "json"])


if __name__ == "__main__":
    main()
