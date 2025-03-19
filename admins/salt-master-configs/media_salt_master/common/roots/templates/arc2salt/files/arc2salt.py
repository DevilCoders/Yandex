#!/usr/bin/env python3

import argparse
import json
import logging
import os
import requests
import shutil
import subprocess
import tempfile
import yaml

from contextlib import contextmanager

API_URL = "https://a.yandex-team.ru/api"
CACHE_PATH = "/var/cache/salt/arc2salt.json"
SALT_DYN_CONFIGS_PATH = "/srv/salt/configs/"
SALT_CHECKOUTS_PATH = "/srv/salt/checkouts/"
ARC_REPO_PATH = "/mnt/arc/arcadia"
ARC_DIR_PATH = "/mnt/arc"
ARC_BINARY_PATH = "/usr/bin/arc"
CONFIG_PATH = "/etc/yandex/salt/arc2salt.yaml"


def is_in_cache(branch, commit, cache):
    return branch in cache.keys() and cache[branch]["commit_id"] == commit


def is_arc_mounted():
    try:
        real_repo_path = (
            subprocess.check_output(["readlink", "-f", ARC_REPO_PATH]).decode().strip()
        )
    except subprocess.CalledProcessError:
        logging.debug("Create %r", ARC_DIR_PATH)
        os.mkdir(ARC_DIR_PATH)
        return False

    with open("/proc/mounts") as pm:
        for line in pm.readlines():
            mount_line = line.split()
            if mount_line[0] == "arc":
                if real_repo_path == mount_line[1]:
                    return True
    return False


def mount_arc():
    mnt_path = os.path.abspath(os.path.join(ARC_REPO_PATH, os.pardir))
    logging.debug("try to mount arc on %r", mnt_path)
    cmd = "sudo arc mount -m {}/arcadia/ -S {}/store/ --allow-other".format(
        mnt_path, mnt_path
    )
    subprocess.check_call(cmd.split())


@contextmanager
def atomic_write(path, mode=0o644):
    tmp = tempfile.NamedTemporaryFile(mode="w+", delete=False)

    try:
        yield tmp
    finally:
        tmp.close()
        os.chmod(tmp.name, mode)
        # mv to support cross device move
        subprocess.check_call(["mv", tmp.name, path])


def write_cache(cache):
    with atomic_write(CACHE_PATH) as cf:
        json.dump(cache, cf)


class ArcRepo:
    def __init__(self, token, projects, group, path, add_branches=[]):
        self.token = token
        self.projects = projects
        self.group = group
        self.path = path
        self.add_branches = add_branches

    def _call_arc_api(self, path):
        headers = {"Authorization": "OAuth {}".format(self.token)}
        url = "{}{}".format(API_URL, path)
        res = requests.get(url=url, headers=headers).json()

        logging.debug("Reply: {}".format(res))

        return res

    def _call_arc(self, params, cwd=ARC_REPO_PATH):
        cmd = ["sudo", ARC_BINARY_PATH] + params
        logging.debug("Calling arc util: {}".format(" ".join(cmd)))
        return subprocess.check_output(cmd, cwd=cwd)

    def _get_members(self):
        # Groups config can be found on arcadia/trunk/groups/
        res = self._call_arc_api("/v2/groups/{}/members".format(self.group))
        return [u["name"] for u in res["data"]]

    def export(self, commit, src, dst):
        if not os.path.exists(dst):
            os.makedirs(dst)

        tmp = tempfile.mkdtemp(prefix=dst + ".")
        self._call_arc(
            params=["export", commit, "--to", tmp], cwd=os.path.join(ARC_REPO_PATH, src)
        )
        os.rename(os.path.join(tmp, src), dst)
        shutil.rmtree(tmp)

    def get_branches(self):
        members = self._get_members()
        out = []

        for m in members:
            res = self._call_arc_api(
                "/v2/repos/arc/branches?name={}&fields=name,commit_id".format(m)
            )
            for branch in res["data"]:
                out.append(branch)

        for b in self.add_branches:
            commit_id = self.get_last_commit(self.path, b)
            out.append({"name": b, "commit_id": commit_id})

        logging.debug("Branches to process: {}".format(out))
        return out

    def add_checkout(self, commit):
        self._call_arc(["checkout", "-f", commit])

        for p in self.projects:
            dst_path = os.path.join(SALT_CHECKOUTS_PATH, commit, p)  # Absolute
            src_path = os.path.join(self.path, p)  # Relative to arc repo

            if not os.path.exists(dst_path) and os.path.exists(
                os.path.join(ARC_REPO_PATH, src_path)
            ):
                logging.debug("add checkout %s -> %s", src_path, dst_path)
                self.export(commit, src_path, dst_path)

    def remove_checkout(self, commit):
        rm_path = os.path.join(SALT_CHECKOUTS_PATH, commit)
        if os.path.exists(rm_path):
            logging.debug("remove checkout %s", rm_path)
            shutil.rmtree(rm_path)

    def update_checkout(self, commit):
        self.remove_checkout(commit)
        self.add_checkout(commit)

    def is_data_modified(self, commit):
        # Always return True in case of None in commit field
        if not commit:
            return True

        res = self._call_arc_api(
            "/v2/repos/arc/commits/{}/changes?context_size=3&fields=path".format(commit)
        )["data"]

        for r in res:
            if r["path"].startswith(self.path):
                return True

        return False

    def read_cache(self):
        try:
            with open(CACHE_PATH, "r") as cf:
                return json.load(cf)
        except Exception:
            logging.exception(
                "Got problems with reading cache file. Starting without cache"
            )
        return {}

    def process_cache_diff(self, cache, no_cache):
        logging.debug("process cache diff")
        old_cache = dict()
        try:
            if not no_cache:
                old_cache = self.read_cache()
        except Exception:
            pass

        if old_cache == cache:
            logging.debug("cache data not changed")
            return

        old_branches = set(old_cache.keys())
        new_branches = set(cache.keys())

        checkouts_to_remove = old_branches.difference(new_branches)
        logging.debug("going to remove %s checkouts", len(checkouts_to_remove))
        for b in checkouts_to_remove:
            self.remove_checkout(cache[b]["commit_id"])

        checkouts_to_add = {
            b for b in new_branches.difference(old_branches) if cache[b]["modified"]
        }
        logging.debug("going to add %s checkouts", len(checkouts_to_add))
        for b in checkouts_to_add:
            self.add_checkout(cache[b]["commit_id"])

        checkouts_to_update = {
            b
            for b in new_branches.intersection(old_branches)
            if old_cache[b] != cache[b] and cache[b]["modified"]
        }
        logging.debug("going to update %s checkouts", len(checkouts_to_update))
        for b in checkouts_to_update:
            self.update_checkout(cache[b]["commit_id"])

    def get_last_commit(self, path, branch="trunk"):
        self._call_arc(["checkout", "-f", branch])
        self._call_arc(["pull"])
        history = self._call_arc(["log", "-n1", "--json", path])
        history = json.loads(history)
        return history[0]["commit"]


def write_salt_config(cache):
    if not os.path.isdir(SALT_DYN_CONFIGS_PATH):
        logging.debug("Create %r", SALT_DYN_CONFIGS_PATH)
        os.mkdir(SALT_DYN_CONFIGS_PATH)

    conf_types = ["roots", "pillar"]

    for t in conf_types:
        with atomic_write(
            os.path.join(SALT_DYN_CONFIGS_PATH, "dynamic_{}_config.yaml".format(t))
        ) as f:
            out = dict()

            for branch in cache:
                if cache[branch]["salt_data"] and cache[branch]["modified"]:
                    out[branch] = [
                        os.path.join(
                            SALT_CHECKOUTS_PATH, cache[branch]["commit_id"], p, t
                        )
                        for p in config["projects"]
                    ]

            yaml.safe_dump(out, f, default_flow_style=False)


def parse_args():
    parser = argparse.ArgumentParser("Generate saltenvs from arcadia branches")
    parser.add_argument(
        "--no-cache",
        action="store_true",
        help="Ignore local cache, get all data from API",
    )
    parser.add_argument("--debug", action="store_true", help="Debug mode")
    parser.add_argument("--logfile", "-l", help="Log to file")

    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    logging.basicConfig(
        level=logging.DEBUG if args.debug else logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
        filename=args.logfile,
    )

    with open(CONFIG_PATH, "r") as cf:
        config = yaml.safe_load(cf)

    if not is_arc_mounted():
        mount_arc()
    else:
        logging.debug("arc mounted")

    add_branches = config["add-branches"] if "add-branches" in config.keys() else []

    repo = ArcRepo(
        config["arc-token"],
        config["projects"],
        config["arc-group"],
        config["arc-path"],
        add_branches,
    )
    branches = repo.get_branches()

    cache = {}
    if not args.no_cache:
        cache = repo.read_cache()

    for b in branches:
        modified = repo.is_data_modified(b["commit_id"])
        salt_data = modified or (b["name"] in cache and cache[b["name"]]["salt_data"])
        if not is_in_cache(b["name"], b["commit_id"], cache):
            cache.update(
                {
                    b["name"]: {
                        "commit_id": b["commit_id"],
                        "modified": modified,
                        "salt_data": salt_data,
                    }
                }
            )

    repo.process_cache_diff(cache, args.no_cache)

    write_salt_config(cache)
    write_cache(cache)
