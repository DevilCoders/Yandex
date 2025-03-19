#!/usr/bin/env python

import argparse
import glob
import json
import logging
import os
import shutil
import solomon
import sys
import webbrowser

AUTH_TOKEN_DIR = os.path.expanduser("~/tmp")
AUTH_TOKEN_FILENAME = "solomon-token.oauth"
AUTH_URL = "https://solomon.yandex-team.ru/api/internal/auth"


def read_auth_token():
    filename = os.path.join(AUTH_TOKEN_DIR, AUTH_TOKEN_FILENAME)
    if not os.path.exists(filename):
        return None
    with open(filename, "r") as f:
        return f.read()


def store_auth_token(token):
    if not os.path.exists(AUTH_TOKEN_DIR):
        os.makedirs(AUTH_TOKEN_DIR)
    filename = os.path.join(AUTH_TOKEN_DIR, AUTH_TOKEN_FILENAME)
    with open(filename, "w") as f:
        return f.write(token)


def ask_auth_token():
    webbrowser.open(AUTH_URL)
    token = raw_input("Please enter OAUTH token from {} > ".format(AUTH_URL))
    return token


def auth_token():
    token = os.environ.get("AUTH_TOKEN")
    if token:
        return token
    token = read_auth_token()
    if token:
        return token
    token = ask_auth_token()
    store_auth_token(token)
    return token


def remove_project_dir(root_dir, project_id):
    project_dir = os.path.join(root_dir, project_id)
    if os.path.exists(project_dir):
        shutil.rmtree(project_dir)


def make_object_dir(root_dir, project_id, object_type):
    object_dir = os.path.join(root_dir, project_id, object_type)

    if not os.path.exists(object_dir):
        os.makedirs(object_dir)

    return object_dir


class Main(object):

    def __init__(self):
        super(Main, self).__init__()
        self.args = None
        self.solomon = None

    def run(self):
        self.parse_args()

        level = logging.INFO
        if self.args.debug:
            level = logging.DEBUG

        log_config = {
            "format": "%(asctime)s - %(levelname)s - %(message)s",
            "level": level,
        }

        if self.args.log_filename:
            log_config["filename"] = self.args.log_filename

        logging.basicConfig(**log_config)

        try:
            return self.exec_command()
        except Exception:
            return 1

    def parse_args(self):
        parser = argparse.ArgumentParser(description="Solomon Backup Utility")

        parser.add_argument(
            "--debug",
            action="store_true",
            default=False,
            help="enable debug logs")

        parser.add_argument(
            "--log",
            action="store",
            dest="log_filename",
            help="print logs to file instead of stdout")

        parser.add_argument(
            "--dry-run",
            action="store_true",
            help="do not actually make changes")

        parser.add_argument(
            "--project",
            action="store",
            default="nbs",
            help="project name")

        parser.add_argument(
            "--root-dir",
            action="store",
            default=".",
            help="where to store data")

        cmd = parser.add_mutually_exclusive_group()

        cmd.add_argument(
            "--backup",
            action="store_true",
            default=False,
            help="backup project settings. It'll overwrite --root-dir contents")

        cmd.add_argument(
            "--restore",
            action="store_true",
            default=False,
            help="restore project settings. It'll update --root-dir contents with new version numbers")

        parser.add_argument(
            "--delete-remote",
            action="store_true",
            default=False,
            help="when restoring project, it'll remove objects not present in --root-dir in Solomon")

        self.args = parser.parse_args()

    def exec_command(self):
        try:
            if self.args.backup:
                return self.backup_all()
            if self.args.restore:
                return self.restore_all()

        except KeyboardInterrupt:
            logging.error("Command interrupted")
            return -1
        except BaseException as e:
            logging.error("Command failed: %s" % e, exc_info=True)
            return -1

    def backup_all(self):
        remove_project_dir(self.args.root_dir, self.args.project)

        self.solomon = solomon.Solomon(
            auth_token(),
            dry_run=self.args.dry_run)

        self.backup_objects("alert")
        self.backup_objects("channel")
        self.backup_objects("cluster")
        self.backup_objects("dashboard")
        self.backup_objects("graph")
        self.backup_object("menu")
        self.backup_object("project")
        self.backup_objects("service")
        self.backup_objects("shard")

    def backup_object(self, object_type):
        object_dir = make_object_dir(
            self.args.root_dir,
            self.args.project,
            object_type)

        get_object = getattr(self.solomon, "get_" + object_type)
        data = get_object(self.args.project)

        file_name = os.path.join(object_dir, object_type + ".json")
        with open(file_name, "w") as file:
            json.dump(data, file, indent=4, sort_keys=True)

    def backup_objects(self, object_type):
        object_dir = make_object_dir(
            self.args.root_dir,
            self.args.project,
            object_type + "s")

        get_objects = getattr(self.solomon, "get_" + object_type + "s")
        objects = get_objects(self.args.project)

        for obj in objects:
            get_object = getattr(self.solomon, "get_" + object_type)
            data = get_object(self.args.project, obj.object_id)

            file_name = os.path.join(object_dir, obj.object_id + ".json")
            with open(file_name, "w") as file:
                json.dump(data, file, indent=4, sort_keys=True)

    def restore_all(self):
        self.solomon = solomon.Solomon(
            auth_token(),
            dry_run=self.args.dry_run)

        self.restore_objects("alert")
        self.restore_objects("channel")
        self.restore_objects("cluster")
        self.restore_objects("dashboard")
        self.restore_objects("graph")
        self.restore_object("menu")
        self.restore_object("project")
        self.restore_objects("service")
        self.restore_objects("shard")

        # Re-backup everything to get new version numbers.
        self.backup_all()

    def restore_object(self, object_type):
        get_object = getattr(self.solomon, "get_" + object_type)
        put_object = getattr(self.solomon, "put_" + object_type)

        object_dir = make_object_dir(
            self.args.root_dir,
            self.args.project,
            object_type)

        file_name = os.path.join(object_dir, object_type + ".json")

        data = None
        with open(file_name, "r") as file:
            data = json.load(file)

        prev_data = get_object(self.args.project)
        if prev_data == data:
            # No need to change if data is the same.
            return

        put_object(self.args.project, data)

    def restore_objects(self, object_type):
        get_object = getattr(self.solomon, "get_" + object_type)
        get_objects = getattr(self.solomon, "get_" + object_type + "s")
        put_object = getattr(self.solomon, "put_" + object_type)
        post_object = getattr(self.solomon, "post_" + object_type)
        delete_object = getattr(self.solomon, "delete_" + object_type)

        object_dir = make_object_dir(
            self.args.root_dir,
            self.args.project,
            object_type + "s")

        seen_ids = set()

        for file_name in glob.glob(os.path.join(object_dir, "*.json")):
            object_id = os.path.basename(file_name)[:-5]
            seen_ids.add(object_id)

            data = None
            with open(file_name, "r") as file:
                data = json.load(file)

            prev_data = get_object(self.args.project, object_id)
            if prev_data == data:
                # No need to change if data is the same.
                continue

            # Make sure that id in data is the same as filename.
            data["id"] = object_id
            if prev_data:
                put_object(self.args.project, object_id, data)
            else:
                post_object(self.args.project, data)

        if self.args.delete_remote:
            for obj in get_objects(self.args.project):
                if obj.object_id in seen_ids:
                    continue
                delete_object(self.args.project, obj.object_id)


if __name__ == "__main__":
    sys.exit(Main().run())
